#include "Internal.h"


#include <iostream>
#include <time.h>

// File for logging
std::ofstream logFile;

// Original function pointers (our trampolines will call these)
Present_t originalPresent = nullptr;
D3D9Present_t originalD3D9Present = nullptr;

// Trampoline buffers - executable memory where we'll store original code + jump
Present_t presentTrampoline = nullptr;
uint8_t* d3d9PresentTrampoline = nullptr;

static bool wasHooked = false;

// Our hook functions that will replace the original functions
void __stdcall hookDXGIPresent(
    IDXGISwapChain* pSwapChain, 
    UINT SyncInterval, 
    UINT Flags
) 
{
    if (!wasHooked)
    {
        // Log the call
        logFile << "[" << time(nullptr) << "] DXGI Present called: SyncInterval=" << SyncInterval
            << ", Flags=" << Flags << std::endl;
        wasHooked = true;
    }

    // Call the original function through our trampoline
    presentTrampoline(pSwapChain, SyncInterval, Flags);
}

void __stdcall hookD3D9Present(
    IDirect3DDevice9* pDevice, 
    const RECT* pSourceRect,
    const RECT* pDestRect, 
    HWND hDestWindowOverride,
    const RGNDATA* pDirtyRegion
) 
{
    // Log the call
    logFile << "[" << time(nullptr) << "] D3D9 Present called" << std::endl;

    // Call the original function through our trampoline
    originalD3D9Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

// Create a trampoline that allows calling the original function
template<typename T>
bool createTrampoline(
    uint64_t targetFnAddr, 
    uint8_t* originalBytes, 
    T* trampolineOut
) 
{
    // Allocate executable memory for the trampoline
    uint8_t* trampoline = (uint8_t*)VirtualAlloc(nullptr, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) 
    {
        fprintf(stderr, "Failed to allocate memory for trampoline: %lu\n", GetLastError());
        return false;
    }

    // Copy the original bytes (instructions) to the trampoline
    memcpy(trampoline, originalBytes, 12);

    // After the original instructions, add a jump back to the original function (after our hook)
    // Create a jump instruction:
    // 0x48 0xB8 [8-byte address] 0xFF 0xE0
    // mov rax, <address>
    // jmp rax
    uint64_t returnAddr = targetFnAddr + 12;  // Address after our hook

    trampoline[12] = 0x48;
    trampoline[13] = 0xB8;
    memcpy(&trampoline[14], &returnAddr, sizeof(returnAddr));
    trampoline[22] = 0xFF;
    trampoline[23] = 0xE0;

    // Return the trampoline
    *trampolineOut = (T)trampoline;
    return true;
}

// Initialize hook for DXGI Present
bool initDXGIHook(uint64_t moduleBase) 
{
    // DXGI hook calculation
    uint64_t presentAddr = moduleBase + DISCORD_DXGI_HOOK_ADDR;

    // Set up hook data
    g_hookData.processHandle = GetCurrentProcess();
    g_hookData.targetFnAddr = presentAddr;
    g_hookData.hookFnAddr = (uint64_t)hookDXGIPresent;

    // Install the hook
    if (!installHookInternal()) 
    {
        fprintf(stderr, "Failed to install DXGI Present hook\n");
        return false;
    }

    // Create the trampoline
    if (!createTrampoline(presentAddr, g_hookData.originalBytes, &presentTrampoline)) 
    {
        fprintf(stderr, "Failed to create DXGI Present trampoline\n");
        return false;
    }

    // Set the original function pointer to the trampoline
    originalPresent = (Present_t)presentTrampoline;

    return true;
}

// Initialize hook for D3D9 Present
bool initD3D9Hook(uint64_t moduleBase) 
{
    // D3D9 hook calculation
    uint64_t d3d9PresentAddr = moduleBase + DISCORD_D3D9_HOOK_ADDR;

    // Save the current hook data
    struct hookData tempHookData = g_hookData;

    // Set up hook data for D3D9
    g_hookData.processHandle = GetCurrentProcess();
    g_hookData.targetFnAddr = d3d9PresentAddr;
    g_hookData.hookFnAddr = (uint64_t)hookD3D9Present;

    // Install the hook
    if (!installHookInternal()) {
        fprintf(stderr, "Failed to install D3D9 Present hook\n");
        // Restore previous hook data
        g_hookData = tempHookData;
        return false;
    }

    // Create the trampoline
    if (!createTrampoline(d3d9PresentAddr, g_hookData.originalBytes, &d3d9PresentTrampoline)) 
    {
        fprintf(stderr, "Failed to create D3D9 Present trampoline\n");
        // Restore the original bytes
        removeHookInternal();
        // Restore previous hook data
        g_hookData = tempHookData;
        return false;
    }

    // Set the original function pointer to the trampoline
    originalD3D9Present = (D3D9Present_t)d3d9PresentTrampoline;

    // Restore previous hook data since we saved the D3D9 data already
    g_hookData = tempHookData;

    return true;
}

// Initialize all hooks
bool initHooks() 
{
    // Open the log file
    logFile.open("discord_hook_log.txt", std::ios::app);
    if (!logFile.is_open()) 
    {
        fprintf(stderr, "Failed to open log file\n");
        return false;
    }

    logFile << "========================================\n";
    logFile << "Hook initialized at: " << time(nullptr) << std::endl;

    // Get the base address of Discord module
    HMODULE discordModule = GetModuleHandleA("DiscordHook64.dll");  // Assuming we're injected into Discord
    if (!discordModule) 
    {
        fprintf(stderr, "Failed to get module handle: %lu\n", GetLastError());
        return false;
    }

    uint64_t moduleBase = (uint64_t)discordModule;

    // Initialize hooks
    bool dxgiSuccess = initDXGIHook(moduleBase);
    //bool d3d9Success = initD3D9Hook(moduleBase);

    //return dxgiSuccess || d3d9Success;  // Return true if at least one hook succeeded
    return dxgiSuccess;
}

// Cleanup hooks
bool cleanupHooks() 
{
    // Remove the DXGI hook
    if (originalPresent) {
        // Set up hook data
        g_hookData.targetFnAddr = (uint64_t)originalPresent - 12;  // Adjust to get original address

        if (!removeHookInternal()) {
            fprintf(stderr, "Failed to remove DXGI Present hook\n");
        }

        // Free the trampoline memory
        if (presentTrampoline) {
            VirtualFree(presentTrampoline, 0, MEM_RELEASE);
            presentTrampoline = nullptr;
        }

        originalPresent = nullptr;
    }

    // Remove the D3D9 hook
    if (originalD3D9Present) {
        // Save current hook data
        struct hookData tempHookData = g_hookData;

        // Set up hook data
        g_hookData.targetFnAddr = (uint64_t)originalD3D9Present - 12;  // Adjust to get original address

        if (!removeHookInternal()) {
            fprintf(stderr, "Failed to remove D3D9 Present hook\n");
        }

        // Free the trampoline memory
        if (d3d9PresentTrampoline) {
            VirtualFree(d3d9PresentTrampoline, 0, MEM_RELEASE);
            d3d9PresentTrampoline = nullptr;
        }

        originalD3D9Present = nullptr;

        // Restore previous hook data
        g_hookData = tempHookData;
    }

    // Close the log file
    if (logFile.is_open()) {
        logFile << "Hook cleaned up at: " << time(nullptr) << std::endl;
        logFile << "========================================\n";
        logFile.close();
    }

    return true;
}

// Original Internal.cpp functions
inline bool installHookInternal()
{
    // Create an absolute jump instruction:
    // 0x48 B8 <8-byte address>  FF E0
    // This translates to:
    //   mov rax, <hook address>
    //   jmp rax
    uint8_t absJumpInstructions[12] = {
        0x48, 0xB8,                     // mov rax, imm64
        0, 0, 0, 0, 0, 0, 0, 0,         // placeholder for hook address
        0xFF, 0xE0                      // jmp rax
    };

    // Set hook function address (see placeholder for hook address right above)
    memcpy(&absJumpInstructions[2], &g_hookData.hookFnAddr, sizeof(g_hookData.hookFnAddr));

    // Save the hook bytes
    memcpy(g_hookData.hookBytes, absJumpInstructions, sizeof(g_hookData.hookBytes));

    // Change the memory protection of the target function to allow writing.
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)g_hookData.targetFnAddr, sizeof(absJumpInstructions), PAGE_EXECUTE_READWRITE, &oldProtect)) 
    {
        fprintf(stderr, "VirtualProtect failed: %lu\n", GetLastError());
        return false;
    }

    // Save the original bytes so we can restore them later.
    memcpy(g_hookData.originalBytes, (void*)g_hookData.targetFnAddr, sizeof(absJumpInstructions));

    // Overwrite the beginning of the target function with our jump instruction.
    memcpy((void*)g_hookData.targetFnAddr, absJumpInstructions, sizeof(absJumpInstructions));

    // Restore the original memory protection.
    VirtualProtect((LPVOID)g_hookData.targetFnAddr, sizeof(absJumpInstructions), oldProtect, &oldProtect);

    fprintf(stdout, "Hook installed at address: 0x%llx \n", g_hookData.targetFnAddr);
    return true;
}

inline bool removeHookInternal()
{
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)g_hookData.targetFnAddr, sizeof(g_hookData.originalBytes), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        fprintf(stderr, "VirtualProtect failed: %lu \n", GetLastError());
        return false;
    }

    // Write back the original bytes.
    memcpy((void*)g_hookData.targetFnAddr, g_hookData.originalBytes, sizeof(g_hookData.originalBytes));

    // Restore memory protection.
    VirtualProtect((LPVOID)g_hookData.targetFnAddr, sizeof(g_hookData.originalBytes), oldProtect, &oldProtect);

    fprintf(stdout, "Hook removed from address: 0x%llx \n", g_hookData.targetFnAddr);
    return true;
}