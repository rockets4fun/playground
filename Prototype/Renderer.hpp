// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Common.hpp"

#include <memory>

#include <glm/glm.hpp>

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Renderer module
struct Renderer : public ModuleIf
{
    struct Mesh
    {
        static size_t TYPE;
        struct Info
        {
            static size_t STATE;
            glm::fvec4 position;
            glm::fvec4 orientation;
            size_t meshId = 0;
        };
    };

    struct Camera
    {
        static size_t TYPE;
        struct Info
        {
            static size_t STATE;
            glm::fvec4 position;
            glm::fvec4 target;
        };
    };

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
