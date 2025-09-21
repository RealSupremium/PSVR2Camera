#pragma once

#include "distortion.h"

#include <windows.h>
#include <stdexcept>

struct SharedMemoryData {
    HANDLE hMapFile = NULL;
    LPVOID pBuf = NULL;
    HANDLE hImageEvent = NULL;
    HANDLE hImageMutex = NULL;
    char* imageMemBase = nullptr;
};

void setup_shared_memory(SharedMemoryData& data);
void cleanup_shared_memory(SharedMemoryData& data);

// Get distortion configuration for a given camera ID
bool get_distortion_config(SharedMemoryData& data, int cameraId, CameraParameters& params, CameraIntrinsics& intrinsics);

// Waits for a new frame, finds the latest image buffer, and copies it to the destination.
// Returns true on success, false on failure.
bool copy_latest_image_buffer(SharedMemoryData& data, void* leftCameraData, void* rightCameraData, size_t cameraDataSize);