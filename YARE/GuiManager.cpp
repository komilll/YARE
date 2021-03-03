#include "GuiManager.h"

void GuiManager::Render(ID3D12GraphicsCommandList* commandList)
{
    // Do not render UI
    if (!m_isActive)
    {
        return;
    }
    ID3D12DescriptorHeap* ppHeaps[] = { m_imGuiHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // Prepare ImGui settings
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Camera:");

        // Camera position/rotation section
        char camPosText[100];
        char camRotText[100];
        sprintf_s(camPosText, "Current cam pos: (%.1f, %.1f, %.1f)", m_renderer->m_cameraPosition.x, m_renderer->m_cameraPosition.y, m_renderer->m_cameraPosition.z);
        sprintf_s(camRotText, "Current cam rot: (%.1f, %.1f, %.1f)", m_renderer->m_cameraRotation.x, m_renderer->m_cameraRotation.y, m_renderer->m_cameraRotation.z);
        ImGui::Text(camPosText);
        ImGui::Text(camRotText);
        ImGui::SliderFloat("Camera speed", &m_renderer->m_cameraSpeed, 0.01f, 25.0f, "%.2f");

        ImGui::End();
    }

    // Finish imgui - render UI
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void GuiManager::ToogleRendering()
{
    m_isActive = !m_isActive;
}

GuiManager::GuiManager(ID3D12Device* device, Renderer* renderer)
{
    m_renderer = renderer;
    assert(renderer != NULL && "Renderer is NULL");
    assert(device != NULL && "Device is NULL");

    // Initialize ImGui SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV , 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imGuiHeap)));
    ImGui_ImplDX12_Init(device, 2, DXGI_FORMAT_R8G8B8A8_UNORM, m_imGuiHeap.Get(), m_imGuiHeap->GetCPUDescriptorHandleForHeapStart(), m_imGuiHeap->GetGPUDescriptorHandleForHeapStart());
}
