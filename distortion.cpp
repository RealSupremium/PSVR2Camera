#include "distortion.h"

using namespace DirectX;

DirectX::XMFLOAT2 get_distorted_point(double x, double y, const CameraParameters& params) {
    const auto& p = params.coeffs;

    double xSq = x * x;
    double ySq = y * y;
    double rSq = xSq + ySq;
    double two_xy = 2.0 * x * y;
    double rSq_p4 = rSq * rSq * rSq * rSq;
    double rSq_p5 = rSq_p4 * rSq;

    double numPoly = 1.0 +
        (((p[4] * rSq + p[1]) * rSq + p[0]) * rSq) +
        (p[16] * rSq_p5 * rSq) +
        (p[15] * rSq_p5) +
        (p[14] * rSq_p4);

    double denPoly = 1.0 +
        (((p[7] * rSq + p[6]) * rSq + p[5]) * rSq) +
        (p[19] * rSq_p5 * rSq) +
        (p[18] * rSq_p5) +
        (p[17] * rSq_p4);

    double radialScale = (abs(denPoly) > 1e-9) ? (numPoly / denPoly) : 1.0;

    double distortedXTerm = (((p[9] * rSq + p[8]) * rSq)) +
                             ((xSq + xSq + rSq) * p[3]) +
                             (radialScale * x) +
                             (two_xy * p[2]);

    double distortedYTerm = (((p[11] * rSq + p[10]) * rSq)) +
                             (two_xy * p[3]) +
                             ((ySq + ySq + rSq) * p[2]) +
                             (radialScale * y);

    XMMATRIX Rx = XMMatrixRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), static_cast<float>(p[12]));
    XMMATRIX Ry = XMMatrixRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), static_cast<float>(p[13]));
    XMMATRIX R = XMMatrixMultiply(Rx, Ry);

    XMVECTOR pIn = XMVectorSet(static_cast<float>(distortedXTerm), static_cast<float>(distortedYTerm), 1.0f, 0.0f);
    XMVECTOR pOut = XMVector3Transform(pIn, R);

    float w = XMVectorGetZ(pOut);
    if (abs(w) < 1e-9) {
        return { -10.0f, -10.0f };
    }

    XMFLOAT2 final_p;
    final_p.x = XMVectorGetX(pOut) / w;
    final_p.y = XMVectorGetY(pOut) / w;
    
    return final_p;
}

void create_undistortion_mesh(
    int imageWidth, int imageHeight,
    float zoomFactor,
    const CameraIntrinsics& intrinsics,
    const CameraParameters& params,
    std::vector<UndistortVertex>& outVertices,
    std::vector<DWORD>& outIndices,
    DWORD meshDensityX,
    DWORD meshDensityY)
{
    outVertices.clear();
    outIndices.clear();

    for (DWORD j = 0; j <= meshDensityY; ++j) {
        for (DWORD i = 0; i <= meshDensityX; ++i) {
            float uOut = static_cast<float>(i) / meshDensityX;
            float vOut = static_cast<float>(j) / meshDensityY;
            float pxOut = uOut * imageWidth;
            float pyOut = vOut * imageHeight;

            UndistortVertex v;
            v.Pos.x = uOut * 2.0f - 1.0f;
            v.Pos.y = (1.0f - vOut) * 2.0f - 1.0f;
            v.Pos.z = 0.0f;

            double xNorm = (pxOut - intrinsics.cx) / intrinsics.fx;
            double yNorm = (pyOut - intrinsics.cy) / intrinsics.fy;

            DirectX::XMFLOAT2 distorted_norm = get_distorted_point(xNorm * zoomFactor, yNorm * zoomFactor, params);

            float srcPx = static_cast<float>(distorted_norm.x * intrinsics.fx + intrinsics.cx);
            float srcPy = static_cast<float>(distorted_norm.y * intrinsics.fy + intrinsics.cy);

            v.Tex.x = srcPx / imageWidth;
            v.Tex.y = srcPy / imageHeight;

            outVertices.push_back(v);
        }
    }

    for (DWORD j = 0; j < meshDensityY; ++j) {
        for (DWORD i = 0; i < meshDensityX; ++i) {
            DWORD topLeft = j * (meshDensityX + 1) + i;
            DWORD topRight = topLeft + 1;
            DWORD bottomLeft = (j + 1) * (meshDensityX + 1) + i;
            DWORD bottomRight = bottomLeft + 1;

            outIndices.push_back(topLeft);
            outIndices.push_back(topRight);
            outIndices.push_back(bottomLeft);

            outIndices.push_back(bottomLeft);
            outIndices.push_back(topRight);
            outIndices.push_back(bottomRight);
        }
    }
}