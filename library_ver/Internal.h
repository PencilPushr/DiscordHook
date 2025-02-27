#pragma once

#include <cstdint>
#include <Windows.h>
#include <fstream>

#include "global_files/Globals.h"

// Hook installation and removal
bool installHookInternal();
bool removeHookInternal();

// Creating a trampoline for calling original functions
template<typename T>
bool createTrampoline(uint64_t targetFnAddr, uint8_t* originalBytes, T* trampolineOut);

// Hook initialization and cleanup
bool initDXGIHook(uint64_t moduleBase);
bool initD3D9Hook(uint64_t moduleBase);
bool initHooks();
bool cleanupHooks();

// Our hook functions
void __stdcall hookDXGIPresent(
    IDXGISwapChain* pSwapChain, 
    UINT SyncInterval, 
    UINT Flags
);

void __stdcall hookD3D9Present(
    IDirect3DDevice9* pDevice, 
    const RECT* pSourceRect,
    const RECT* pDestRect, 
    HWND hDestWindowOverride,
    const RGNDATA* pDirtyRegion
);