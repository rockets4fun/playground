// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 22.12.2015
// -------------------------------------------------------------------------------------------------

#include "ImGuiEval.hpp"

#include <imgui.h>

#include "Assets.hpp"

// -------------------------------------------------------------------------------------------------
ImGuiEval::ImGuiEval()
{
}

// -------------------------------------------------------------------------------------------------
ImGuiEval::~ImGuiEval()
{
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::registerTypesAndStates(StateDb &sdb)
{
}

// -------------------------------------------------------------------------------------------------
bool ImGuiEval::initialize(StateDb &sdb, Assets &assets)
{
    ImGuiIO &imGui = ImGui::GetIO();

    m_imGuiModelAsset = assets.asset(
        "Procedural/Models/ImGui", Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);

    m_imGuiTextureAsset = assets.asset(
        "Procedural/Textures/ImGui", Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);

    unsigned char *imGuiPixels = nullptr;
    Assets::Texture *texture = assets.refTexture(m_imGuiTextureAsset);
    imGui.Fonts->GetTexDataAsRGBA32(&imGuiPixels, &texture->width, &texture->height);
    texture->pixels.resize(texture->width * texture->height);
    memcpy(&texture->pixels[0], imGuiPixels,
        texture->pixels.size() * sizeof(Assets::Texture::Pixel));

    return true;
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::shutdown(StateDb &sdb)
{
    ImGui::Shutdown();
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
    // (1) Define the user interface
    {
        
    }

    // (2) Render UI into command buffers
    {
    }

    // (3) Retrieve render command buffers and transform/prepare for renderer
    {
    }
}
