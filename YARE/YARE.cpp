// YARE.cpp - aplication entry point

#include "pch.h"
#include "Main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
    HRESULT result = S_OK;

    // Initialize window
    std::shared_ptr<Renderer> renderer = std::shared_ptr<Renderer>(new Renderer());
    std::shared_ptr<Main> main = std::shared_ptr<Main>(new Main());
    result = main->InitializeWindow(hInstance, renderer);

    if (SUCCEEDED(result))
    {
        // Prepare renderer resources
        renderer->OnInit(main->GetWindowHandle());

        // Run main loop
        main->Run(renderer);
    }

    return 0;
}