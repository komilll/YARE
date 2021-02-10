#include "Main.h"

HRESULT Main::InitializeWindow(HINSTANCE hInstance, std::shared_ptr<Renderer> renderer)
{
    m_isInitialized = false;

    m_renderer = renderer;
    assert(m_renderer && "Renderer is null");

    m_hInstance = hInstance;
    assert(m_hInstance && "hInstance is null");

    ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    EnableDebugLayer();

    RegisterWindowClass(hInstance, m_windowClassName.c_str());

    m_hwnd = CreateWindow(m_windowClassName.c_str(), hInstance, L"RTCP", static_cast<int>(m_renderer->GetWindowSize().x), static_cast<int>(m_renderer->GetWindowSize().y));
    ::GetWindowRect(m_hwnd, &m_windowRect);
    ToggleFullscreen();

    self = this;

    return 0;
}

HRESULT Main::Run(std::shared_ptr<Renderer> renderer)
{
    HRESULT result = S_OK;

    //m_inputManager = std::shared_ptr<InputManager>(new InputManager());
    //m_guiManager = std::shared_ptr<GuiManager>(new GuiManager(m_renderer->m_device.Get(), renderer.get()));

    m_isInitialized = true;
    ::ShowWindow(m_hwnd, SW_SHOW);

    // Setup Dear ImGui context
    //IMGUI_CHECKVERSION();
    //ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    //ImGui_ImplWin32_Init(m_hwnd);

    bool bGotMsg;
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        bGotMsg = ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (bGotMsg)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        if (msg.message == WM_PAINT)
        {
            ////Change camera position
            //{
            //    const float xStrengthPosition = m_renderer->m_cameraSpeed, yStrengthPosition = xStrengthPosition, zStrengthPosition = xStrengthPosition;
            //    const float x = xStrengthPosition * (m_inputManager->IsKeyDown(VK_A) ? -1.0f : (m_inputManager->IsKeyDown(VK_D) ? 1.0f : 0.0f));
            //    const float y = yStrengthPosition * (m_inputManager->IsKeyDown(VK_E) ? 1.0f : (m_inputManager->IsKeyDown(VK_Q) ? -1.0f : 0.0f));
            //    const float z = zStrengthPosition * (m_inputManager->IsKeyDown(VK_W) ? 1.0f : (m_inputManager->IsKeyDown(VK_S) ? -1.0f : 0.0f));
            //    renderer->AddCameraPosition(x, y, z);
            //}
            ////Change camera rotation
            //{
            //    constexpr float xStrengthRotation = 1.f, yStrengthRotation = xStrengthRotation;
            //    const float x = xStrengthRotation * (m_inputManager->IsKeyDown(VK_LEFT) ? -1.0f : (m_inputManager->IsKeyDown(VK_RIGHT) ? 1.0f : 0.0f));
            //    const float y = yStrengthRotation * (m_inputManager->IsKeyDown(VK_UP) ? -1.0f : (m_inputManager->IsKeyDown(VK_DOWN) ? 1.0f : 0.0f));
            //    renderer->AddCameraRotation(y, x, 0.0f);
            //}
            //// Change rendering mode
            //{
            //    if (m_inputManager->IsKeyDown(VK_R))
            //    {
            //        m_renderer->ToggleRaytracing();
            //        m_inputManager->KeyUp(VK_R);
            //    }
            //    if (m_inputManager->IsKeyDown(VK_G))
            //    {
            //        m_guiManager->ToogleRendering();
            //        m_inputManager->KeyUp(VK_G);
            //    }
            //}
        }
    }

    renderer->OnDestroy();
    return result;
}

// Forward declare message handler from imgui_impl_win32.cpp
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT Main::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //if (ImGui::GetCurrentContext() == NULL)
    //{
    //    ImGui::CreateContext();
    //}

    //if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
    //    return true;

    if (m_isInitialized)
    {
        bool alt = false;
        int width = 0;
        int height = 0;
        RECT clientRect{};

        switch (message)
        {
        case WM_PAINT:
            m_renderer->OnUpdate();

            //m_renderer->PopulateCommandList();
            self->RenderGUI();
            //m_renderer->CloseCommandList();
            m_renderer->OnRender();
            break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            switch (wParam)
            {
            case VK_ESCAPE:
                ::PostQuitMessage(0);
                break;

            case VK_RETURN:
                if (alt)
                {
            case VK_F11:
                self->ToggleFullscreen();
                break;
                }
            default:
                //m_inputManager->KeyDown(static_cast<unsigned int>(wParam));
                break;
            }
            return 0;

        case WM_KEYUP:
            //m_inputManager->KeyUp(static_cast<unsigned int>(wParam));
            return 0;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;

        case WM_CLOSE:
            if (GetMenu(hwnd)) {
                DestroyMenu(GetMenu(hwnd));
            }
            DestroyWindow(hwnd);
            UnregisterClass(m_windowClassName.c_str(), m_hInstance);
            return 0;

        default:
            return ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
    else
    {
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}

void Main::RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
    WNDCLASSEXW windowClass{};

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Main::WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInst;
    windowClass.hIcon = ::LoadIcon(hInst, NULL);
    windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = windowClassName;
    windowClass.hIconSm = ::LoadIcon(hInst, NULL);

    static ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);
}

HWND Main::CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, int width, int height)
{
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    HWND hwnd = ::CreateWindowExW(
        NULL,
        windowClassName,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        windowWidth,
        windowHeight,
        NULL,
        NULL,
        hInst,
        nullptr
    );

    assert(hwnd && "Failed to create window");
    return hwnd;
}

void Main::EnableDebugLayer()
{
#if defined(_DEBUG_YARE)
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

void Main::ToggleFullscreen()
{
//    m_fullscreen = !m_fullscreen;
//    if (m_fullscreen)
//    {
//        ::GetWindowRect(m_hwnd, &m_windowRect);
//
//        UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
//        ::SetWindowLongW(m_hwnd, GWL_STYLE, windowStyle);
//
//        HMONITOR hMonitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
//        MONITORINFOEX monitorInfo{};
//        monitorInfo.cbSize = sizeof(MONITORINFOEX);
//        ::GetMonitorInfo(hMonitor, &monitorInfo);
//
//        ::SetWindowPos(m_hwnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
//        ::ShowWindow(m_hwnd, SW_MAXIMIZE);
//    }
//    else
//    {
//        ::SetWindowLong(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
//        ::SetWindowPos(m_hwnd, HWND_NOTOPMOST, m_windowRect.left, m_windowRect.top, m_windowRect.right - m_windowRect.left, m_windowRect.bottom - m_windowRect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
//        ::ShowWindow(m_hwnd, SW_NORMAL);
//    }
}

void Main::RenderGUI()
{
    //// Render ImGui
    //auto commandList = m_renderer->DO_RAYTRACING ? m_renderer->m_commandList : m_renderer->m_commandListSkybox;

    //CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_renderer->m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_renderer->m_frameIndex, m_renderer->m_rtvDescriptorSize);
    //CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_renderer->m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    //commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    //commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_backBuffers[m_renderer->m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    //m_guiManager->Render(commandList.Get());
    //commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_backBuffers[m_renderer->m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}
