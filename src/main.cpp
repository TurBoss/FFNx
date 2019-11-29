#include <windows.h>

__declspec(dllexport) void* new_dll_graphics_driver(void* game_object) {
    void* ret;

    return ret;
}

BOOL APIENTRY DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}
