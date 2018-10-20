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
        DEFAULT             = 0x01,
        DEFAULT_TRANSPARENT = 0x02,
        DEFAULT_POST        = 0x04,
        DEFAULT_GUI         = 0x08,
        DEFAULT_IM_GUI      = 0x10
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
        static u64 TYPE;
        enum Flag
        {
            HIDDEN         = 0x01,
            SCALED         = 0x02,
            DIFFUSE_MUL    = 0x04,
            AMBIENT_ADD    = 0x08,
            DRAW_PARTS     = 0x10
        };
        struct Info
        {
            static u64 STATE;
            glm::fvec3 translation;
            glm::fquat rotation;
            glm::fvec3 scale;
            glm::fvec4 diffuseMul;
            glm::fvec4 ambientAdd;
            u32 modelAsset = 0;
            u32 flags = 0;
            u32 groups = 0;
        };
        struct PrivateInfo;
    };

    struct Texture
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            u32 textureAsset = 0;
        };
        struct PrivateInfo;
    };

    //struct Material
    //{
    //    static u64 TYPE;
    //    struct Info
    //    {
    //        static u64 STATE;
    //        u32 programAsset;
    //        u32 textureAssets[8];
    //    };
    //    struct PrivateInfo;
    //};

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

    struct Pass
    {
        static u64 TYPE;
        enum Flag
        {
            CLEAR_COLOR     = 0x00000001,
            CLEAR_DEPTH     = 0x00000002,
            CLEAR_STENCIL   = 0x00000004,
            // ...
            BLEND           = 0x00000100,
            CULL_FACE       = 0x00000200,
            DEPTH_TEST      = 0x00000400,
            SCISSOR_TEST    = 0x00000800,
            // ...
            NO_DEPTH_WRITES = 0x00010000,
            NO_COLOR_WRITES = 0x00020000
        };
        enum BlendFunc
        {
            ONE                 = 0,
            SRC_ALPHA           = 1,
            ONE_MINUS_SRC_ALPHA = 2
        };
        struct Info
        {
            static u64 STATE;
            u32 flags = 0;
            glm::fvec4 clearColor;
            float clearDepth = 0.0f;
            u32 blendFuncSrc = 0;
            u32 blendFuncDst = 0;
            u32 groups = 0;
            u64 cameraHandle = 0;
            u64 programHandle = 0;
            //u64 srcFramebufferHandle = 0;
            //u64 dstFramebufferHandle = 0;
        };
    };

    //struct Transform
    //{
    //    static u64 TYPE;
    //    struct Info
    //    {
    //        static u64 STATE;
    //        u64 parentTransformHandle = 0;
    //        glm::fvec3 translation;
    //        glm::fquat rotation;
    //        glm::fmat4 localToWorld;
    //        glm::fmat4 worldToLocal;
    //    };
    //};

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

    void renderPass(StateDb &sdb, u32 renderMask, const Program::PrivateInfo *programPrivate,
            const glm::fmat4 &projection, const glm::fmat4 &worldToView = glm::fmat4(1.0f),
            const glm::fvec4 &renderParams = glm::fvec4(0.0f));

    bool initializeGl();

private:
    COMMON_DISABLE_COPY(Renderer)
};

#endif
