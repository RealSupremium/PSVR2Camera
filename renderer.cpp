#include "renderer.h"
#include "distortion.h"
#include <d3dcompiler.h>
#include <iostream>

using Microsoft::WRL::ComPtr;

const int IMAGE_WIDTH = 1016 * 2; // Changed to double width
const int IMAGE_HEIGHT = 1016;
const size_t BC4_DATA_SIZE = (1016 * 1016) / 2; // BC4 is 4 bits per pixel

DisplayMode g_displayMode = DisplayMode::SideBySide;

ComPtr<ID3D11Device>             g_pd3dDevice;
ComPtr<ID3D11DeviceContext>      g_pImmediateContext;
ComPtr<IDXGISwapChain>           g_pSwapChain;
ComPtr<ID3D11RenderTargetView>   g_pRenderTargetView;
ComPtr<ID3D11Texture2D>          g_pBC4Texture1;
ComPtr<ID3D11ShaderResourceView> g_pBC4TextureSRV1;
ComPtr<ID3D11Texture2D>          g_pBC4Texture2;
ComPtr<ID3D11ShaderResourceView> g_pBC4TextureSRV2;
ComPtr<ID3D11SamplerState>       g_pSamplerState;

ComPtr<ID3D11Buffer>             g_pUndistortVertexBuffer;
ComPtr<ID3D11Buffer>             g_pUndistortIndexBuffer;
ComPtr<ID3D11VertexShader>       g_pUndistortVertexShader;
ComPtr<ID3D11PixelShader>        g_pUndistortPixelShader;
ComPtr<ID3D11InputLayout>        g_pUndistortInputLayout;
UINT                             g_uUndistortIndexCount = 0;

ComPtr<ID3D11Buffer>             g_pUndistortVertexBuffer2;
ComPtr<ID3D11Buffer>             g_pUndistortIndexBuffer2;
UINT                             g_uUndistortIndexCount2 = 0;

void Resize(UINT width, UINT height)
{
    if (g_pSwapChain)
    {
        g_pImmediateContext->OMSetRenderTargets(0, 0, 0);

        g_pRenderTargetView.Reset();

        g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        
        ComPtr<ID3D11Texture2D> pBuffer;
        g_pSwapChain->GetBuffer(0, __uuidof( ID3D11Texture2D), &pBuffer);

        g_pd3dDevice->CreateRenderTargetView( pBuffer.Get(), NULL, &g_pRenderTargetView);

        g_pImmediateContext->OMSetRenderTargets( 1, g_pRenderTargetView.GetAddressOf(), NULL );
    }
}

void UpdateMeshes(SharedMemoryData& sharedMemoryData, float zoomFactor) {
    // Camera 1
    CameraParameters cam_params;
    CameraIntrinsics cam_intrinsics;
    if (!get_distortion_config(sharedMemoryData, 0, cam_params, cam_intrinsics)) {
        return;
    }

    std::vector<UndistortVertex> vertices;
    std::vector<DWORD> indices;
    create_undistortion_mesh(1016, 1016, zoomFactor, cam_intrinsics, cam_params, vertices, indices);
    g_uUndistortIndexCount = static_cast<UINT>(indices.size());

    g_pUndistortVertexBuffer.Reset();
    g_pUndistortIndexBuffer.Reset();

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth = sizeof(UndistortVertex) * (UINT)vertices.size();
    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices.data();
    g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pUndistortVertexBuffer);

    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.ByteWidth = sizeof(DWORD) * (UINT)indices.size();
    InitData.pSysMem = indices.data();
    g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pUndistortIndexBuffer);

    // Camera 2
    CameraParameters cam_params2;
    CameraIntrinsics cam_intrinsics2;
    if (!get_distortion_config(sharedMemoryData, 1, cam_params2, cam_intrinsics2)) {
        return;
    }

    std::vector<UndistortVertex> vertices2;
    std::vector<DWORD> indices2;
    create_undistortion_mesh(1016, 1016, zoomFactor, cam_intrinsics2, cam_params2, vertices2, indices2);
    g_uUndistortIndexCount2 = static_cast<UINT>(indices2.size());

    g_pUndistortVertexBuffer2.Reset();
    g_pUndistortIndexBuffer2.Reset();

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth = sizeof(UndistortVertex) * (UINT)vertices2.size();
    InitData.pSysMem = vertices2.data();
    g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pUndistortVertexBuffer2);

    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.ByteWidth = sizeof(DWORD) * (UINT)indices2.size();
    InitData.pSysMem = indices2.data();
    g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pUndistortIndexBuffer2);
}

