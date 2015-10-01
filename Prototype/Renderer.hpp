// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Common.hpp"

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Renderer module
struct Renderer : public ModuleIf
{
    enum Group
    {
        DEFAULT             = 0x1,
        DEFAULT_TRANSPARENT = 0x2,
        DEFAULT_POST        = 0x4,
        DEFAULT_UI          = 0x8
    };

    struct Program
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            u32 programAsset = 0;
        };
        struct PrivateInfo;
    };

    struct Mesh
    {
        enum Flag
        {
            HIDDEN      = 0x1,
            SCALED      = 0x2,
            DIFFUSE_MUL = 0x4,
            AMBIENT_ADD = 0x8
        };
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            glm::fvec3 translation;
            glm::fquat rotation;
            float uniformScale = 0.0f;
            glm::fvec4 diffuseMul;
            glm::fvec4 ambientAdd;
            u32 modelAsset = 0;
            u32 flags = 0;
            u32 groups = 0;
        };
        struct PrivateInfo;
    };

    struct Camera
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            glm::fvec3 position;
            glm::fvec3 target;
        };
    };

    struct Transform
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            u64 parentTransformHandle = 0;
            glm::fvec3 translation;
            glm::fquat rotation;
            glm::fmat4 localToWorld;
            glm::fmat4 worldToLocal;
        };
    };

    u64 activeCameraHandle = 0;
    bool debugNormals = false;

    Renderer();
    virtual ~Renderer();

public: // Implementation of module interface
    virtual void registerTypesAndStates(StateDb &sdb);
    virtual bool initialize(StateDb &sdb, Assets &assets);
    virtual void shutdown(StateDb &sdb);
    virtual void update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS);

private:
    struct PrivateFuncs;
    struct PrivateState;
    struct PrivateHelpers;
    struct PrivateMesh;

    std::shared_ptr< PrivateFuncs > funcs;
    std::shared_ptr< PrivateState > state;
    std::shared_ptr< PrivateHelpers > helpers;

    void renderPass(StateDb &sdb, u32 renderMask, const Program::PrivateInfo *program,
            const glm::fmat4 &projection, const glm::fmat4 &worldToView,
            const glm::fvec4 &renderParams);

    bool initializeGl();

private:
    COMMON_DISABLE_COPY(Renderer)
};

#endif
