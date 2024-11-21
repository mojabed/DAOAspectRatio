#pragma once

#include <windows.h>  
#include <dbghelp.h>
#include <d3d9.h>
#include <mutex>

using Direct3DCreate9_t = IDirect3D9 * (WINAPI*)(UINT);
using SetViewport_t = HRESULT(__stdcall*)(IDirect3DDevice9*, const D3DVIEWPORT9*);

class D3D9Proxy {
public:
	D3D9Proxy();
	~D3D9Proxy();
	static D3D9Proxy& getInstance();

	void Initialize();
	void Cleanup();

	IDirect3D9* WINAPI Direct3DCreate9Proxy(UINT SDKVersion);

private:
	D3D9Proxy(const D3D9Proxy&) = delete;
	D3D9Proxy& operator=(const D3D9Proxy&) = delete;

	HMODULE hOriginalD3D9 = NULL;
	IDirect3D9* pOriginalDirect3D9 = nullptr;
	
	Direct3DCreate9_t originalDirect3DCreate9 = nullptr;
	SetViewport_t originalSetViewport = nullptr;

	std::mutex m_mutex;
};

extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion);