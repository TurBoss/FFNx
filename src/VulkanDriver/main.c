#include <windows.h>

__declspec(dllexport) void ModMain(const void* unused)
{
    MessageBoxA(NULL, "Hello", "Hello from the Vulkan Driver!", 0);
}
