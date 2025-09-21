#include "shared_memory.h"
#include <iostream>

const char* FILE_MAPPING_NAME = "SHARE_VRT2_WIN";
const DWORD SHARED_MEM_SIZE = 0x2000000; // 32MB
const char* EVENT_NAME = "SHARE_VRT2_WIN_IMAGE_EVT";
const char* MUTEX_NAME = "SHARE_VRT2_WIN_IMAGE_MTX";
const DWORD IMAGE_BUFFER_OFFSET = 0x10ba00 + 256;
const size_t PER_CAMERA_BUFFER_STRIDE = 0x200100;

#pragma pack(push, 1)
struct CameraConfig {
    uint32_t camId;
    uint16_t widthPx;
    uint16_t heightPx;
    float pxMat[9];
    double coff[20];
    uint32_t zeros[6];
};
#pragma pack(pop)

void setup_shared_memory(SharedMemoryData& data) {
    try {
        data.hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, FILE_MAPPING_NAME);
        if (!data.hMapFile) throw std::runtime_error("Could not open file mapping object. Is your PSVR2 and SteamVR on?");

        data.pBuf = MapViewOfFile(data.hMapFile, FILE_MAP_READ, 0, 0, SHARED_MEM_SIZE);
        if (!data.pBuf) throw std::runtime_error("Could not map view of file.");

        data.imageMemBase = static_cast<char*>(data.pBuf);

        data.hImageEvent = OpenEventA(SYNCHRONIZE, FALSE, EVENT_NAME);
        data.hImageMutex = OpenMutexA(SYNCHRONIZE, FALSE, MUTEX_NAME);

        if (!data.hImageEvent || !data.hImageMutex) {
            throw std::runtime_error("Failed to open sync objects.");
        }
    } catch (const std::runtime_error&) {
        cleanup_shared_memory(data);
        throw;
    }
}

void cleanup_shared_memory(SharedMemoryData& data) {
    if (data.hImageEvent) CloseHandle(data.hImageEvent);
    if (data.hImageMutex) CloseHandle(data.hImageMutex);
    if (data.pBuf) UnmapViewOfFile(data.pBuf);
    if (data.hMapFile) CloseHandle(data.hMapFile);
}

bool get_distortion_config(SharedMemoryData& data, int cameraId, CameraParameters& params, CameraIntrinsics& intrinsics)
{
    // We should open, lock, get data, then unlock the SHARE_VRT2_WIN_CALIB_MTX mutex
    HANDLE hCalibMutex = OpenMutexA(SYNCHRONIZE, FALSE, "SHARE_VRT2_WIN_CALIB_MTX");
    if (!hCalibMutex) {
        std::cerr << "Failed to open calibration mutex." << std::endl;
        return false;
    }

    if (WaitForSingleObject(hCalibMutex, INFINITE) != WAIT_OBJECT_0) {
        CloseHandle(hCalibMutex);
        std::cerr << "Failed to acquire calibration mutex." << std::endl;
        return false;
    }

    // The camera config data is an array of CameraConfig structs
    // The base address for this array is data.imageMemBase + 0x524
    // Each CameraConfig struct is 0x70 bytes long
    const CameraConfig* configs = reinterpret_cast<const CameraConfig*>(static_cast<char*>(data.pBuf) + 0x524);

    bool found = false;
    for (int i = 0; i < 4; ++i) {
        if (configs[i].camId == cameraId) {
            // Populate CameraParameters
            for (int j = 0; j < 20; ++j) {
                params.coeffs[j] = configs[i].coff[j];
            }

            // Populate CameraIntrinsics
            // Matrix looks like this:
            // fx  0   cx
            // 0   fy  cy
            // 0   0   1
            
            intrinsics.fx = configs[i].pxMat[0];
            intrinsics.fy = configs[i].pxMat[4];
            intrinsics.cx = configs[i].pxMat[2];
            intrinsics.cy = configs[i].pxMat[5];
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Camera ID " << cameraId << " not found in shared memory." << std::endl;
        ReleaseMutex(hCalibMutex);
        CloseHandle(hCalibMutex);
        return false;
    }
    
    if (!ReleaseMutex(hCalibMutex)) {
        std::cerr << "Failed to release calibration mutex." << std::endl;
        CloseHandle(hCalibMutex);
        return false;
    }

    CloseHandle(hCalibMutex);
    return true;
}

bool copy_latest_image_buffer(SharedMemoryData& data, void* leftCameraData, void* rightCameraData, size_t cameraDataSize) {
    if (WaitForSingleObject(data.hImageEvent, INFINITE) != WAIT_OBJECT_0) return false;
    if (WaitForSingleObject(data.hImageMutex, INFINITE) != WAIT_OBJECT_0) return false;

    uint32_t latestTimestamp = 0;
    uint32_t latestIndex = 0;

    // Get the most recent camera frame based on which entry is the latest by timestamp.
    if (*(int *)(data.imageMemBase + 0x3c10) - 1U < 2) { latestTimestamp = *(uint32_t *)(data.imageMemBase + 0x3c18); latestIndex = 0; }
    if ((*(int *)(data.imageMemBase + 0x4488) - 1U < 2) && (latestTimestamp <= *(uint32_t *)(data.imageMemBase + 0x4490))) { latestIndex = 1; latestTimestamp = *(uint32_t *)(data.imageMemBase + 0x4490); }
    if ((*(int *)(data.imageMemBase + 0x4d00) - 1U < 2) && (latestTimestamp <= *(uint32_t *)(data.imageMemBase + 0x4d08))) { latestIndex = 2; latestTimestamp = *(uint32_t *)(data.imageMemBase + 0x4d08); }
    if ((*(int *)(data.imageMemBase + 0x5578) - 1U < 2) && (latestTimestamp <= *(uint32_t *)(data.imageMemBase + 0x5580))) { latestIndex = 3; latestTimestamp = *(uint32_t *)(data.imageMemBase + 0x5580); }
    if ((*(int *)(data.imageMemBase + 0x5df0) - 1U < 2) && (latestTimestamp <= *(uint32_t *)(data.imageMemBase + 0x5df8))) { latestIndex = 4; latestTimestamp = *(uint32_t *)(data.imageMemBase + 0x5df8); }
    if ((*(int *)(data.imageMemBase + 0x6668) - 1U < 2) && (latestTimestamp <= *(uint32_t *)(data.imageMemBase + 0x6670))) { latestIndex = 5; latestTimestamp = *(uint32_t *)(data.imageMemBase + 0x6670); }
    if ((*(int *)(data.imageMemBase + 0x6ee0) - 1U < 2) && (latestTimestamp <= *(uint32_t *)(data.imageMemBase + 0x6ee8))) { latestIndex = 6; latestTimestamp = *(uint32_t *)(data.imageMemBase + 0x6ee8); }
    if ((*(int *)(data.imageMemBase + 0x7758) - 1U < 2) && (latestTimestamp <= *(uint32_t *)(data.imageMemBase + 0x7760))) { latestIndex = 7; }

    const char* data_ptr = data.imageMemBase + IMAGE_BUFFER_OFFSET + (PER_CAMERA_BUFFER_STRIDE * latestIndex);
    memcpy(leftCameraData, data_ptr, cameraDataSize);
    memcpy(rightCameraData, data_ptr + cameraDataSize, cameraDataSize);
    
    if (!ReleaseMutex(data.hImageMutex)) return false;

    return true;
}