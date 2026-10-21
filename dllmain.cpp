// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "minhook.h"
#include "peggle.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        MH_Initialize();
        MH_CreateHook((void*)Peggle::GetTunableInt, &Hooks::HookedGetTunableInt, (void**)&Hooks::OriginalGetTunableInt);
        //            ^ the target                  ^ the hook                   ^ the "trampoline" (lets us call the original function)
        MH_CreateHook((void*)Peggle::GetTunableFloat, &Hooks::HookedGetTunableFloat, (void**)&Hooks::OriginalGetTunableFloat);
        MH_CreateHook((void*)Peggle::FireGun, &Hooks::HookedFireGun, (void**)&Hooks::OriginalFireGun);
        MH_EnableHook(MH_ALL_HOOKS);

        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

