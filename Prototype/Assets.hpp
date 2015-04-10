// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 10.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "Common.hpp"

#include <memory>
#include <map>
#include <vector>
#include <string>

#include <glm/glm.hpp>

// -------------------------------------------------------------------------------------------------
/// @brief Application assets handling
struct Assets
{
    struct Info
    {
        enum Type
        {
            UNDEFINED,
            MODEL
        };
        u32 hash = 0;
        std::string name;
        Type type = UNDEFINED;
        u32 version = 0;
    };

    struct SubMeshInfo
    {
        std::string name;
        u64 triangleOffset = 0;
        u64 triangleCount = 0;
    };

    struct Model
    {
        // Positions and normals for triangle vertices
        std::vector< glm::fvec3 > positions;
        std::vector< glm::fvec3 > normals;

        std::vector< SubMeshInfo > subMeshes;

        // TODO(martinmo): Think about restructuring into meshes storing
        // TODO(martinmo): - Separate name and hash
        // TODO(martinmo): - Primitive type (others than just triangles...)
        // TODO(martinmo): - Indices into actual mesh data stored in model?
    };

    Assets();
    virtual ~Assets();

    u32 asset(const std::string &name);
    u32 assetVersion(u32 hash);

    const Model *refModel(u32 hash);

private:
    struct PrivateState;

    std::shared_ptr< PrivateState > m_privateState;

    std::map< u32, Info > m_assetInfos;
    std::map< u32, Model > m_models;

    static bool loadModel(PrivateState &privateState, Info &info, Model &model);

    u32 krHash(const char *data, size_t size);

private:
    COMMON_DISABLE_COPY(Assets);
};

#endif
