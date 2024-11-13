#include <windows.h>
#include <dbghelp.h>
#include <mutex>
#include "d3d9proxy.h"
#include "logger.h"

#pragma comment(lib, "d3d9.lib")

HMODULE hOriginalD3D9 = NULL;

const char* REL_JMP = "\xE9";
const unsigned int SIZE_OF_REL_JMP = 5;

typedef IDirect3D9* (WINAPI* Direct3DCreate9_t)(UINT);
Direct3DCreate9_t originalDirect3DCreate9 = nullptr;

typedef HRESULT(__stdcall* SetViewport_t)(IDirect3DDevice9*, const D3DVIEWPORT9*);
SetViewport_t originalSetViewport = nullptr;

std::mutex hookMutex;

HRESULT __stdcall SetViewportHook(IDirect3DDevice9* pDevice, const D3DVIEWPORT9* pViewport) {
	Logger::getInstance().log("SetViewportHook called");

	// Modify the viewport as needed
	D3DVIEWPORT9 modifiedViewport = *pViewport;
	modifiedViewport.X = 0;
	modifiedViewport.Y = 0;
	// need to get original width and height
	modifiedViewport.Width = 3440;
	modifiedViewport.Height = 1440;
	modifiedViewport.MinZ = 0.0f;
	modifiedViewport.MaxZ = 1.0f;

	// Call the original SetViewport function
	//std::lock_guard<std::mutex> lock(hookMutex);
	return originalSetViewport(pDevice, &modifiedViewport);
}

void HookSetViewport(IDirect3DDevice9* pDevice) {
	Logger::getInstance().log("HookSetViewport called");
	// Get the vtable of the IDirect3DDevice9 interface
	void** vtable = *reinterpret_cast<void***>(pDevice);
	if (!vtable) {
		Logger::getInstance().logError("Failed to get vtable of IDirect3DDevice9");
		return;
	}

	// Save the original SetViewport function pointer
	{
		//std::lock_guard<std::mutex> lock(hookMutex);
		originalSetViewport = reinterpret_cast<SetViewport_t>(vtable[47]);
		if (!originalSetViewport) {
			Logger::getInstance().logError("Failed to get original SetViewport function pointer");
			return;
		}

		// Replace the SetViewport function pointer with hook function
		DWORD oldProtect;
		if (!VirtualProtect(&vtable[47], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
			Logger::getInstance().logError("Failed to change memory protection for vtable");
			return;
		}
		vtable[47] = reinterpret_cast<void*>(&SetViewportHook);
		if (!VirtualProtect(&vtable[47], sizeof(void*), oldProtect, &oldProtect)) {
			Logger::getInstance().logError("Failed to restore memory protection for vtable");
			return;
		}
	}

	Logger::getInstance().log("SetViewport hooked");
}

class CustomDirect3D9 : public IDirect3D9 {
public:
	CustomDirect3D9(IDirect3D9* original) : m_original(original) {}

	// Override the CreateDevice method
	HRESULT STDMETHODCALLTYPE CreateDevice(
		UINT Adapter,
		D3DDEVTYPE DeviceType,
		HWND hFocusWindow,
		DWORD BehaviorFlags,
		D3DPRESENT_PARAMETERS* pPresentationParameters,
		IDirect3DDevice9** ppReturnedDeviceInterface) override {

		HRESULT hr = m_original->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
		if (SUCCEEDED(hr)) {
			// Hook the SetViewport function
			Logger::getInstance().log("CreateDevice succeeded\nCalling HookSetViewport");
			HookSetViewport(*ppReturnedDeviceInterface);
		}
		return hr;
	}

	// Implement all other pure virtual methods of IDirect3D9
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override {
		return m_original->QueryInterface(riid, ppvObj);
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return m_original->AddRef();
	}

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG count = m_original->Release();
		if (count == 0) {
			delete this;
		}
		return count;
	}

	HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* pInitializeFunction) override {
		return m_original->RegisterSoftwareDevice(pInitializeFunction);
	}

	UINT STDMETHODCALLTYPE GetAdapterCount() override {
		return m_original->GetAdapterCount();
	}

	HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) override {
		return m_original->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
	}

	UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) override {
		return m_original->GetAdapterModeCount(Adapter, Format);
	}

	HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) override {
		return m_original->EnumAdapterModes(Adapter, Format, Mode, pMode);
	}

	HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) override {
		return m_original->GetAdapterDisplayMode(Adapter, pMode);
	}

	HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed) override {
		return m_original->CheckDeviceType(Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed);
	}

	HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override {
		return m_original->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
	}

	HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) override {
		return m_original->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels);
	}

	HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override {
		return m_original->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);
	}

	HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) override {
		return m_original->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat, TargetFormat);
	}

	HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) override {
		return m_original->GetDeviceCaps(Adapter, DeviceType, pCaps);
	}

	HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT Adapter) override {
		return m_original->GetAdapterMonitor(Adapter);
	}

private:
	IDirect3D9* m_original;
};

