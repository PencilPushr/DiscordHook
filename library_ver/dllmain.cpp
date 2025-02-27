#include "Internal.h"

#include <Windows.h>
#include <stdio.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        // Initialize hooks when the DLL is loaded
        DisableThreadLibraryCalls(hinstDLL);

        // Create a console for debugging output
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);

        printf("DLL injected into process\n");

        // Initialize the hooks
        if (!initHooks()) 
        {
            printf("Failed to initialize hooks\n");
            return FALSE;
        }

        printf("Hooks initialized successfully\n");
        break;

    case DLL_PROCESS_DETACH:
        // Clean up hooks when the DLL is unloaded and clean up console
        cleanupHooks();
        FreeConsole();
        break;
    }

    return TRUE;
}