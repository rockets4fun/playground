// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 10.01.2015
// -------------------------------------------------------------------------------------------------

#include "Assets.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Assets::PrivateState
{
    Assimp::Importer importer;
};

// -------------------------------------------------------------------------------------------------
Assets::Assets()
{
    m_privateState = std::make_shared< PrivateState >();
}

// -------------------------------------------------------------------------------------------------
Assets::~Assets()
{
    m_privateState = nullptr;
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
        if (loadModel(*m_privateState.get(), info, model))
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
bool Assets::loadModel(PrivateState &privateState, Info &info, Model &model)
{
    model.positions.clear();
    model.normals.clear();

    const aiScene *scene = privateState.importer.ReadFile(info.name, aiProcess_Triangulate);
    if (!scene)
    {
        // FIXME(martinmo): Proper error message through platform abstraction
        return false;
    }

    // Iterate over all faces of all meshes and append data
    for (size_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
    {
        const aiMesh *mesh = scene->mMeshes[meshIdx];
        // TODO(martinmo): Support more than just triangles...
        if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
        {
            continue;
        }
        for (size_t faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx)
        {
            const aiFace *face = &mesh->mFaces[faceIdx];
            for (size_t idx = 0; idx < face->mNumIndices; ++idx)
            {
                model.positions.push_back(mesh->mVertices[face->mIndices[idx]].x);
                model.positions.push_back(mesh->mVertices[face->mIndices[idx]].y);
                model.positions.push_back(mesh->mVertices[face->mIndices[idx]].z);
                model.normals.push_back(mesh->mNormals[face->mIndices[idx]].x);
                model.normals.push_back(mesh->mNormals[face->mIndices[idx]].y);
                model.normals.push_back(mesh->mNormals[face->mIndices[idx]].z);
            }
        }
    }

    COMMON_ASSERT(model.normals.size() == model.positions.size());
    COMMON_ASSERT(model.positions.size() % 3 == 0);

    return true;
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
    return hash == 0 ? 1 : hash;
}
