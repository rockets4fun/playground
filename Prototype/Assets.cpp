// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 10.01.2015
// -------------------------------------------------------------------------------------------------

#include "Assets.hpp"

#include <list>
#include <regex>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Logger.hpp"
#include "Platform.hpp"
#include "Str.hpp"
#include "Parser.hpp"

struct Assets::PrivateState
{
    Assimp::Importer importer;
};

// -------------------------------------------------------------------------------------------------
Assets::Assets()
{
    COMMON_ASSERT( sizeof( glm::fvec3 ) == 3 * sizeof( float ) );

    m_privateState = std::make_shared< PrivateState >();
}

// -------------------------------------------------------------------------------------------------
Assets::~Assets()
{
    m_privateState = nullptr;
}

// -------------------------------------------------------------------------------------------------
Assets::Model::Attr::Attr(
    const std::string& nameInit, void* dataInit, CompType typeInit, u64 countInit, u64 offsetInBInit,
    bool normalizeInit )
    : name( nameInit )
    , data( dataInit )
    , type( typeInit )
    , count( countInit )
    , offsetInB( offsetInBInit )
    , normalize( normalizeInit )
{
}

// -------------------------------------------------------------------------------------------------
u32 Assets::asset( const std::string& name, u32 flags )
{
    u32 hash      = krHash( name.c_str(), name.length() );
    auto infoIter = m_assetInfos.find( hash );
    if ( infoIter != m_assetInfos.end() ) {
        // Check for hash function collision
        COMMON_ASSERT( infoIter->second.name == name );
    }
    else {
        Info& info = m_assetInfos[ hash ];
        info.name  = name;
        info.hash  = hash;
        info.flags = flags;
    }
    return hash;
}

// -------------------------------------------------------------------------------------------------
const Assets::Info* Assets::info( u32 hash )
{
    auto infoIter = m_assetInfos.find( hash );
    if ( infoIter == m_assetInfos.end() ) {
        return nullptr;
    }
    return &infoIter->second;
}

// -------------------------------------------------------------------------------------------------
u32 Assets::touch( u32 hash )
{
    auto infoIter = m_assetInfos.find( hash );
    if ( infoIter == m_assetInfos.end() ) {
        return 0;
    }
    return ++infoIter->second.version;
}

// -------------------------------------------------------------------------------------------------
Assets::Model* Assets::refModel( u32 hash )
{
    auto ref = refAsset( hash, Type::MODEL, m_models );
    if ( !ref.first ) {
        return nullptr;
    }
    if ( ref.second->type != Type::UNDEFINED ) {
        return ref.first;
    }

    // This is the first time the model is referenced...
    if ( !( ref.second->flags & Flag::PROCEDURAL ) ) {
        Logger::debug( "Loading model \"%s\"...", ref.second->name.c_str() );
        if ( loadModel( *ref.second, *ref.first ) ) {
            ++ref.second->version;
            for ( auto& part : ref.first->parts ) {
                Logger::debug( "  Part \"%s\" (%d triangles)", part.name.c_str(), part.count / 3 );
            }
            Logger::debug(
                "Successfully loaded model with %d triangles", int( ref.first->positions.size() / 3 ) );
        }
        else {
            Logger::debug( "ERROR: Failed to load model \"%s\"", ref.second->name.c_str() );
            // TODO(martinmo): Default to unit cube if we fail to load?
        }
    }
    ref.second->type = Type::MODEL;

    return ref.first;
}

// -------------------------------------------------------------------------------------------------
Assets::Program* Assets::refProgram( u32 hash )
{
    auto ref = refAsset( hash, Type::PROGRAM, m_programs );
    if ( !ref.first ) {
        return nullptr;
    }
    if ( ref.second->type != Type::UNDEFINED ) {
        return ref.first;
    }

    if ( !( ref.second->flags & Flag::PROCEDURAL ) ) {
        if ( loadProgram( *ref.second, *ref.first ) ) {
            ++ref.second->version;
            Logger::debug( "Program \"%s\" loaded", ref.second->name.c_str() );
        }
        else {
            Logger::debug( "ERROR: Failed to load program \"%s\"", ref.second->name.c_str() );
        }
    }

    ref.second->type = Type::PROGRAM;
    return ref.first;
}

