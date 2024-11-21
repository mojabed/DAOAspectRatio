#pragma once

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "logger.h"

class CustomDirect3D9 : public IDirect3D9 {
public:
    ~CustomDirect3D9();
    using SetViewport_t = HRESULT(__stdcall*)(IDirect3DDevice9*, const D3DVIEWPORT9*);
    using SetTransform_t = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DTRANSFORMSTATETYPE, const D3DMATRIX*);

    static CustomDirect3D9& getInstance(IDirect3D9* original = nullptr) {
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            instance.reset(new CustomDirect3D9(original));
            });
        return *instance;
    }

    CustomDirect3D9(const CustomDirect3D9&) = delete;
    CustomDirect3D9& operator=(const CustomDirect3D9&) = delete;

    // Hooked functions
    static HRESULT __stdcall SetViewportHook(IDirect3DDevice9* device, const D3DVIEWPORT9* pViewport);
    static HRESULT __stdcall SetTransformHook(IDirect3DDevice9* device, D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix);

    void HookSetViewport(IDirect3DDevice9* pDevice);
    void HookSetTransform(IDirect3DDevice9* pDevice);

    IDirect3D9* GetOriginal() const { return m_original; }
    IDirect3DDevice9* GetDevice() const { return m_device; }

    // Currently overriden functions
    HRESULT STDMETHODCALLTYPE CreateDevice(
        UINT Adapter,
        D3DDEVTYPE DeviceType,
        HWND hFocusWindow,
        DWORD BehaviorFlags,
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        IDirect3DDevice9** ppReturnedDeviceInterface) override;

    // Implement all other pure virtual methods of IDirect3D9
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* pInitializeFunction) override;
    UINT STDMETHODCALLTYPE GetAdapterCount() override;
    HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) override;
    UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) override;
    HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) override;
    HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) override;
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) override;
    HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT Adapter) override;

private:
    CustomDirect3D9(IDirect3D9* original) : m_original(original), aspectRatio(1.0f), m_viewport{} {}

    IDirect3D9* m_original;
    IDirect3DDevice9* m_device = nullptr;
    D3DVIEWPORT9 m_viewport;

    SetViewport_t originalSetViewport = nullptr;
    SetTransform_t originalSetTransform = nullptr;

    static std::unique_ptr<CustomDirect3D9> instance;

    float aspectRatio;

    bool setViewportFlag = false;
};