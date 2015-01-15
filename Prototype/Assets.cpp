// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 10.01.2015
// -------------------------------------------------------------------------------------------------

#include "Assets.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

// -------------------------------------------------------------------------------------------------
Assets::Assets()
{
}

// -------------------------------------------------------------------------------------------------
Assets::~Assets()
{
}

// -------------------------------------------------------------------------------------------------
u32 Assets::asset(const std::string &name)
{
    u32 hash = krHash(name.c_str(), name.length());
    auto infoIter = m_assetInfos.find(hash);
    if (infoIter != m_assetInfos.end())
    {
        // Check for hash function collision
        COMMON_ASSERT(infoIter->second.name == name);
    }
    else
    {
        Info &info = m_assetInfos[hash];
        info.name = name;
        info.hash = hash;
    }
    return hash;
}

// -------------------------------------------------------------------------------------------------
u32 Assets::assetVersion(u32 hash)
{
    auto infoIter = m_assetInfos.find(hash);
    if (infoIter == m_assetInfos.end())
    {
        return 0;
    }
    return infoIter->second.version;
}

// -------------------------------------------------------------------------------------------------
const Assets::Model *Assets::refModel(u32 hash)
{
    // TODO(martinmo): Generalize this into a template function for all asset types
    auto infoIter = m_assetInfos.find(hash);
    if (infoIter == m_assetInfos.end())
    {
        return nullptr;
    }
    Info &info = infoIter->second;
    if (info.type != Info::Type::UNDEFINED && info.type != Info::Type::MODEL)
    {
        return nullptr;
    }
    Model &model = m_models[hash];
    if (info.type == Info::Type::UNDEFINED)
    {
        // This function triggers a load only the first time it touches an asset
        if (loadModel(info, model))
        {
            ++info.version;
        }
        info.type = Info::Type::MODEL;
    }
    if (info.version == 0)
    {
        // TODO(martinmo): Return default model data if we failed to load (e.g. unit cube)
    }
    return &model;
}

// -------------------------------------------------------------------------------------------------
u32 Assets::krHash(const char *data, size_t size)
{
    // Kernigan & Ritchie "The C Programming Language" hash
    u32 hash = 0;
    while (size-- > 0)
    {
        hash = (hash * 131 + *data++) & 0xffffffff;
    }
    return hash;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadModel(Info &info, Model &model)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(info.name, 0);
    if (!scene)
    {
        // FIXME(martinmo): Proper error message through platform abstraction
        return false;
    }

    return true;
}