// -------------------------------------------------------------------------------------------------
Assets::Texture* Assets::refTexture( u32 hash )
{
    auto ref = refAsset( hash, Type::TEXTURE, m_textures );
    if ( !ref.first ) {
        return nullptr;
    }
    if ( ref.second->type != Type::UNDEFINED ) {
        return ref.first;
    }

    if ( !( ref.second->flags & Flag::PROCEDURAL ) ) {
        // Only procedural textures for now...
        return nullptr;
    }
    ref.second->type = Type::TEXTURE;

    return ref.first;
}

// -------------------------------------------------------------------------------------------------
void Assets::reloadModifiedAssets()
{
    std::set< u32 > toBeUpdated;
    for ( auto& dep : m_depsByFile ) {
        const std::string& filename = dep.first;
        s64 newModificationTime     = Platform::fileModificationTime( filename );
        if ( newModificationTime == dep.second.modificationTime ) {
            continue;
        }
        Logger::debug( "File \"%s\" changed on disc", filename.c_str() );
        for ( auto hash : dep.second.hashes )
            toBeUpdated.insert( hash );
        dep.second.modificationTime = newModificationTime;
    }
    for ( auto hash : toBeUpdated ) {
        Info& info = m_assetInfos[ hash ];
        if ( info.type == Type::PROGRAM ) loadProgram( info, m_programs[ hash ] );
        ++info.version;
    }
}

// -------------------------------------------------------------------------------------------------
void Assets::Model::clear()
{
    vertexCount = 0;

    positions.clear();
    normals.clear();
    diffuse.clear();
    ambient.clear();

    indices.clear();
    indicesAttr = Attr( "", nullptr, Attr::U32, 0 );

    attrs.clear();
    parts.clear();
}

