// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "Common.hpp"

struct StateDb;

// -------------------------------------------------------------------------------------------------
/// @brief Brief implementation module description
struct Platform
{
    Platform(StateDb &stateDb);
    virtual ~Platform();

public:
    StateDb &stateDb;

    size_t RendererMesh = 0;
    size_t RendererMeshInfo = 0;

    size_t RendererCamera = 0;
    size_t RendererCameraInfo = 0;

private:
    COMMON_DISABLE_COPY(Platform);
};

#endif
