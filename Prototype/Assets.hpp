// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 10.01.2015
// -------------------------------------------------------------------------------------------------

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "Common.hpp"

#include <memory>
#include <map>
#include <set>
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
        PROGRAM,
        TEXTURE
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
        Type type = Type::UNDEFINED;
        u32 version = 0;
    };

    struct Model
    {
        struct Part
        {
            std::string name;
            u64 vertexOffset = 0;
            u64 vertexCount = 0;
            u32 materialHint = 0;
        };

        // Vertex attribute arrays
        std::vector< glm::fvec3 > positions;
        std::vector< glm::fvec3 > normals;
        std::vector< glm::fvec3 > diffuse;
        std::vector< glm::fvec3 > ambient;

        std::vector< Part > parts;

        // TODO(martinmo): Think about restructuring into meshes storing
        // TODO(martinmo): - Separate name and hash
        // TODO(martinmo): - Primitive type (others than just triangles...)
        // TODO(martinmo): - Indices into actual mesh data stored in model?
    };

    struct Program
    {
        enum Type
        {
            VERTEX_SHADER,
            FRAGMENT_SHADER
        };

        std::map< Type, std::string > sourceByType;
    };

    struct Texture
    {
        typedef u32 Pixel;
        int width = 0;
        int height = 0;
        std::vector< Pixel > pixels;
    };

    Assets();
    virtual ~Assets();

    u32 asset(const std::string &name, u32 flags = 0);
    const Info *assetInfo(u32 hash);

    Model *refModel(u32 hash);
    Program *refProgram(u32 hash);
    Texture *refTexture(u32 hash);

    void reloadModifiedAssets();

private:
    struct PrivateState;

    struct DepInfo
    {
        s64 modificationTime = 0;
        std::set< u32 > hashes;
    };

    std::shared_ptr< PrivateState > m_privateState;

    std::map< u32, Info > m_assetInfos;

    std::map< u32, Model > m_models;
    std::map< u32, Program > m_programs;
    std::map< u32, Texture > m_textures;

    std::map< std::string, DepInfo > m_depsByFile;

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

    static bool loadFileIntoString(const std::string &filename, std::string &contents);

    bool loadModel(const Info &info, Model &model);
    bool loadProgram(const Info &info, Program &model);

    void registerDep(u32 hash, const std::string &filename);
    void resetDeps(u32 hash);

    u32 krHash(const char *data, size_t size);

private:
    COMMON_DISABLE_COPY(Assets)
};

#endif
