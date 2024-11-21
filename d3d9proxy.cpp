#include "d3d9proxy.h"
#include "logger.h"
#include "hooks.h"
#include "overrides.h"

D3D9Proxy& D3D9Proxy::getInstance() {
	static D3D9Proxy instance;
	return instance;
}

D3D9Proxy::D3D9Proxy() {
	Initialize();
}

D3D9Proxy::~D3D9Proxy() {
	Cleanup();
}

// Direct3DCreate9 proxied to intercept IDirect3D9 object
IDirect3D9* WINAPI D3D9Proxy::Direct3DCreate9Proxy(UINT SDKVersion) {
	Logger::getInstance().log("Attempting to create device through proxy");
	if (originalDirect3DCreate9) {
		pOriginalDirect3D9 = originalDirect3DCreate9(SDKVersion);
		Logger::getInstance().log("Created IDirect3D9 object");
		if (pOriginalDirect3D9) {
			Logger::getInstance().log("Direct3DCreate9 called with SDKVersion: " + std::to_string(SDKVersion));
			CustomDirect3D9& pCustomDirect3D9 = CustomDirect3D9::getInstance(pOriginalDirect3D9);

			return &pCustomDirect3D9; // return intercepted interface to original caller
		} else {
			Logger::getInstance().logError("Failed to get original IDirect3D9 object");
		}
	} else {
		Logger::getInstance().logError("originalDirect3DCreate9 is nullptr");
	}

	return nullptr;
}

// Startup when DLL is attached
void D3D9Proxy::Initialize() {
	if (hOriginalD3D9) {
		Logger::getInstance().log("D3D9Proxy already initialized");
		//return;
	}
	char systemPath[MAX_PATH];
	GetSystemDirectoryA(systemPath, MAX_PATH);
	strcat_s(systemPath, "\\d3d9.dll");
	Logger::getInstance().log("System path: " + std::string(systemPath));

	hOriginalD3D9 = LoadLibraryA(systemPath); // load d3d9.dll from System32
	if (!hOriginalD3D9) {
		Logger::getInstance().logError("Failed to load original d3d9.dll from system path");
		return;
	}
	Logger::getInstance().log("Loaded original d3d9.dll from system path");

	originalDirect3DCreate9 = reinterpret_cast<Direct3DCreate9_t>(GetProcAddress(hOriginalD3D9, "Direct3DCreate9"));
	if (!originalDirect3DCreate9) {
		Logger::getInstance().logError("Failed to get address of Direct3DCreate9");
	}
}

void D3D9Proxy::Cleanup() {
	if (hOriginalD3D9) {
		FreeLibrary(hOriginalD3D9);
		hOriginalD3D9 = NULL;
	}

	if (pOriginalDirect3D9) {
		pOriginalDirect3D9->Release();
		pOriginalDirect3D9 = nullptr;
	}
}

extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion) {
	Logger::getInstance().log("Direct3DCreate9 called");
	return D3D9Proxy::getInstance().Direct3DCreate9Proxy(SDKVersion);
}