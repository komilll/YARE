#pragma once

#include "pch.h"
//#include "DeviceManager.h"
#include "Renderer.h"
#include "InputManager.h"
#include "GuiManager.h"

class Main
{
public:
	HRESULT InitializeWindow(HINSTANCE hInstance, std::shared_ptr<Renderer> renderer);
	HRESULT Run(std::shared_ptr<Renderer> renderer);
	HWND GetWindowHandle() const { return m_hwnd; };

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);
	HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, int width, int height);
	void EnableDebugLayer();

	void ToggleFullscreen();

	void RenderGUI();

private:
	HWND m_hwnd = NULL;
	RECT m_windowRect;

	bool m_fullscreen = false;
};

static std::shared_ptr<Renderer> m_renderer;
static std::shared_ptr<InputManager> m_inputManager;
static std::shared_ptr<GuiManager> m_guiManager;
static bool m_isInitialized;
static Main* self;
static std::wstring m_windowClassName = L"YARE";
static HINSTANCE m_hInstance;