HRESULT InitDevice(HWND hWnd, SharedMemoryData& sharedMemoryData, float zoomFactor) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = IMAGE_WIDTH;
    sd.BufferDesc.Height = IMAGE_HEIGHT;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pImmediateContext);
    if (FAILED(hr)) return hr;

    ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)pBackBuffer.GetAddressOf());
    if (FAILED(hr)) return hr;
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, &g_pRenderTargetView);
    if (FAILED(hr)) return hr;
    g_pImmediateContext->OMSetRenderTargets(1, g_pRenderTargetView.GetAddressOf(), NULL);

    const char* undistort_shader_code = R"(
        Texture2D txDiffuse : register(t0);
        SamplerState samLinear : register(s0);

        struct VS_INPUT {
            float4 Pos : POSITION;
            float2 Tex : TEXCOORD0;
        };

        struct PS_INPUT {
            float4 Pos : SV_POSITION;
            float2 Tex : TEXCOORD0;
        };

        PS_INPUT VS(VS_INPUT input) {
            PS_INPUT output;
            output.Pos = input.Pos;
            output.Tex = input.Tex;
            return output;
        }

        float4 PS(PS_INPUT input) : SV_Target {
            if (input.Tex.x < 0.0 || input.Tex.x > 1.0 || input.Tex.y < 0.0 || input.Tex.y > 1.0) {
                return float4(0.0, 0.0, 0.0, 1.0);
            }
            float val = pow(txDiffuse.Sample(samLinear, input.Tex).r, 0.75f) * 4.0f - 0.5f;
            return float4(val, val, val, 1.0f);
        }
    )";

    ComPtr<ID3DBlob> pVSBlob, pPSBlob, pErrorBlob;
    hr = D3DCompile(undistort_shader_code, strlen(undistort_shader_code), NULL, NULL, NULL, "VS", "vs_5_0", 0, 0, &pVSBlob, &pErrorBlob);
    if (FAILED(hr)) { if(pErrorBlob) MessageBoxA(NULL, (char*)pErrorBlob->GetBufferPointer(), "Shader Error", MB_OK); return hr; }
    
    hr = D3DCompile(undistort_shader_code, strlen(undistort_shader_code), NULL, NULL, NULL, "PS", "ps_5_0", 0, 0, &pPSBlob, &pErrorBlob);
    if (FAILED(hr)) { if(pErrorBlob) MessageBoxA(NULL, (char*)pErrorBlob->GetBufferPointer(), "Shader Error", MB_OK); return hr; }

    g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pUndistortVertexShader);
    g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pUndistortPixelShader);

    D3D11_INPUT_ELEMENT_DESC undistort_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = g_pd3dDevice->CreateInputLayout(undistort_layout, ARRAYSIZE(undistort_layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pUndistortInputLayout);
    if (FAILED(hr)) return hr;

    UpdateMeshes(sharedMemoryData, zoomFactor);

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 1016;
    texDesc.Height = 1016;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_BC4_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DYNAMIC;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = g_pd3dDevice->CreateTexture2D(&texDesc, NULL, &g_pBC4Texture1);
    if (FAILED(hr)) return hr;
    hr = g_pd3dDevice->CreateShaderResourceView(g_pBC4Texture1.Get(), NULL, &g_pBC4TextureSRV1);
    if (FAILED(hr)) return hr;
    
    hr = g_pd3dDevice->CreateTexture2D(&texDesc, NULL, &g_pBC4Texture2);
    if (FAILED(hr)) return hr;
    hr = g_pd3dDevice->CreateShaderResourceView(g_pBC4Texture2.Get(), NULL, &g_pBC4TextureSRV2);
    if (FAILED(hr)) return hr;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerState);
    if (FAILED(hr)) return hr;

    return hr;
}

