// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 22.12.2015
// -------------------------------------------------------------------------------------------------

#include "ImGuiEval.hpp"

#include <SDL.h>
#include <imgui.h>

#include "Str.hpp"

#include "Assets.hpp"
#include "StateDb.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

// -------------------------------------------------------------------------------------------------
ImGuiEval::ImGuiEval()
{
}

// -------------------------------------------------------------------------------------------------
ImGuiEval::~ImGuiEval()
{
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::registerModule(ModuleIf &module)
{
    m_modules.push_back(&module);
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::registerTypesAndStates(StateDb &sdb)
{
}

// -------------------------------------------------------------------------------------------------
bool ImGuiEval::initialize(StateDb &sdb, Assets &assets)
{
    ImGui::CreateContext();

    ImGuiIO &imGui = ImGui::GetIO();

    imGui.DisplaySize.x = 800.0f;
    imGui.DisplaySize.y = 450.0f;

    {
        u32 fontTextureAsset = assets.asset(
            "Procedural/Textures/ImGuiFont", Assets::Flag::PROCEDURAL | Assets::Flag::DYNAMIC);
        unsigned char *imGuiPixels = nullptr;
        Assets::Texture *texture = assets.refTexture(fontTextureAsset);
        imGui.Fonts->GetTexDataAsRGBA32(&imGuiPixels, &texture->width, &texture->height);
        texture->pixels.resize(texture->width * texture->height);
        memcpy(&texture->pixels[0], imGuiPixels,
            texture->pixels.size() * sizeof(Assets::Texture::Pixel));
        assets.touch(fontTextureAsset);

        auto fontTexture = sdb.create< Renderer::Texture::Info >(m_fontTextureHandle);
        fontTexture->textureAsset = fontTextureAsset;
        imGui.Fonts->TexID = &m_fontTextureHandle;
    }

    {
        u32 programAsset = assets.asset("Assets/Programs/ImGui.program");
        assets.refProgram(programAsset);
        auto program = sdb.create< Renderer::Program::Info >(m_programHandle);
        program->programAsset = programAsset;
    }

    {
        auto pass = sdb.create< Renderer::Pass::Info >(m_passHandle);
        pass->programHandle = m_programHandle;
        pass->groups = Renderer::Group::DEFAULT_IM_GUI;
        pass->flags = Renderer::Pass::BLEND;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::shutdown(StateDb &sdb)
{
    for (auto &meshHandle : m_meshHandles) sdb.destroy(meshHandle);
    sdb.destroy(m_fontTextureHandle);

    ImGui::DestroyContext();
}

// -------------------------------------------------------------------------------------------------
void ImGuiEval::update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
    ImGuiIO &imGui = ImGui::GetIO();
    imGui.DeltaTime = float(deltaTimeInS);

    // (1) Immediate mode style UI definition
    {
        ImGui::NewFrame();
        ImGui::ShowMetricsWindow(&m_metricsVisible);
        if (m_profilerVisible)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
            ImGui::Begin("Profiler", &m_profilerVisible);

            Profiler *profiling = Profiler::instance();
            Profiler::Thread *mainThread = profiling->mainThreadPrevFrame();

            if (mainThread)
            {
                int maxCallDepth = -1;
                int prevCallDepth = -1;
                for (Profiler::SectionSample &sample : mainThread->samples)
                {
                    float enterMs   = float(profiling->ticksToMs(sample.ticksEnter));
                    float exitMs    = float(profiling->ticksToMs(sample.ticksExit));
                    if (maxCallDepth >= 0)
                    {
                        if (sample.callDepth > maxCallDepth)
                        {
                            continue;
                        }
                        maxCallDepth = -1;
                    }
                    while (prevCallDepth >= sample.callDepth)
                    {
                        ImGui::TreePop();
                        --prevCallDepth;
                    }
                    int flags = 0;
                    // TODO: Set 'ImGuiTreeNodeFlags_Leaf' if leaf node...
                    if (!ImGui::TreeNodeEx(sample.section->name.c_str(), flags))
                    {
                        maxCallDepth = sample.callDepth;
                    }
                    ImGui::SameLine(200);
                    ImGui::Text("%6.0f us", Profiler::instance()->ticksToMs(
                        sample.ticksExit - sample.ticksEnter) * 1000.0f);
                    if (maxCallDepth >= 0)
                    {
                        continue;
                    }
                    prevCallDepth = sample.callDepth;
                }
                while (prevCallDepth-- >= 0)
                {
                    ImGui::TreePop();
                }
            }

            ImGui::End();
        }
        for (auto &module : m_modules) module->imGuiUpdate(sdb, assets);
        ImGui::Render();
    }

    // (2) Retrieve render command buffers and transform/prepare for renderer
    {
        ImDrawData *drawData = ImGui::GetDrawData();
        // Get rid of meshes/reset models no longer used...
        while (m_meshHandles.size() > drawData->CmdListsCount)
        {
            u64 meshHandle = m_meshHandles.back();
            auto mesh = sdb.state< Renderer::Mesh::Info >(meshHandle);
            assets.refModel(mesh->modelAsset)->clear();
            sdb.destroy(meshHandle);
            m_meshHandles.pop_back();
        }
        // Transform UI meshes into models and mesh references for renderer
        for (int cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
        {
            ImDrawList *cmdList = drawData->CmdLists[cmdListIdx];
            // We need one model/mesh pair for each command list
            // ==> At the time of writing ImGui uses one command list per UI window
            if (cmdListIdx >= int(m_meshHandles.size()))
            {
                m_meshHandles.resize(cmdListIdx + 1);
                auto mesh = sdb.create< Renderer::Mesh::Info >(m_meshHandles[cmdListIdx]);
                std::string assetName = Str::build("Procedural/Models/ImGui_%03d", cmdListIdx);
                mesh->modelAsset = assets.asset(assetName, Assets::PROCEDURAL | Assets::DYNAMIC);
            }
            auto mesh = sdb.state< Renderer::Mesh::Info >(m_meshHandles[cmdListIdx]);
            Assets::Model *model = assets.refModel(mesh->modelAsset);

            if (model->attrs.empty())
            {
                model->attrs =
                {
                    Assets::MAttr("Position", nullptr, Assets::MAttr::FLOAT, 2,  0, false),
                    Assets::MAttr("UV",       nullptr, Assets::MAttr::FLOAT, 2,  8, false),
                    Assets::MAttr("Color",    nullptr, Assets::MAttr::U8,    4, 16, true),
                };
                model->indicesAttr = Assets::MAttr("", nullptr, Assets::MAttr::U16, 0);
                COMMON_ASSERT(sizeof(ImDrawIdx) == 2);
                mesh->groups = Renderer::Group::DEFAULT_IM_GUI;
                mesh->flags |= Renderer::Mesh::DRAW_PARTS;
            }

            void *dataPointer = &cmdList->VtxBuffer[0].pos.x;
            for (auto &attr : model->attrs) attr.data = dataPointer;

            model->indicesAttr.data = &cmdList->IdxBuffer[0];
            model->indicesAttr.count = cmdList->IdxBuffer.size();

            u64 prevElemOffset = 0;
            int cmdCount = cmdList->CmdBuffer.size();
            if (int(model->parts.size()) != cmdCount)
            {
                model->parts.resize(cmdCount);
            }
            for (int cmdIdx = 0; cmdIdx < cmdCount; ++cmdIdx)
            {
                ImDrawCmd &cmd = cmdList->CmdBuffer[cmdIdx];
                Assets::Model::Part &part = model->parts[cmdIdx];
                // FIXME(martinmo): Pass the scissor rectangle stored in 'cmd'...
                part.materialHint = *(u64 *)cmd.TextureId;
                part.offset = prevElemOffset;
                part.count = cmd.ElemCount;
                prevElemOffset += part.count;
            }

            model->vertexCount = cmdList->VtxBuffer.size();
        }
    }

    // (3) Read input state via SDL and pass on to ImGui
    {
        int mouseX = -1, mouseY = -1;
        Uint32 mouseMask = SDL_GetMouseState(&mouseX, &mouseY);
        imGui.MousePos = ImVec2((float)mouseX, (float)mouseY);
        imGui.MouseDown[0] = (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        imGui.MouseDown[1] = (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        imGui.MouseDown[2] = (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    }
}
