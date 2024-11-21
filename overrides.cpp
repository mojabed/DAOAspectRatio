#include "overrides.h"
#include <mutex>
#include "hooks.h"

typedef HRESULT(__stdcall* SetViewport_t)(IDirect3DDevice9*, const D3DVIEWPORT9*);
SetViewport_t originalSetViewport = nullptr;

std::unique_ptr<CustomDirect3D9> CustomDirect3D9::instance = nullptr;

std::mutex hookMutex;

CustomDirect3D9::~CustomDirect3D9() {
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    if (m_original) {
        m_original->Release();
        m_original = nullptr;
    }
}

// Intercepted SetViewport
HRESULT __stdcall CustomDirect3D9::SetViewportHook(IDirect3DDevice9* pDevice, const D3DVIEWPORT9* pViewport) {
    std::lock_guard<std::mutex> lock(hookMutex);

    // Modify viewport only when the game attempts to draw letterboxes (X = 440 at 3440x1440)
    if (pViewport->X == 440 && pViewport->Width != CustomDirect3D9::getInstance().m_viewport.Width) { // probably need to adjust X == 440 to account for larger displays
        pViewport = &CustomDirect3D9::getInstance().m_viewport;
        CustomDirect3D9::getInstance().setViewportFlag = true;
        return CustomDirect3D9::getInstance().originalSetViewport(pDevice, pViewport);

        int nScreenWidth, nScreenHeight;
        nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
        D3DVIEWPORT9 vp = { (nScreenWidth - 1024) / 2,(nScreenHeight - 768) / 2,1024,768,0,1 };
        pDevice->SetViewport(&vp);
    }
    CustomDirect3D9::getInstance().setViewportFlag = false;
    // Call the original SetViewport function
    return CustomDirect3D9::getInstance().originalSetViewport(pDevice, pViewport);
}

// Testing function
void LogMatrix(const D3DMATRIX& matrix) {
    Logger::getInstance().log("Matrix values:");
    for (int i = 0; i < 4; ++i) {
        std::string row;
        for (int j = 0; j < 4; ++j) {
            row += std::to_string(matrix.m[i][j]) + " ";
        }
        Logger::getInstance().log(row);
    }
}

D3DMATRIX CreateProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane) {
    D3DMATRIX projectionMatrix = {};

    float yScale = 1.0f / tanf(fov / 2.0f);
    float xScale = yScale / aspectRatio;

    projectionMatrix._11 = xScale;
    projectionMatrix._22 = yScale;
    projectionMatrix._33 = farPlane / (farPlane - nearPlane);
    projectionMatrix._34 = 1.0f;
    projectionMatrix._43 = -nearPlane * farPlane / (farPlane - nearPlane);

    return projectionMatrix;
}

// Intercepted SetTransform
HRESULT __stdcall CustomDirect3D9::SetTransformHook(IDirect3DDevice9* pDevice, D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) {
    std::lock_guard<std::mutex> lock(hookMutex);

    if (state == D3DTS_PROJECTION && CustomDirect3D9::getInstance().setViewportFlag) {

        //float modifiedFOV = D3DX_PI / 2; // 90 degrees field of view

        //float nearPlane = //matrix->_43 / (matrix->_33 - 1.0f);
        //float farPlane = //matrix->_43 / (matrix->_33 + 1.0f);

        //D3DMATRIX projectionMatrix = CreateProjectionMatrix(modifiedFOV, aspectRatio, nearPlane, farPlane);

        D3DMATRIX modifiedMatrix = *matrix;

        // Calculate the new scaling factors
        float originalAspectRatio = matrix->_22 / matrix->_11;
        float newAspectRatio = CustomDirect3D9::getInstance().aspectRatio;

        // Adjust the scaling factors
        modifiedMatrix._11 = matrix->_11 * (originalAspectRatio / newAspectRatio);
        modifiedMatrix._22 = matrix->_22;

        // Log the modified matrix
        Logger::getInstance().log("Modified projection matrix");
        LogMatrix(modifiedMatrix);
        //return CustomDirect3D9::getInstance().originalSetTransform(pDevice, state, &modifiedMatrix);
    }
    return CustomDirect3D9::getInstance().originalSetTransform(pDevice, state, matrix);
}

