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

struct Platform;

// -------------------------------------------------------------------------------------------------
/// @brief Application assets handling
struct Assets
{
    enum Type
    {
        UNDEFINED,
        MODEL,
    };

    enum Flag
    {
        PROCEDURAL = 0x1,
        DYNAMIC    = 0x2
    };

    struct Info
    {
        u32 hash = 0;
        std::string name;
        u32 flags = 0;
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
        // Vertex attributes
        std::vector< glm::fvec3 > positions;
        std::vector< glm::fvec3 > normals;
        std::vector< glm::fvec3 > colors;

        std::vector< SubMeshInfo > subMeshes;

        // TODO(martinmo): Think about restructuring into meshes storing
        // TODO(martinmo): - Separate name and hash
        // TODO(martinmo): - Primitive type (others than just triangles...)
        // TODO(martinmo): - Indices into actual mesh data stored in model?
    };

    Assets();
    virtual ~Assets();

    u32 asset(const std::string &name, u32 flags = 0);
    u32 assetVersion(u32 hash);
    u32 assetFlags(u32 hash);

    Model *refModel(u32 hash);

private:
    struct PrivateState;

    std::shared_ptr< PrivateState > m_privateState;

    std::map< u32, Info > m_assetInfos;
    std::map< u32, Model > m_models;

    template< class AssetType >
    std::pair< AssetType *, Info * > refAsset(u32 hash,
        Type assetType, std::map< u32, AssetType > &assetMap)
    {
        auto ref = std::make_pair((AssetType *)(nullptr), (Info *)(nullptr));
        auto infoIter = m_assetInfos.find(hash);
        if (infoIter == m_assetInfos.end())
        {
            return ref;
        }
        if (infoIter->second.type != Type::UNDEFINED && infoIter->second.type != assetType)
        {
            return ref;
        }
        ref.first = &assetMap[hash];
        ref.second = &infoIter->second;
        return ref;
    }

    static bool loadModel(PrivateState &privateState, Model &model, Info &info);

    u32 krHash(const char *data, size_t size);

private:
    COMMON_DISABLE_COPY(Assets);
};

#endif
