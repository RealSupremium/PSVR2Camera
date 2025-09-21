#include <windows.h>
#include <iostream>
#include <stdexcept>
#include "renderer.h"
#include "shared_memory.h"
#include "distortion.h"

SharedMemoryData sharedMemoryData;
float zoomFactor = 2.0f;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            switch (wParam) {
                case '1':
                    g_displayMode = DisplayMode::Camera1;
                    break;
                case '2':
                    g_displayMode = DisplayMode::Camera2;
                    break;
                case '3':
                    g_displayMode = DisplayMode::SideBySide;
                    break;
                case VK_OEM_MINUS:
                    if (zoomFactor > 0.1f) {
                        zoomFactor -= 0.1f;
                        UpdateMeshes(sharedMemoryData, zoomFactor);
                    }
                    break;
                case VK_OEM_PLUS:
                    zoomFactor += 0.1f;
                    UpdateMeshes(sharedMemoryData, zoomFactor);
                    break;
            }
            return 0;
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
            {
                Resize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
        setup_shared_memory(sharedMemoryData);
    } catch (const std::runtime_error& e) {
        MessageBoxA(NULL, e.what(), "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "BC4Viewer", NULL };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow(wc.lpszClassName, "PSVR2 Camera Viewer", WS_OVERLAPPEDWINDOW, 100, 100, IMAGE_WIDTH, IMAGE_HEIGHT, NULL, NULL, wc.hInstance, NULL);

    if (FAILED(InitDevice(hWnd, sharedMemoryData, zoomFactor))) {
        CleanupDevice();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {0};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        Render(sharedMemoryData);
    }

    cleanup_shared_memory(sharedMemoryData);
    CleanupDevice();
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 0;
}