void CustomDirect3D9::HookSetViewport(IDirect3DDevice9* pDevice) {
    void** vtable = *reinterpret_cast<void***>(pDevice); // vtable of the IDirect3DDevice9 interface
    if (!vtable) {
        Logger::getInstance().logError("Failed to get vtable of IDirect3DDevice9");
        return;
    }

    HookManager<SetViewport_t> hook(vtable, 47, SetViewportHook);

    if (hook.apply()) {
        originalSetViewport = hook.getOriginal(); // grab the original function pointer
        Logger::getInstance().log("SetViewport hooked successfully");
    }
    else {
        Logger::getInstance().logError("Failed to hook SetViewport");
    }
}

void CustomDirect3D9::HookSetTransform(IDirect3DDevice9* pDevice) {
    void** vtable = *reinterpret_cast<void***>(pDevice);
    if (!vtable) {
        Logger::getInstance().logError("Failed to get vtable of IDirect3DDevice9");
        return;
    }

    HookManager<SetTransform_t> hook(vtable, 44, SetTransformHook);

    if (hook.apply()) {
        originalSetTransform = hook.getOriginal();
        Logger::getInstance().log("SetTransform hooked successfully");
    }
    else {
        Logger::getInstance().logError("Failed to hook SetTransform");
    }
}

HRESULT CustomDirect3D9::CreateDevice(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface) {

    //std::lock_guard<std::mutex> lock(hookMutex);
    Logger::getInstance().log("Calling original CreateDevice");
    HRESULT hr = m_original->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (SUCCEEDED(hr)) {
        Logger::getInstance().log("CreateDevice successful");

        m_device = *ppReturnedDeviceInterface;
        if (m_device) {
            HRESULT hr = m_device->GetViewport(&m_viewport);
            if (FAILED(hr)) {
                Logger::getInstance().logError("GetViewport failed with error code: " + std::to_string(hr));
                return hr;
            }
            aspectRatio = static_cast<float>(m_viewport.Width) / static_cast<float>(m_viewport.Height);
            // Hook the functions of interest
            HookSetViewport(m_device);
            HookSetTransform(m_device);
        }
        else {
            Logger::getInstance().logError("Failed to get device pointer");
        }
    }
    return hr;
}

HRESULT CustomDirect3D9::QueryInterface(REFIID riid, void** ppvObj) {
    return m_original->QueryInterface(riid, ppvObj);
}

ULONG CustomDirect3D9::AddRef() {
    return m_original->AddRef();
}

ULONG CustomDirect3D9::Release() {
    ULONG count = m_original->Release();
    if (count == 0) {
        delete this;
    }
    return count;
}

HRESULT CustomDirect3D9::RegisterSoftwareDevice(void* pInitializeFunction) {
    return m_original->RegisterSoftwareDevice(pInitializeFunction);
}

UINT CustomDirect3D9::GetAdapterCount() {
    return m_original->GetAdapterCount();
}

HRESULT CustomDirect3D9::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) {
    return m_original->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
}

UINT CustomDirect3D9::GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) {
    return m_original->GetAdapterModeCount(Adapter, Format);
}

HRESULT CustomDirect3D9::EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) {
    return m_original->EnumAdapterModes(Adapter, Format, Mode, pMode);
}

HRESULT CustomDirect3D9::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) {
    return m_original->GetAdapterDisplayMode(Adapter, pMode);
}

HRESULT CustomDirect3D9::CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed) {
    return m_original->CheckDeviceType(Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed);
}

HRESULT CustomDirect3D9::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    return m_original->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
}

HRESULT CustomDirect3D9::CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) {
    return m_original->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels);
}

HRESULT CustomDirect3D9::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    return m_original->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);
}

HRESULT CustomDirect3D9::CheckDeviceFormatConversion(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) {
    return m_original->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat, TargetFormat);
}

HRESULT CustomDirect3D9::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) {
    return m_original->GetDeviceCaps(Adapter, DeviceType, pCaps);
}

HMONITOR CustomDirect3D9::GetAdapterMonitor(UINT Adapter) {
    return m_original->GetAdapterMonitor(Adapter);
}