void* WINAPI FuncHook(char* FuncTarget, char* FuncHook, int copyBytesSize, unsigned char* backupBytes, std::string descr) {
	Logger::getInstance().log("FuncHook called for function: " + descr);

	if (!FuncTarget || !FuncHook) {
		Logger::getInstance().logError("FuncTarget or FuncHook is null");
		return nullptr;
	}

	// Backup the original bytes
	if (!ReadProcessMemory(GetCurrentProcess(), FuncTarget, backupBytes, copyBytesSize, nullptr)) {
		Logger::getInstance().logError("Failed to read process memory for backup bytes");
		return nullptr;
	}
	Logger::getInstance().log("Read original bytes for function: " + descr);

	// Allocate memory for the trampoline
	void* trampoline = VirtualAlloc(nullptr, copyBytesSize + SIZE_OF_REL_JMP, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!trampoline) {
		Logger::getInstance().logError("Failed to allocate memory for trampoline");
		return nullptr;
	}
	Logger::getInstance().log("Allocated memory for trampoline at: " + std::to_string((DWORD_PTR)trampoline) + " for function: " + descr);

	// Copy the original bytes to the trampoline
	if (memcmp(trampoline, backupBytes, copyBytesSize) != 0) {
		Logger::getInstance().logError("Failed to copy original bytes to trampoline");
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return nullptr;
	}

	// Calculate the relative jump offset
	DWORD relJmpOffset = (DWORD)(FuncTarget - (char*)trampoline - SIZE_OF_REL_JMP);
	Logger::getInstance().log("Relative jump offset: " + std::to_string(relJmpOffset) + " for function: " + descr);
    if (!memcpy((char*)trampoline + copyBytesSize, REL_JMP, 1)) {
					Logger::getInstance().logError("Failed to copy REL_JMP to trampoline");
					VirtualFree(trampoline, 0, MEM_RELEASE);
					return nullptr;
    }
	if (!memcpy((char*)trampoline + copyBytesSize + 1, &relJmpOffset, sizeof(DWORD))) {
		Logger::getInstance().logError("Failed to copy relative jump offset to trampoline");
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return nullptr;
	}

	// Change memory protection to allow writing to the target function
	DWORD oldProtect;
	if (!VirtualProtect(FuncTarget, copyBytesSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		Logger::getInstance().logError("Failed to change memory protection for target function");
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return nullptr;
	}

	// Write the hook to the target function
	DWORD hookRelJmpOffset = (DWORD)(FuncHook - FuncTarget - SIZE_OF_REL_JMP);
	memcpy(FuncTarget, REL_JMP, 1);
	memcpy(FuncTarget + 1, &hookRelJmpOffset, sizeof(DWORD));

	// Restore the original memory protection
	if (!VirtualProtect(FuncTarget, copyBytesSize, oldProtect, &oldProtect)) {
		Logger::getInstance().logError("Failed to restore memory protection for target function");
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return nullptr;
	}

	Logger::getInstance().log("FuncHook succeeded for function: " + descr);

	return trampoline;
}

IDirect3D9* WINAPI Direct3DCreate9Hook(UINT SDKVersion) {
	Logger::getInstance().log("HookedDirect3DCreate9 called");
	//std::lock_guard<std::mutex> lock(hookMutex);
	Logger::getInstance().log("HookedDirect3DCreate9 called with SDKVersion: " + std::to_string(SDKVersion));
	IDirect3D9* pOriginalD3D9 = originalDirect3DCreate9(SDKVersion);
	if (pOriginalD3D9) {
		return new CustomDirect3D9(pOriginalD3D9); // return the custom IDirect3D9 object
	}
	return nullptr;
}

void HookDirect3DCreate9() {
	Logger::getInstance().log("Entering InstallDirect3DCreate9Hook");
	//std::lock_guard<std::mutex> lock(hookMutex);
	if (hOriginalD3D9) {
		Logger::getInstance().log("Loaded d3d9.dll");
		originalDirect3DCreate9 = (Direct3DCreate9_t)GetProcAddress(hOriginalD3D9, "Direct3DCreate9");
		if (originalDirect3DCreate9) {
			Logger::getInstance().log("Got address of Direct3DCreate9\nInstalling hook for Direct3DCreate9 with FuncHook");
			unsigned char backupBytes[SIZE_OF_REL_JMP];
			FuncHook((char*)originalDirect3DCreate9, (char*)Direct3DCreate9Hook, SIZE_OF_REL_JMP, backupBytes, "Direct3DCreate9");
		}
		else {
			Logger::getInstance().logError("Failed to get address of Direct3DCreate9");
		}
	}
	else {
		Logger::getInstance().logError("Failed to load d3d9.dll");
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		//InitializeProxy();
		Logger::getInstance().log("Initializing proxy");
		char systemPath[MAX_PATH];
		GetSystemDirectoryA(systemPath, MAX_PATH);
		strcat_s(systemPath, "\\d3d9.dll");


		Logger::getInstance().log("System path: " + std::string(systemPath));
		hOriginalD3D9 = LoadLibraryA(systemPath); // Load d3d9.dll from System32

		if (!hOriginalD3D9) {
			Logger::getInstance().logError("Failed to load original d3d9.dll from system path");
			return FALSE;
		}
		Logger::getInstance().log("Loaded original d3d9.dll from system path\nInstalling Direct3DCreate9 hook");
		HookDirect3DCreate9();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		//CleanupProxy();
		break;
	}
	return TRUE;
}