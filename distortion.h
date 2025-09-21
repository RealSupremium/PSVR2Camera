#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <array>

struct UndistortVertex {
    DirectX::XMFLOAT3 Pos; // Position in NDC [-1, 1]
    DirectX::XMFLOAT2 Tex; // UV coords [0, 1] for the distorted source texture
};

struct CameraParameters {
    double coeffs[20];
};

struct CameraIntrinsics {
    double cx, fx, cy, fy;
};

DirectX::XMFLOAT2 get_distorted_point(double x, double y, const CameraParameters& params);

DirectX::XMFLOAT2 get_undistorted_point_iterative(double dx_norm, double dy_norm, const CameraParameters& params, const CameraIntrinsics& intrinsics);

void create_undistortion_mesh(
    int imageWidth, int imageHeight,
    float zoomFactor,
    const CameraIntrinsics& intrinsics,
    const CameraParameters& params,
    std::vector<UndistortVertex>& outVertices,
    std::vector<DWORD>& outIndices,
    DWORD meshDensityX = 256,
    DWORD meshDensityY = 256
);