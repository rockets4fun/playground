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
    struct Mesh
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            glm::fvec3 translation;
            glm::fquat rotation;
            u32 modelAsset = 0;
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

    Renderer();
    virtual ~Renderer();

public: // Implementation of module interface
    virtual void registerTypesAndStates(StateDb &stateDb);
    virtual bool initialize(Platform &platform);
    virtual void shutdown(Platform &platform);
    virtual void update(Platform &platform, double deltaTimeInS);

private:
    struct GlFuncs;
    struct GlState;
    struct GlMesh;
    struct GlHelpers;

    std::shared_ptr< GlFuncs > funcs;
    std::shared_ptr< GlState > state;
    std::shared_ptr< GlHelpers > helpers;

    void updateTransforms(Platform &platform);
    bool initializeGl();

private:
    COMMON_DISABLE_COPY(Renderer);
};

#endif