void Render(SharedMemoryData& sharedMemoryData) {
    D3D11_MAPPED_SUBRESOURCE mappedResourceL;
    D3D11_MAPPED_SUBRESOURCE mappedResourceR;
    if (SUCCEEDED(g_pImmediateContext->Map(g_pBC4Texture1.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceL)) 
        && SUCCEEDED(g_pImmediateContext->Map(g_pBC4Texture2.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResourceR))) {
        copy_latest_image_buffer(sharedMemoryData, mappedResourceL.pData, mappedResourceR.pData, BC4_DATA_SIZE);
        g_pImmediateContext->Unmap(g_pBC4Texture1.Get(), 0);
        g_pImmediateContext->Unmap(g_pBC4Texture2.Get(), 0);
    }

    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView.Get(), clear_color);

    ComPtr<ID3D11Texture2D> pBackBuffer;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
    D3D11_TEXTURE2D_DESC backBufferDesc;
    pBackBuffer->GetDesc(&backBufferDesc);

    float window_width = static_cast<float>(backBufferDesc.Width);
    float window_height = static_cast<float>(backBufferDesc.Height);

    D3D11_VIEWPORT vp;
    float image_aspect_ratio = 1.0f; // 1016 / 1016
    float viewport_width, viewport_height;

    if (g_displayMode == DisplayMode::SideBySide) {
        viewport_width = window_width / 2.0f;
        viewport_height = viewport_width / image_aspect_ratio;
    } else {
        viewport_width = window_width;
        viewport_height = viewport_width / image_aspect_ratio;
    }

    vp.Width = viewport_width;
    vp.Height = viewport_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftY = (window_height - viewport_height) / 2.0f;


    UINT stride = sizeof(UndistortVertex);
    UINT offset = 0;
    g_pImmediateContext->IASetInputLayout(g_pUndistortInputLayout.Get());
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pImmediateContext->VSSetShader(g_pUndistortVertexShader.Get(), NULL, 0);
    g_pImmediateContext->PSSetShader(g_pUndistortPixelShader.Get(), NULL, 0);
    g_pImmediateContext->PSSetSamplers(0, 1, g_pSamplerState.GetAddressOf());

    if (g_displayMode == DisplayMode::Camera1) {
        vp.TopLeftX = 0;
        g_pImmediateContext->RSSetViewports(1, &vp);
        g_pImmediateContext->PSSetShaderResources(0, 1, g_pBC4TextureSRV1.GetAddressOf());
        g_pImmediateContext->IASetVertexBuffers(0, 1, g_pUndistortVertexBuffer.GetAddressOf(), &stride, &offset);
        g_pImmediateContext->IASetIndexBuffer(g_pUndistortIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        g_pImmediateContext->DrawIndexed(g_uUndistortIndexCount, 0, 0);
    } else if (g_displayMode == DisplayMode::Camera2) {
        vp.TopLeftX = 0;
        g_pImmediateContext->RSSetViewports(1, &vp);
        g_pImmediateContext->PSSetShaderResources(0, 1, g_pBC4TextureSRV2.GetAddressOf());
        g_pImmediateContext->IASetVertexBuffers(0, 1, g_pUndistortVertexBuffer2.GetAddressOf(), &stride, &offset);
        g_pImmediateContext->IASetIndexBuffer(g_pUndistortIndexBuffer2.Get(), DXGI_FORMAT_R32_UINT, 0);
        g_pImmediateContext->DrawIndexed(g_uUndistortIndexCount2, 0, 0);
    } else { // SideBySide
        // Draw left camera
        vp.TopLeftX = 0;
        g_pImmediateContext->RSSetViewports(1, &vp);
        g_pImmediateContext->PSSetShaderResources(0, 1, g_pBC4TextureSRV1.GetAddressOf());
        g_pImmediateContext->IASetVertexBuffers(0, 1, g_pUndistortVertexBuffer.GetAddressOf(), &stride, &offset);
        g_pImmediateContext->IASetIndexBuffer(g_pUndistortIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        g_pImmediateContext->DrawIndexed(g_uUndistortIndexCount, 0, 0);

        // Draw right camera
        vp.TopLeftX = window_width / 2.0f;
        g_pImmediateContext->RSSetViewports(1, &vp);
        g_pImmediateContext->PSSetShaderResources(0, 1, g_pBC4TextureSRV2.GetAddressOf());
        g_pImmediateContext->IASetVertexBuffers(0, 1, g_pUndistortVertexBuffer2.GetAddressOf(), &stride, &offset);
        g_pImmediateContext->IASetIndexBuffer(g_pUndistortIndexBuffer2.Get(), DXGI_FORMAT_R32_UINT, 0);
        g_pImmediateContext->DrawIndexed(g_uUndistortIndexCount2, 0, 0);
    }

    g_pSwapChain->Present(1, 0);
}

void CleanupDevice() {
    if (g_pImmediateContext) g_pImmediateContext->ClearState();
    
    g_pUndistortInputLayout.Reset();
    g_pUndistortPixelShader.Reset();
    g_pUndistortVertexShader.Reset();
    g_pUndistortIndexBuffer.Reset();
    g_pUndistortVertexBuffer.Reset();
    g_pUndistortIndexBuffer2.Reset();
    g_pUndistortVertexBuffer2.Reset();
    g_pSamplerState.Reset();
    g_pBC4TextureSRV1.Reset();
    g_pBC4Texture1.Reset();
    g_pBC4TextureSRV2.Reset();
    g_pBC4Texture2.Reset();
    g_pRenderTargetView.Reset();
    g_pSwapChain.Reset();
    g_pImmediateContext.Reset();
    g_pd3dDevice.Reset();
}