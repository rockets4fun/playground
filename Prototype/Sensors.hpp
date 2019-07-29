// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 15.03.2017
// -------------------------------------------------------------------------------------------------

#ifndef SENSORS_HPP
#define SENSORS_HPP

#include "Common.hpp"

#include "ModuleIf.hpp"

// -------------------------------------------------------------------------------------------------
/// @brief Sensors module
struct Sensors : public ModuleIf
{
    Sensors();
    virtual ~Sensors();

    static void updateAccelerometer( Sensor::Info& sensor, glm::fvec4 accelLinear );
    static void updateGyro( Sensor::Info& sensor, glm::fvec4 accelAngular );

public:  // Implementation of module interface
    virtual void registerTypesAndStates( StateDb& sdb );
    virtual bool initialize( StateDb& sdb, Assets& assets );
    virtual void shutdown( StateDb& sdb );
    virtual void update( StateDb& sdb, Assets& assets, Renderer& renderer, double deltaTimeInS );

private:
    COMMON_DISABLE_COPY( Sensors )
};

#endif
