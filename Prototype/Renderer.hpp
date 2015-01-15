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
            glm::fvec4 translation;
            glm::fquat rotation;
            u64 meshId = 0;
        };
    };

    struct Camera
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            glm::fvec4 position;
            glm::fvec4 target;
        };
    };

    // TODO(martinmo): Define transform hierarchy that other entities link to
    struct Transform
    {
        static u64 TYPE;
        struct Info
        {
            static u64 STATE;
            u64 parent;
            glm::fmat4 world;
            glm::fmat4 local;
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
    struct GlHelpers;

    std::shared_ptr< GlFuncs > funcs;
    std::shared_ptr< GlState > state;
    std::shared_ptr< GlHelpers > helpers;

    bool initializeGl();

private:
    COMMON_DISABLE_COPY(Renderer);
};

#endif