// -------------------------------------------------------------------------------------------------
void Assets::Model::setDefaultAttrs()
{
    if ( !indices.empty() ) {
        indicesAttr = Attr( "", &indices[ 0 ], Attr::U32, indices.size() );
    }
    else {
        indicesAttr = Attr( "", nullptr, Attr::U32, 0 );
    }

    vertexCount = positions.size();

    if ( !parts.empty() ) {
        u64 overallCount = indicesAttr.count ? indicesAttr.count : vertexCount;
        COMMON_ASSERT( overallCount == parts.back().offset + parts.back().count );
    }

    if ( !indicesAttr.count ) {
        COMMON_ASSERT( vertexCount % 3 == 0 );
    }

    COMMON_ASSERT( positions.size() == vertexCount );
    COMMON_ASSERT( normals.size() == vertexCount );
    COMMON_ASSERT( diffuse.size() == vertexCount );
    COMMON_ASSERT( ambient.size() == vertexCount );

    if ( vertexCount > 0 ) {
        attrs = {
            MAttr( "Position", &positions[ 0 ] ),
            MAttr( "Normal", &normals[ 0 ] ),
            MAttr( "Diffuse", &diffuse[ 0 ] ),
            MAttr( "Ambient", &ambient[ 0 ] ),
        };
    }
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadModel( const Info& info, Model& model )
{
    resetDeps( info.hash );
    registerDep( info.hash, info.name );

    model.clear();
    std::string loader;
    if ( loadModelCustom( info, model ) ) {
        loader = "custom";
    }
    else if ( loadModelAssimp( info, model ) ) {
        loader = "assimp";
    }
    else {
        model.clear();
        Logger::debug( "ERROR: Failed to load model \"%s\"", info.name.c_str() );
        return false;
    }
    model.setDefaultAttrs();

    Logger::debug( "Success loading model \"%s\" using <%s> loader", info.name.c_str(), loader.c_str() );

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadModelCustom( const Info& info, Model& model )
{
    if ( !Str::endsWith( info.name, ".model" ) ) {
        return false;
    }
    std::string data;
    if ( !Str::fromFile( info.name, data ) ) {
        return false;
    }

    Parser parser;
    parser.init( data );
    parser.advance();

    // Parse instances
    while ( parser.chr() == 'i' ) {
        Model::Instance instance;

        parser.advance();
        instance.type = parser.str();
        parser.advance();
        instance.name = parser.str();
        parser.advance();
        instance.parent = parser.str();

        while ( parser.chr() != 'x' )
            parser.advance();
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 0 ].x ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 1 ].x ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 2 ].x ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 3 ].x ) ) break;
        while ( parser.chr() != 'x' )
            parser.advance();
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 0 ].y ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 1 ].y ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 2 ].y ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 3 ].y ) ) break;
        while ( parser.chr() != 'x' )
            parser.advance();
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 0 ].z ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 1 ].z ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 2 ].z ) ) break;
        parser.advance();
        if ( !parser.floatHex( instance.xform[ 3 ].z ) ) break;
        instance.xform[ 0 ].w = 0.0;
        instance.xform[ 1 ].w = 0.0;
        instance.xform[ 2 ].w = 0.0;
        instance.xform[ 3 ].w = 1.0;

        model.instances.push_back( instance );

        while ( !parser.isEol() )
            parser.advance();
        parser.advance();
    }

    // Parse materials
    while ( parser.chr() == 'm' ) {
        Model::Material material;
        parser.advance();
        material.name = parser.str();
        u32 color     = 0;
        parser.advance();
        parser.uint32Hex( color );
        material.ambient = glm::fvec4(
            float( ( color >> 24 ) & 0xff ) / 255.0f, float( ( color >> 16 ) & 0xff ) / 255.0f,
            float( ( color >> 8 ) & 0xff ) / 255.0f, float( (color)&0xff ) / 255.0f );
        parser.advance();
        parser.uint32Hex( color );
        material.diffuse = glm::fvec4(
            float( ( color >> 24 ) & 0xff ) / 255.0f, float( ( color >> 16 ) & 0xff ) / 255.0f,
            float( ( color >> 8 ) & 0xff ) / 255.0f, float( (color)&0xff ) / 255.0f );
        parser.advance();
        parser.uint32Hex( color );
        material.emission = glm::fvec4(
            float( ( color >> 24 ) & 0xff ) / 255.0f, float( ( color >> 16 ) & 0xff ) / 255.0f,
            float( ( color >> 8 ) & 0xff ) / 255.0f, float( (color)&0xff ) / 255.0f );
        parser.advance();
        parser.uint32Hex( color );
        parser.advance();  // Parse EOL
        model.materials.push_back( material );
    }
    std::map< std::string, const Model::Material* > materialsByName;
    for ( const Model::Material& material : model.materials ) {
        materialsByName[ material.name ] = &material;
    }
    Model::Material defaultMaterial;

    // Parse parts including vertices and triangles
    while ( parser.chr() == 'p' ) {
        Model::Part part;

        part.offset = model.positions.size();
        parser.advance();
        part.name = parser.str();
        parser.advance();
        part.instance = parser.str();
        parser.advance();
        part.material = parser.str();

        const Model::Material* material = materialsByName[ part.material ];
        if ( !material ) material = &defaultMaterial;

        u32 vertexCount = 0;
        parser.advance();
        parser.uint32( vertexCount );

        u32 triCount = 0;
        parser.advance();
        parser.uint32( triCount );

        std::vector< glm::fvec3 > positions( vertexCount );
        std::vector< glm::fvec3 > normals( vertexCount );
        for ( u32 vertexIdx = 0; vertexIdx < vertexCount; ++vertexIdx ) {
            while ( parser.chr() != 'v' )
                parser.advance();
            glm::fvec3& position = positions[ vertexIdx ];
            parser.advance();
            parser.floatHex( position.x );
            parser.advance();
            parser.floatHex( position.y );
            parser.advance();
            parser.floatHex( position.z );
            glm::fvec3& normal = normals[ vertexIdx ];
            parser.advance();
            parser.floatHex( normal.x );
            parser.advance();
            parser.floatHex( normal.y );
            parser.advance();
            parser.floatHex( normal.z );
            //Logger::debug("Vertex %d/%d", vertexIdx + 1, vertexCount);
        }

        while ( parser.chr() != 't' )
            parser.advance();
        for ( u32 triIdx = 0; triIdx < triCount; ++triIdx ) {
            u32 a = 0, b = 0, c = 0;
            parser.advance();
            if ( parser.isEol() ) {
                parser.advance();  // skip <eol>
                parser.advance();  // skip 't'
            }
            parser.uint32( a );
            parser.advance();
            parser.uint32( b );
            parser.advance();
            parser.uint32( c );
            model.positions.push_back( positions[ a ] );
            model.positions.push_back( positions[ b ] );
            model.positions.push_back( positions[ c ] );
            model.normals.push_back( normals[ a ] );
            model.normals.push_back( normals[ b ] );
            model.normals.push_back( normals[ c ] );
            model.ambient.push_back( glm::fvec3( material->ambient ) );
            model.ambient.push_back( glm::fvec3( material->ambient ) );
            model.ambient.push_back( glm::fvec3( material->ambient ) );
            model.diffuse.push_back( glm::fvec3( material->diffuse ) );
            model.diffuse.push_back( glm::fvec3( material->diffuse ) );
            model.diffuse.push_back( glm::fvec3( material->diffuse ) );
            //Logger::debug("Triangle %d/%d", triIdx + 1, triCount);
        }

        part.count = model.positions.size() - part.offset;
        model.parts.push_back( part );

        parser.advance();
        COMMON_ASSERT( parser.isEol() );
        if ( !parser.isEof() ) parser.advance();
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadModelAssimp( const Info& info, Model& model )
{
    const aiScene* scene = m_privateState->importer.ReadFile(
        info.name, aiProcess_Triangulate | aiProcess_FixInfacingNormals
        /*| aiProcess_PreTransformVertices*/ );
    if ( !scene ) {
        return false;
    }

    std::list< aiNode* > leafs;
    std::list< aiNode* > branches = { scene->mRootNode };
    while ( !branches.empty() ) {
        aiNode* branch = branches.front();
        branches.pop_front();

        for ( size_t childIdx = 0; childIdx < branch->mNumChildren; ++childIdx ) {
            aiNode* child = branch->mChildren[ childIdx ];
            if ( child->mNumChildren ) {
                branches.push_back( child );
            }
            else {
                leafs.push_back( child );
            }
        }
    }

    /*
    for (size_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
    {
        const aiMesh *mesh = scene->mMeshes[meshIdx];
        std::string meshName = mesh->mName.C_Str();
        std::string partName = meshName;
    }
    */

    // Iterate over all faces of all meshes and append data
    model.parts.push_back( Model::Part() );
    for ( const auto& leaf : leafs ) {
        if ( !leaf->mNumMeshes ) {
            continue;
        }

        aiMesh* firstMesh         = scene->mMeshes[ leaf->mMeshes[ 0 ] ];
        std::string firstMeshName = firstMesh->mName.C_Str();
        std::string leafName      = leaf->mName.C_Str();
        std::string partName      = leafName;

        Model::Part* currentPart = &model.parts.back();
        if ( currentPart->name.empty() ) currentPart->name = partName;
        if ( partName == "defaultobject" ) partName = currentPart->name;

        // Determine node transformation by walking from leaf up to root
        aiMatrix4x4 xform;
        aiNode* xformIter = leaf;
        do {
            xform     = xformIter->mTransformation * xform;
            xformIter = xformIter->mParent;
        } while ( xformIter );
        glm::fmat4 xformGlm(
            glm::fvec4( xform[ 0 ][ 0 ], xform[ 0 ][ 1 ], xform[ 0 ][ 2 ], xform[ 0 ][ 3 ] ),
            glm::fvec4( xform[ 1 ][ 0 ], xform[ 1 ][ 1 ], xform[ 1 ][ 2 ], xform[ 1 ][ 3 ] ),
            glm::fvec4( xform[ 2 ][ 0 ], xform[ 2 ][ 1 ], xform[ 2 ][ 2 ], xform[ 2 ][ 3 ] ),
            glm::fvec4( xform[ 3 ][ 0 ], xform[ 3 ][ 1 ], xform[ 3 ][ 2 ], xform[ 3 ][ 3 ] ) );

        for ( size_t meshIdx = 0; meshIdx < leaf->mNumMeshes; ++meshIdx ) {
            const aiMesh* mesh = scene->mMeshes[ leaf->mMeshes[ meshIdx ] ];
            COMMON_ASSERT( mesh->mNumFaces > 0 );
            // TODO(martinmo): Support more than just triangles...
            if ( mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE ) {
                Logger::debug( "WARNING: Skipping mesh in %s (non-triangle)", partName.c_str() );
                continue;
            }

            glm::fvec3 diffuse( 1.0f, 1.0f, 1.0f );
            glm::fvec3 ambient( 0.0f, 0.0f, 0.0f );
            aiMaterial* material = scene->mMaterials[ mesh->mMaterialIndex ];
            aiString materialNameAssimp;
            material->Get( AI_MATKEY_NAME, materialNameAssimp );
            std::string materialName = materialNameAssimp.C_Str();
            if ( materialName != "DefaultMaterial" ) {
                aiColor3D diffuseAssimp;
                if ( material->Get( AI_MATKEY_COLOR_DIFFUSE, diffuseAssimp ) == AI_SUCCESS ) {
                    diffuse = glm::fvec3( diffuseAssimp.r, diffuseAssimp.g, diffuseAssimp.b );
                }
                aiColor3D ambientAssimp;
                if ( material->Get( AI_MATKEY_COLOR_AMBIENT, ambientAssimp ) == AI_SUCCESS ) {
                    ambient = glm::fvec3( ambientAssimp.r, ambientAssimp.g, ambientAssimp.b );
                }
            }

            for ( size_t faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx ) {
                const aiFace* face = &mesh->mFaces[ faceIdx ];
                for ( size_t idx = 0; idx < face->mNumIndices; ++idx ) {
                    model.positions.push_back( glm::fvec3(
                        glm::fvec4(
                            mesh->mVertices[ face->mIndices[ idx ] ].x,
                            mesh->mVertices[ face->mIndices[ idx ] ].y,
                            mesh->mVertices[ face->mIndices[ idx ] ].z, 1.0f )
                        * xformGlm ) );
                    model.normals.push_back( glm::fvec3(
                        glm::fvec4(
                            mesh->mNormals[ face->mIndices[ idx ] ].x,
                            mesh->mNormals[ face->mIndices[ idx ] ].y,
                            mesh->mNormals[ face->mIndices[ idx ] ].z, 1.0f )
                        * xformGlm ) );
                    model.diffuse.push_back( diffuse );
                    model.ambient.push_back( ambient );
                }
            }
        }
        if ( partName != currentPart->name ) {
            Model::Part newPart;
            newPart.offset = currentPart->offset + currentPart->count;
            newPart.name   = partName;
            model.parts.push_back( newPart );
            currentPart = &model.parts.back();
        }
        currentPart->count = model.positions.size() - currentPart->offset;
    }

    /*
    // Debug colors for individual parts
    glm::fvec3 debugColors[] =
    {
        glm::fvec3(1.0, 0.0, 0.0),
        glm::fvec3(0.0, 1.0, 0.0),
        glm::fvec3(0.0, 0.0, 1.0),
        glm::fvec3(1.0, 1.0, 0.0),
        glm::fvec3(0.0, 1.0, 1.0),
        glm::fvec3(1.0, 0.0, 1.0)
    };
    for (auto &part : model.parts)
    {
        u64 firstVertexIdx = part.vertexOffset;
        u64 lastVertexIdx = firstVertexIdx + part.vertexCount - 1;
        for (u64 vertexIdx = firstVertexIdx; vertexIdx <= lastVertexIdx; ++vertexIdx)
        {
            model.diffuse[vertexIdx] = debugColors[(&part - &model.parts[0]) % 6];
        }
    }
    */

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Assets::loadProgram( const Info& info, Program& program )
{
    resetDeps( info.hash );
    registerDep( info.hash, info.name );

    program.sourceByType.clear();

    std::string filename = info.name;
    std::string filepath = filename.substr( 0, filename.find_last_of( "/" ) + 1 );

    std::string source;
    if ( !Str::fromFile( filename.c_str(), source ) ) {
        return false;
    }

    std::string input;
    std::smatch searchResult;

    // Pre-process include directives
    std::string preprocessedSource;
    std::regex includeRegex( "\\$include\\((.+)\\)" );
    input = source;
    while ( true ) {
        if ( !std::regex_search( input, searchResult, includeRegex ) ) {
            preprocessedSource += input;
            break;
        }
        preprocessedSource += searchResult.prefix().str();

        // FIXME(martinmo): Avoid direct/indirect recursive includes
        std::string includeFilename = filepath + searchResult.str( 1 );
        registerDep( info.hash, includeFilename );

        std::string includeSource;
        if ( !Str::fromFile( includeFilename, includeSource ) ) {
            Logger::debug( "WARNING: Failed to load include \"%s\"", includeFilename.c_str() );
        }
        else {
            Logger::debug(
                "Included \"%s\" into \"%s\" (%d B)", includeFilename.c_str(), filename.c_str(),
                int( includeSource.length() ) );
        }
        preprocessedSource += includeSource;

        input = searchResult.suffix().str();
    }

    // Split source according to type
    std::string* target = nullptr;
    std::regex targetRegex( "\\$type\\((vertex|fragment)-shader\\)" );
    input = preprocessedSource;
    while ( true ) {
        if ( !std::regex_search( input, searchResult, targetRegex ) ) {
            if ( target ) ( *target ) += input;
            break;
        }
        if ( target ) ( *target ) += searchResult.prefix();
        const std::string& match = searchResult.str( 1 );
        if ( match == "vertex" )
            target = &program.sourceByType[ Program::VERTEX_SHADER ];
        else if ( match == "fragment" )
            target = &program.sourceByType[ Program::FRAGMENT_SHADER ];
        input = searchResult.suffix().str();
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void Assets::registerDep( u32 hash, const std::string& filename )
{
    auto foundDepInfo = m_depsByFile.find( filename );
    if ( foundDepInfo == m_depsByFile.end() ) {
        foundDepInfo = m_depsByFile.insert( std::make_pair( filename, DepInfo() ) ).first;
        foundDepInfo->second.modificationTime = Platform::fileModificationTime( filename );
    }
    foundDepInfo->second.hashes.insert( hash );
}

// -------------------------------------------------------------------------------------------------
void Assets::resetDeps( u32 hash )
{
    for ( auto& dep : m_depsByFile ) {
        auto& hashes  = dep.second.hashes;
        auto foundDep = hashes.find( hash );
        if ( foundDep != hashes.end() ) hashes.erase( foundDep );
    }
}

// -------------------------------------------------------------------------------------------------
u32 Assets::krHash( const char* data, size_t size )
{
    // Inspired by:
    // http://www.irrelevantconclusion.com/2013/07/hashing-file-paths/
    // Kernighan & Ritchie "The C Programming Language" hash
    u32 hash = 0;
    while ( size-- > 0 ) {
        hash = ( hash * 131 + *data++ ) & 0xffffffff;
    }
    return hash == 0 ? 1 : hash;
}
