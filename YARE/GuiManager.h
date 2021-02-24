#pragma once
#ifndef _GUI_MANAGER_H_
#define _GUI_MANAGER_H_

#include "pch.h"
#include "Renderer.h"

#define IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
#define IMGUI_IMPL_WIN32_DISABLE_LINKING_XINPUT
#define DIRECTINPUT_VERSION 0x0800

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_impl_win32.h"
#include "External/imgui/imgui_impl_dx12.h"

// Class dedicated to rendering UI which controls rendering settings
class GuiManager
{
public:
	// Constructor
	GuiManager(ID3D12Device* device, Renderer* renderer);

	// Render UI - called on command list after everything else was rendered
	void Render(ID3D12GraphicsCommandList* commandList);

	// Start/stop rendering UI
	void ToogleRendering();

private:
	// Inline imgui render calls
	void RenderLightSettings();
	void SamplingTypeSettings();
	void ShadingModelSettings();
	void AoSettings();

private:
	ComPtr<ID3D12DescriptorHeap> m_imGuiHeap;
	Renderer* m_renderer;
	bool m_isActive = true;
};

#endif // !_GUI_MANAGER_H_
