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

        // hook the ball set vel
        void* ballSetVelFn = reinterpret_cast<void*>(Utilities::ReadPtr(Peggle::BallSetVel, 0));
        MH_CreateHook(ballSetVelFn, Hooks::HookedSetBallVelocity, NULL);

        // and the ball update
        void* ballUpdateFn = reinterpret_cast<void*>(Utilities::ReadPtr(Peggle::BallUpdate, 0));
        MH_CreateHook(ballUpdateFn, Hooks::HookedBallUpdate, NULL);

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

