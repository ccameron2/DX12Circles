#include "App.h"
#include <memory>

#pragma warning(disable : 4996)


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
    #if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    // Create the app
    if (VISUAL)
    {
        auto app = std::make_unique<App>();
    }
    else
    {
        // Spawn a console
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

        auto circles = new Circles();
        circles->InitCircles();
        while (true)
        {
            circles->UpdateCircles();
            circles->OutputFrame();
        }
        circles->ClearMemory();
    }

    return 0;
}