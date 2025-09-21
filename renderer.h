#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "shared_memory.h"

extern const int IMAGE_WIDTH;
extern const int IMAGE_HEIGHT;
extern const size_t BC4_DATA_SIZE;

extern Microsoft::WRL::ComPtr<ID3D11Device>            g_pd3dDevice;
extern Microsoft::WRL::ComPtr<ID3D11DeviceContext>     g_pImmediateContext;
extern Microsoft::WRL::ComPtr<IDXGISwapChain>          g_pSwapChain;
extern Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  g_pRenderTargetView;

enum class DisplayMode {
    Camera1,
    Camera2,
    SideBySide
};

extern DisplayMode g_displayMode;

HRESULT InitDevice(HWND hWnd, SharedMemoryData& sharedMemoryData, float zoomFactor);
void CleanupDevice();
void Render(SharedMemoryData& sharedMemoryData);
void Resize(UINT width, UINT height);
void UpdateMeshes(SharedMemoryData& sharedMemoryData, float zoomFactor, bool useDistortion = true);