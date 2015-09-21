// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 10.01.2015
// -------------------------------------------------------------------------------------------------

#include "Assets.hpp"

#include <list>
#include <fstream>
#include <regex>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Logging.hpp"
#include "Platform.hpp"

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
const Assets::Info *Assets::assetInfo(u32 hash)
{
    auto infoIter = m_assetInfos.find(hash);
    if (infoIter == m_assetInfos.end())
    {
        return nullptr;
    }
    return &infoIter->second;
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
        else if (loadModel(*ref.second, *ref.first))
        {
            ++ref.second->version;
            Logging::debug("Model \"%s\" loaded (%d triangles)",
                ref.second->name.c_str(), int(ref.first->positions.size() / 3));
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
Assets::Program *Assets::refProgram(u32 hash)
{
    auto ref = refAsset(hash, Type::PROGRAM, m_programs);
    if (!ref.first)
    {
        return nullptr;
    }
    if (ref.second->type == Type::UNDEFINED)
    {
        if (loadProgram(*ref.second, *ref.first))
        {
            ++ref.second->version;
            Logging::debug("Program \"%s\" loaded", ref.second->name.c_str());
        }
        else
        {
            Logging::debug("ERROR: Failed to load program \"%s\"", ref.second->name.c_str());
        }
        ref.second->type = Type::PROGRAM;
    }
    return ref.first;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadFileIntoString(const std::string &filename, std::string &contents)
{
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        return false;
    }
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return true;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadModel(const Info &info, Model &model)
{
    resetDeps(info.hash);
    registerDep(info.hash, info.name);

    model.positions.clear();
    model.normals.clear();
    model.colors.clear();
    model.parts.clear();

    const aiScene *scene = m_privateState->importer.ReadFile(info.name, aiProcess_Triangulate);
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
                Model::Part part;
                part.name = leaf->mName.C_Str();
                part.triangleOffset = prevTriangleCount;
                part.triangleCount = triangleCount - prevTriangleCount;
                model.parts.push_back(part);
            }
            prevTriangleCount = triangleCount;
        }
    }

    COMMON_ASSERT(model.normals.size() == model.positions.size());
    COMMON_ASSERT(model.positions.size() % 3 == 0);

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadProgram(const Info &info, Program &program)
{
    resetDeps(info.hash);
    registerDep(info.hash, filename);

    program.sourceByType.clear();

    std::string filename = info.name;
    std::string filepath = filename.substr(0, filename.find_last_of("/") + 1);

    std::string source;
    if (!loadFileIntoString(filename.c_str(), source))
    {
        return false;
    }

    std::string input;
    std::smatch searchResult;

    // Pre-process include directives
    std::string preprocessedSource;
    std::regex includeRegex("\\$include\\((.+)\\)");
    input = source;
    while (true)
    {
        if (!std::regex_search(input, searchResult, includeRegex))
        {
            preprocessedSource += input;
            break;
        }
        preprocessedSource += searchResult.prefix().str();

        // FIXME(martinmo): Avoid direct/indirect recursive includes
        std::string includeFilename = filepath + searchResult.str(1);
        registerDep(info.hash, includeFilename);

        std::string includeSource;
        if (!loadFileIntoString(includeFilename, includeSource))
        {
            Logging::debug("WARNING: Failed to load include \"%s\"", includeFilename.c_str());
        }
        else
        {
            Logging::debug("Included \"%s\" into \"%s\" (%d B)",
                includeFilename.c_str(), filename.c_str(), int(includeSource.length()));
        }
        preprocessedSource += includeSource;

        input = searchResult.suffix().str();
    }

    // Split source according to type
    std::string *target = nullptr;
    std::regex targetRegex("\\$type\\((vertex|fragment)-shader\\)");
    input = preprocessedSource;
    while (true)
    {
        if (!std::regex_search(input, searchResult, targetRegex))
        {
            if (target) (*target) += input;
            break;
        }
        if (target) (*target) += searchResult.prefix();
        const std::string &match = searchResult.str(1);
        if      (match == "vertex")   target = &program.sourceByType[Program::VERTEX_SHADER];
        else if (match == "fragment") target = &program.sourceByType[Program::FRAGMENT_SHADER];
        input = searchResult.suffix().str();
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void Assets::registerDep(u32 hash, const std::string &filename)
{
    auto foundDepInfo = m_depsByFile.find(filename);
    if (foundDepInfo == m_depsByFile.end())
    {
        foundDepInfo = m_depsByFile.insert(std::make_pair(filename, DepInfo())).first;
        foundDepInfo->second.modificationTime = Platform::fileModificationTime(filename);
    }
    foundDepInfo->second.hashes.insert(hash);
}

// -------------------------------------------------------------------------------------------------
void Assets::resetDeps(u32 hash)
{
    for (auto &dep : m_depsByFile)
    {
        auto &hashes = dep.second.hashes;
        auto foundDep = hashes.find(hash);
        if (foundDep != hashes.end()) hashes.erase(foundDep);
    }
}

// -------------------------------------------------------------------------------------------------
void Assets::checkDeps()
{
    std::set< u32 > toBeUpdated;
    for (auto &dep : m_depsByFile)
    {
        const std::string &filename = dep.first;
        s64 newModificationTime = Platform::fileModificationTime(filename);
        if (newModificationTime == dep.second.modificationTime)
        {
            continue;
        }
        Logging::debug("File \"%s\" changed on disc", filename.c_str());
        for (auto hash : dep.second.hashes) toBeUpdated.insert(hash);
        dep.second.modificationTime = newModificationTime;
    }
    for (auto hash : toBeUpdated)
    {
        Info &info = m_assetInfos[hash];
        if (info.type == Type::PROGRAM) loadProgram(info, m_programs[hash]);
        ++info.version;
    }
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
