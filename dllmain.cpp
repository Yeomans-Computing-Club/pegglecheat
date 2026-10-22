// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "minhook.h"
#include "peggle.h"
#include "bass.h"

// hooks
void* ballUpdateFn = reinterpret_cast<void*>(Utilities::ReadPtr(Peggle::BallUpdate, 0));

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        MH_Initialize();
        MH_CreateHook(ballUpdateFn, &Hooks::HookedBallUpdate, reinterpret_cast<void**>(&Hooks::OriginalBallUpdate));
        //            ^ the target  ^ the hook                ^ the "trampoline" (lets us call the original function)
        MH_EnableHook(MH_ALL_HOOKS);

        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}