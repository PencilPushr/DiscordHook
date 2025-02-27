#pragma once

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DISCORD_D3D9_HOOK_ADDR 0x62B0
#define DISCORD_DXGI_HOOK_ADDR 0x16430

typedef HRESULT(__stdcall* Present_t)(
    struct IDXGISwapChain* pSwapChain, 
    UINT SyncInterval, 
    UINT Flags
);

typedef HRESULT(__stdcall* D3D9Present_t)(
    struct IDirect3DDevice9* pDevice,
    const RECT* pSourceRect,
    const RECT* pDestRect,
    HWND hDestWindowOverride,
    const RGNDATA* pDirtyRegion
);

struct hookData {
    // Process stuff
    HANDLE processHandle;

    // Original function 
    uint8_t originalBytes[12];
    uint64_t targetFnAddr;

    // Hooking function
    uint8_t hookBytes[12];
    uint64_t hookFnAddr;

};

inline hookData g_hookData;