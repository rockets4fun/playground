// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Common.hpp"

#include <memory>

#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief ...
struct Renderer : public ModuleIf
{
    struct MeshInfo
    {
        glm::dvec4 position;
        glm::dquat orientation;
        size_t meshId = 0;
    };
    struct CameraInfo
    {
        glm::fvec4 position;
        glm::fvec4 target;
    };

    Renderer();
    virtual ~Renderer();

public: // Implementation of module interface
    virtual bool initialize(Platform &platform);
    virtual void shutdown(Platform &platform);
    virtual void update(Platform &platform, real64 deltaTimeInS);

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
