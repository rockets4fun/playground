// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 10.01.2015
// -------------------------------------------------------------------------------------------------

#include "Assets.hpp"

#include <list>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Logging.hpp"

struct Assets::PrivateState
{
    Assimp::Importer importer;
};

// -------------------------------------------------------------------------------------------------
Assets::Assets()
{
    COMMON_ASSERT(sizeof(glm::fvec3) == 3 * sizeof(float));

    m_privateState = std::make_shared< PrivateState >();
}

// -------------------------------------------------------------------------------------------------
Assets::~Assets()
{
    m_privateState = nullptr;
}

// -------------------------------------------------------------------------------------------------
u32 Assets::asset(const std::string &name, u32 flags)
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
        info.flags = flags;
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
u32 Assets::assetFlags(u32 hash)
{
    auto infoIter = m_assetInfos.find(hash);
    if (infoIter == m_assetInfos.end())
    {
        return 0;
    }
    return infoIter->second.flags;
}

// -------------------------------------------------------------------------------------------------
Assets::Model *Assets::refModel(u32 hash)
{
    auto ref = refAsset(hash, Type::MODEL, m_models);
    if (!ref.first)
    {
        return nullptr;
    }
    if (ref.second->type == Type::UNDEFINED)
    {
        // This is the first time the model is referenced
        if (ref.second->flags & Flag::PROCEDURAL)
        {
            // Procedural models will be defined by application logic
        }
        else if (loadModel(*m_privateState.get(), *ref.first, *ref.second))
        {
            ++ref.second->version;
            Logging::debug("Model \"%s\" loaded (%d triangles)",
                ref.second->name.c_str(),
                int(ref.first->positions.size() / 3));
        }
        else
        {
            Logging::debug("ERROR: Failed to load model \"%s\"", ref.second->name.c_str());
            // TODO(martinmo): Default to unit cube if we fail to load?
        }
        ref.second->type = Type::MODEL;
    }
    return ref.first;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadModel(PrivateState &privateState, Model &model, Info &info)
{
    model.positions.clear();
    model.normals.clear();
    model.colors.clear();

    const aiScene *scene = privateState.importer.ReadFile(info.name, aiProcess_Triangulate);
    if (!scene)
    {
        // FIXME(martinmo): Proper error message through platform abstraction
        return false;
    }

    std::list< aiNode * > leafs;
    std::list< aiNode * > branches = { scene->mRootNode };
    while (!branches.empty())
    {
        aiNode *branch = branches.front();
        branches.pop_front();

        for (size_t childIdx = 0; childIdx < branch->mNumChildren; ++childIdx)
        {
            aiNode *child = branch->mChildren[childIdx];
            if (child->mChildren)
            {
                branches.push_back(child);
            }
            else
            {
                leafs.push_back(child);
            }
        }
    }

    // Iterate over all faces of all meshes and append data
    u64 prevTriangleCount = 0;
    for (auto leaf : leafs)
    {
        for (size_t meshIdx = 0; meshIdx < leaf->mNumMeshes; ++meshIdx)
        {
            const aiMesh *mesh = scene->mMeshes[leaf->mMeshes[meshIdx]];
            COMMON_ASSERT(mesh->mNumFaces > 0);
            // TODO(martinmo): Support more than just triangles...
            if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
            {
                // FIXME(martinmo): Add warning if we skip meshes...
                continue;
            }

            glm::fvec3 diffuseColor(1.0f, 1.0f, 1.0f);

            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            aiString materialNameAssimp;
            material->Get(AI_MATKEY_NAME, materialNameAssimp);
            std::string materialName = materialNameAssimp.C_Str();
            if (materialName != "DefaultMaterial")
            {
                aiColor3D diffuseColorAssimp;
                if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColorAssimp) == AI_SUCCESS)
                {
                    diffuseColor = glm::fvec3(
                        diffuseColorAssimp.r, diffuseColorAssimp.g, diffuseColorAssimp.b);
                }
            }

            for (size_t faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx)
            {
                const aiFace *face = &mesh->mFaces[faceIdx];
                for (size_t idx = 0; idx < face->mNumIndices; ++idx)
                {
                    model.positions.push_back(glm::fvec3(
                        mesh->mVertices[face->mIndices[idx]].x,
                        mesh->mVertices[face->mIndices[idx]].y,
                        mesh->mVertices[face->mIndices[idx]].z));
                    model.normals.push_back(glm::fvec3(
                        mesh->mNormals[face->mIndices[idx]].x,
                        mesh->mNormals[face->mIndices[idx]].y,
                        mesh->mNormals[face->mIndices[idx]].z));
                    model.colors.push_back(diffuseColor);
                }
            }
            u64 triangleCount = model.positions.size() / 3;
            {
                SubMeshInfo subMesh;
                subMesh.name = leaf->mName.C_Str();
                subMesh.triangleOffset = prevTriangleCount;
                subMesh.triangleCount = triangleCount - prevTriangleCount;
                model.subMeshes.push_back(subMesh);
            }
            prevTriangleCount = triangleCount;
        }
    }

    COMMON_ASSERT(model.normals.size() == model.positions.size());
    COMMON_ASSERT(model.positions.size() % 3 == 0);

    return true;
}

// -------------------------------------------------------------------------------------------------
u32 Assets::krHash(const char *data, size_t size)
{
    // Inspired by:
    // http://www.irrelevantconclusion.com/2013/07/hashing-file-paths/
    // Kernigan & Ritchie "The C Programming Language" hash
    u32 hash = 0;
    while (size-- > 0)
    {
        hash = (hash * 131 + *data++) & 0xffffffff;
    }
    return hash == 0 ? 1 : hash;
}
