// -------------------------------------------------------------------------------------------------
/// @author Martin Moerth (MARTINMO)
/// @date 02.01.2015
// -------------------------------------------------------------------------------------------------

#define GLM_SWIZZLE

#include "Renderer.hpp"

// TODO(martinmo): Remove dependency to SDL through OpenGL function retrieval interface
// TODO(martinmo): This can then be implemented in the platform layer through e.g. SDL/Qt
// TODO(martinmo): - 'glProcAddr(name)'
// TODO(martinmo): - 'glIsExtensionSupported(name)'
#include <SDL.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Logging.hpp"
#include "Profiling.hpp"

#include "StateDb.hpp"
#include "Assets.hpp"

// Include file containing all OpenGL declarations and definitions
// ==> This will never be included outside of this class
#include "CoreGl/glcorearb.h"

#ifdef COMMON_WINDOWS
#   include <Brofiler.h>
#endif

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::PrivateFuncs
{
    // Basic functions
    PFNGLENABLEPROC   glEnable = nullptr;
    PFNGLCULLFACEPROC glCullFace = nullptr;
    PFNGLFINISHPROC   glFinish = nullptr;
    // Buffer clearing functions
    PFNGLCLEARCOLORPROC glClearColor = nullptr;
    PFNGLCLEARPROC      glClear = nullptr;
    // Shader object functions
    PFNGLCREATESHADERPROC     glCreateShader = nullptr;
    PFNGLSHADERSOURCEPROC     glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC    glCompileShader = nullptr;
    PFNGLDELETESHADERPROC     glDeleteShader = nullptr;
    PFNGLGETSHADERIVPROC      glGetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
    // Program object functions
    PFNGLCREATEPROGRAMPROC     glCreateProgram = nullptr;
    PFNGLATTACHSHADERPROC      glAttachShader = nullptr;
    PFNGLDETACHSHADERPROC      glDetachShader = nullptr;
    PFNGLLINKPROGRAMPROC       glLinkProgram = nullptr;
    PFNGLUSEPROGRAMPROC        glUseProgram = nullptr;
    PFNGLDELETEPROGRAMPROC     glDeleteProgram = nullptr;
    PFNGLGETPROGRAMIVPROC      glGetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
    // Vertex array object functions
    PFNGLGENVERTEXARRAYSPROC    glGenVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC    glBindVertexArray = nullptr;
    // Buffer object functions
    PFNGLGENBUFFERSPROC    glGenBuffers = nullptr;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
    PFNGLBINDBUFFERPROC    glBindBuffer = nullptr;
    PFNGLBUFFERDATAPROC    glBufferData = nullptr;
    PFNGLBUFFERSUBDATAPROC glBufferSubData = nullptr;
    // Shader uniform functions
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
    PFNGLUNIFORM4FVPROC         glUniform4fv = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv = nullptr;
    // Vertex shader attribute functions
    PFNGLGETATTRIBLOCATIONPROC       glGetAttribLocation = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
    // Drawing command functions
    PFNGLDRAWARRAYSPROC glDrawArrays = nullptr;
    // ARB_debug_output (extension to 3.2 core)
    PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = nullptr;
};

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::PrivateState
{
    GLuint defVs = 0;
    GLuint defFs = 0;
    GLuint defProg = 0;

    GLint defProgUniformModelWorldMatrix;
    GLint defProgUniformModelViewMatrix;
    GLint defProgUniformProjectionMatrix;
    GLint defProgUniformRenderParams;

    GLint defProgAttribPosition;
    GLint defProgAttribNormal;
    GLint defProgAttribColor;

    GLuint defVao = 0;

    GLuint cubePositionsVbo;
    GLuint cubeNormalsVbo;

    std::map< u32, PrivateMesh > meshes;
};

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::PrivateMesh
{
    enum Flag
    {
        DYNAMIC = 0x1,
        DIRTY   = 0x2
    };

    Assets::Model *model = nullptr;

    GLuint positionsVbo = 0;
    GLuint normalsVbo = 0;
    GLuint colorsVbo = 0;
    GLenum usage = GL_STATIC_DRAW;

    int vertexCount = 0;
    u32 flags = 0;
};

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::PrivateHelpers
{
    typedef void (APIENTRYP GetProc)
        (GLuint shader, GLenum pname, GLint *params);
    typedef void (APIENTRYP InfoLogProc)
        (GLuint object, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

    PrivateHelpers(PrivateFuncs *funcs);

    bool createAndCompileShader(GLuint &shader, GLenum type, const std::string &source);
    bool createAndLinkSimpleProgram(GLuint &program, GLuint vertexShader, GLuint fragmentShader);

    void printInfoLog(GLuint object, GetProc getProc, InfoLogProc infoLogProc);

    void updateVertexBufferData(GLuint vbo,
            const std::vector< glm::fvec3 > &data, GLenum usage, int prevVertexCount = 0);

private:
    PrivateFuncs *funcs = nullptr;
};

u64 Renderer::Mesh::TYPE = 0;
u64 Renderer::Mesh::Info::STATE = 0;

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::Mesh::PrivateInfo
{
    static u64 STATE;
    // Store 1:n relation between mesh data from model ('PrivateMesh') and mesh instance
    PrivateMesh *privateMesh = nullptr;
};
u64 Renderer::Mesh::PrivateInfo::STATE = 0;

u64 Renderer::Camera::TYPE = 0;
u64 Renderer::Camera::Info::STATE = 0;

u64 Renderer::Transform::TYPE = 0;
u64 Renderer::Transform::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
Renderer::PrivateHelpers::PrivateHelpers(PrivateFuncs *funcs) : funcs(funcs)
{
}

// -------------------------------------------------------------------------------------------------
bool Renderer::PrivateHelpers::createAndCompileShader(
    GLuint &shader, GLenum type, const std::string &src)
{
    shader = funcs->glCreateShader(type);
    if (!shader)
    {
        Logging::debug("ERROR: Failed to create shader object");
        return false;
    }

    GLint shaderLength = GLint(src.length());
    const GLchar *srcPtr = (const GLchar *)src.c_str();
    funcs->glShaderSource(shader, 1, &srcPtr, &shaderLength);
    funcs->glCompileShader(shader);

    printInfoLog(shader, funcs->glGetShaderiv, funcs->glGetShaderInfoLog);

    GLint compileStatus = GL_FALSE;
    funcs->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE)
    {
        Logging::debug("ERROR: Failed to compile shader");
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Renderer::PrivateHelpers::createAndLinkSimpleProgram(
    GLuint &program, GLuint vertexShader, GLuint fragmentShader)
{
    program = funcs->glCreateProgram();
    if (!program)
    {
        Logging::debug("ERROR: Failed to create program object");
        return false;
    }

    funcs->glAttachShader(program, vertexShader);
    funcs->glAttachShader(program, fragmentShader);
    funcs->glLinkProgram(program);

    printInfoLog(program, funcs->glGetProgramiv, funcs->glGetProgramInfoLog);

    GLint linkStatus = GL_FALSE;
    funcs->glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        Logging::debug("ERROR: Failed to link program");
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::PrivateHelpers::printInfoLog(
        GLuint object, GetProc getProc, InfoLogProc infoLogProc)
{
    // TODO(martinmo): Refactor into 'getInfoLog' and separate print
    GLint infoLogLength = 0;
    getProc(object, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 1)
    {
        std::string infoLog(infoLogLength, 'x');
        infoLogProc(object, GLsizei(infoLog.length()), nullptr, &infoLog[0]);
        Logging::debug("----------------------------------------");
        Logging::debug("%s", infoLog.c_str());
        Logging::debug("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    }
}

// -------------------------------------------------------------------------------------------------
void Renderer::PrivateHelpers::updateVertexBufferData(GLuint vbo,
        const std::vector< glm::fvec3 > &data, GLenum usage, int prevVertexCount)
{
    int vertexCount = int(data.size());
    funcs->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (vertexCount != prevVertexCount)
    {
        funcs->glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::fvec3), &data[0].x, usage);
    }
    else
    {
        funcs->glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::fvec3), &data[0].x);
    }
}

// -------------------------------------------------------------------------------------------------
void APIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const void *userParam)
{
    Logging::debug("GL: %s", message);
}

// -------------------------------------------------------------------------------------------------
Renderer::Renderer()
{
}

// -------------------------------------------------------------------------------------------------
Renderer::~Renderer()
{
}

// -------------------------------------------------------------------------------------------------
void Renderer::registerTypesAndStates(StateDb &stateDb)
{
    Mesh::TYPE = stateDb.registerType("Mesh", 4096);
    Mesh::Info::STATE = stateDb.registerState(
        Mesh::TYPE, "Info", sizeof(Mesh::Info));
    Mesh::PrivateInfo::STATE = stateDb.registerState(
        Mesh::TYPE, "PrivateInfo", sizeof(Mesh::PrivateInfo));

    Camera::TYPE = stateDb.registerType("Camera", 8);
    Camera::Info::STATE = stateDb.registerState(
        Camera::TYPE, "Info", sizeof(Camera::Info));

    Transform::TYPE = stateDb.registerType("Transform", 4096);
    Transform::Info::STATE = stateDb.registerState(
        Transform::TYPE, "Info", sizeof(Transform::Info));
}

// -------------------------------------------------------------------------------------------------
bool Renderer::initialize(StateDb &sdb, Assets &assets)
{
    funcs = std::make_shared< PrivateFuncs >();
    state = std::make_shared< PrivateState >();
    helpers = std::make_shared< PrivateHelpers >(funcs.get());

    if (!initializeGl())
    {
        return false;
    }

    if (SDL_GL_ExtensionSupported("GL_ARB_debug_output"))
    {
        funcs->glDebugMessageCallbackARB(debugMessageCallback, this);
        funcs->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    }

    /*
    funcs->glCullFace(GL_BACK);
    funcs->glEnable(GL_CULL_FACE);
    */

    std::string defVsSrc =
        "#version 150\n"
        "\n"
        "uniform mat4 ModelWorldMatrix;\n"
        "uniform mat4 ModelViewMatrix;\n"
        "uniform mat4 ProjectionMatrix;\n"
        "\n"
        "uniform vec4 RenderParams;\n"
        "\n"
        "in vec3 Position;\n"
        "in vec3 Normal;\n"
        "in vec3 Color;\n"
        "\n"
        "out vec4 renderParams;\n"
        "out vec3 vertexNormal;\n"
        "out vec3 vertexNormalWorld;\n"
        "out vec3 vertexColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    renderParams = RenderParams;\n"
        "    vertexNormal = normalize(Normal).xyz;\n"
        "    vertexNormalWorld = normalize((ModelWorldMatrix * vec4(Normal, 0.0)).xyz);\n"
        "    vertexColor = Color;\n"
        "    gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(Position, 1.0);\n"
        "}\n";
    helpers->createAndCompileShader(state->defVs, GL_VERTEX_SHADER, defVsSrc);

    std::string defFsSrc =
        "#version 150\n"
        "\n"
        "in vec4 renderParams;\n"
        "in vec3 vertexNormal;\n"
        "in vec3 vertexNormalWorld;\n"
        "in vec3 vertexColor;\n"
        "\n"
        "out vec4 fragmentColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (renderParams.x > 0.5)\n"
        "    {\n"
        "        fragmentColor = vec4(abs(vertexNormal) * vertexColor, 1.0);\n"
        "    }\n"
        "    else if (renderParams.y > 0.5)\n"
        "    {\n"
        "        fragmentColor = vec4(vertexColor, 1.0);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        vec3 lightDir = normalize(vec3(0.0, 1.0, -1.0));\n"
        "        float lambert = max(0.0, dot(vertexNormalWorld, -lightDir));\n"
        "        fragmentColor = vec4(vertexColor * max(0.2, lambert), 1.0);\n"
        "    }\n"
        "}\n";
    helpers->createAndCompileShader(state->defFs, GL_FRAGMENT_SHADER, defFsSrc);

    helpers->createAndLinkSimpleProgram(state->defProg, state->defVs, state->defFs);

    // TODO(martinmo): Use Uniform Buffer Objects to pass uniforms to shaders

    state->defProgUniformModelWorldMatrix =
        funcs->glGetUniformLocation(state->defProg, "ModelWorldMatrix");
    state->defProgUniformModelViewMatrix =
        funcs->glGetUniformLocation(state->defProg, "ModelViewMatrix");
    state->defProgUniformProjectionMatrix =
        funcs->glGetUniformLocation(state->defProg, "ProjectionMatrix");
    state->defProgUniformRenderParams =
        funcs->glGetUniformLocation(state->defProg, "RenderParams");

    // TODO(martinmo): Use 'glBindAttribLocation()' before linking the program to force
    // TODO(martinmo): assignment of attributes to specific fixed locations
    // TODO(martinmo): (e.g. position always 0, color always 1 etc.)
    // TODO(martinmo): ==> This allows us to use the same VAO for different shader programs

    state->defProgAttribPosition =
        funcs->glGetAttribLocation(state->defProg, "Position");
    state->defProgAttribNormal =
        funcs->glGetAttribLocation(state->defProg, "Normal");
    state->defProgAttribColor =
        funcs->glGetAttribLocation(state->defProg, "Color");

    funcs->glUseProgram(state->defProg);

    funcs->glGenVertexArrays(1, &state->defVao);
    funcs->glBindVertexArray(state->defVao);

    // World coordinate system: +X=E, +Y=N and +Z=up
    // Front facing is CCW towards negative axis
    float cubePositions[] =
    {
        // West -X
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, +0.5f, -0.5f, +0.5f, +0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, -0.5f,
        // East +X
        +0.5f, -0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, -0.5f, +0.5f,
        +0.5f, -0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, +0.5f, +0.5f,
        // South -Y
        -0.5f, -0.5f, -0.5f, +0.5f, -0.5f, +0.5f, -0.5f, -0.5f, +0.5f,
        -0.5f, -0.5f, -0.5f, +0.5f, -0.5f, -0.5f, +0.5f, -0.5f, +0.5f,
        // North +Y
        -0.5f, +0.5f, -0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, +0.5f, 
        -0.5f, +0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, +0.5f, -0.5f, 
        // Bottom -Z
        -0.5f, -0.5f, -0.5f, -0.5f, +0.5f, -0.5f, +0.5f, +0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, -0.5f, -0.5f,
        // Top +Z
        -0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, -0.5f, +0.5f, +0.5f,
        -0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f,
    };
    float cubeNormals[] =
    {
        // West -X (red)
        -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        // East +X (red)
        +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f,
        +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f,
        // South -Y (green)
         0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,
         0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,
        // North +Y (green)
         0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,
         0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,
        // Bottom -Z (blue)
         0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,
         0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f,
        // Top +Z (blue)
         0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,
         0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,  0.0f,  0.0f, +1.0f,
    };

    // Create cube VBOs and define data
    {
        funcs->glGenBuffers(1, &state->cubePositionsVbo);
        funcs->glBindBuffer(GL_ARRAY_BUFFER, state->cubePositionsVbo);
        funcs->glBufferData(GL_ARRAY_BUFFER, sizeof(cubePositions), cubePositions, GL_STATIC_DRAW);

        funcs->glGenBuffers(1, &state->cubeNormalsVbo);
        funcs->glBindBuffer(GL_ARRAY_BUFFER, state->cubeNormalsVbo);
        funcs->glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNormals), cubeNormals, GL_STATIC_DRAW);
    }

    funcs->glClearColor(0.15f, 0.15f, 0.15f, 1.0);
    funcs->glEnable(GL_DEPTH_TEST);

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::shutdown(StateDb &sdb)
{
    if (state->cubeNormalsVbo) funcs->glDeleteBuffers(1, &state->cubeNormalsVbo);
    if (state->cubePositionsVbo) funcs->glDeleteBuffers(1, &state->cubePositionsVbo);

    if (state->defVao) funcs->glDeleteVertexArrays(1, &state->defVao);

    if (state->defProg) funcs->glDeleteProgram(state->defProg);
    if (state->defFs) funcs->glDeleteShader(state->defFs);
    if (state->defVs) funcs->glDeleteShader(state->defVs);

    for (auto &mapIter : state->meshes)
    {
        funcs->glDeleteBuffers(2, &mapIter.second.positionsVbo);
    }

    helpers = nullptr;
    state = nullptr;
    funcs = nullptr;
}

// -------------------------------------------------------------------------------------------------
void Renderer::update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
//#ifdef COMMON_WINDOWS
//    BROFILER_CATEGORY("Renderer", Profiler::Color::Blue)
//#endif
    PROFILING_SECTION(Renderer, glm::fvec3(0.0f, 0.0f, 1.0f))
    {
        PROFILING_SECTION(FinishBegin, glm::fvec3(1.0f, 0.5f, 0.0f))
        funcs->glFinish();
    }

    // Prepare references to per-model private data
    auto meshes = sdb.stateAll< Mesh::Info >();
    auto meshesPrivate = sdb.stateAll< Mesh::PrivateInfo >();
    for (auto meshPrivate : meshesPrivate)
    {
        if (!meshPrivate->privateMesh)
        {
            auto mesh = meshes.rel(meshesPrivate, meshPrivate);
            meshPrivate->privateMesh = &state->meshes[mesh->modelAsset];
        }
    }
    // Prepare per-model private data
    for (auto &meshMapEntry : state->meshes)
    {
        u32 modelAsset = meshMapEntry.first;
        PrivateMesh *privateMesh = &meshMapEntry.second;
        // TODO(martinmo): Delay loading the model further until we actually render?
        // TODO(martinmo): ==> Practical applications might even prefetch on init...
        if (!privateMesh->model)
        {
            privateMesh->model = assets.refModel(modelAsset);
            COMMON_ASSERT(privateMesh->model);
            // TODO(martinmo): Add way of getting asset and flags in one call/lookup
            if (assets.assetFlags(modelAsset) & Assets::Flag::DYNAMIC)
            {
                privateMesh->flags |= PrivateMesh::Flag::DYNAMIC;
                privateMesh->usage = GL_DYNAMIC_DRAW;
            }
            funcs->glGenBuffers(1, &privateMesh->positionsVbo);
            funcs->glGenBuffers(1, &privateMesh->normalsVbo);
            funcs->glGenBuffers(1, &privateMesh->colorsVbo);
            privateMesh->flags |= PrivateMesh::Flag::DIRTY;
        }
        else if (privateMesh->flags & PrivateMesh::Flag::DYNAMIC)
        {
            privateMesh->flags |= PrivateMesh::Flag::DIRTY;
        }
    }

    // Default render pass
    {
        const float aspect = 16.0f / 9.0f;
        glm::fmat4 projection = glm::perspective(
                    glm::radians(30.0f * aspect), aspect, 0.5f, 100.0f);
        glm::fmat4 worldToView;
        if (activeCameraHandle)
        {
            auto camera = sdb.state< Camera::Info >(activeCameraHandle);
            worldToView = glm::lookAt(camera->position.xyz(),
                camera->target.xyz(), glm::fvec3(0.0f, 0.0f, 1.0f));
        }
        funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        funcs->glUniform4fv(state->defProgUniformRenderParams, 1,
            glm::value_ptr(glm::fvec4(debugNormals ? 1.0f : 0.0f, 0.0, 0.0, 0.0)));
        renderPass(sdb, Group::DEFAULT, projection, worldToView);
    }

    // UI render pass
    {
        glm::fmat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 450.0f, -10.0f, 10.0f);
        glm::fmat4 worldToView;
        funcs->glClear(GL_DEPTH_BUFFER_BIT);
        funcs->glUniform4fv(state->defProgUniformRenderParams, 1,
            glm::value_ptr(glm::fvec4(debugNormals ? 1.0f : 0.0f, 1.0, 0.0, 0.0)));
        renderPass(sdb, Group::DEFAULT_UI, projection, worldToView);
    }

    {
        PROFILING_SECTION(FinishEnd, glm::fvec3(1.0f, 0.5f, 0.0f))
        funcs->glFinish();
    }
}

// -------------------------------------------------------------------------------------------------
void Renderer::renderPass(StateDb &sdb, u32 renderMask,
        const glm::fmat4 &projection, const glm::fmat4 &worldToView)
{
    funcs->glUniformMatrix4fv(state->defProgUniformProjectionMatrix,
                1, GL_FALSE, glm::value_ptr(projection));

    auto meshes = sdb.stateAll< Mesh::Info >();
    auto meshesPrivate = sdb.stateAll< Mesh::PrivateInfo >();

    // Pseudo-instanced rendering of meshes
    for (auto mesh : meshes)
    {
        if (mesh->flags & MeshFlag::HIDDEN)
        {
            continue;
        }
        if (!(mesh->groups & renderMask))
        {
            continue;
        }

        auto meshPrivate = meshesPrivate.rel(meshes, mesh);
        PrivateMesh *privateMesh = meshPrivate->privateMesh;

        // Update/define vertex buffer data
        int oldVertexCount = privateMesh->vertexCount;
        privateMesh->vertexCount = int(privateMesh->model->positions.size());
        if (privateMesh->flags & PrivateMesh::Flag::DIRTY && privateMesh->vertexCount)
        {
            helpers->updateVertexBufferData(privateMesh->positionsVbo,
                privateMesh->model->positions, privateMesh->usage, oldVertexCount);
            helpers->updateVertexBufferData(privateMesh->normalsVbo,
                privateMesh->model->normals, privateMesh->usage, oldVertexCount);
            helpers->updateVertexBufferData(privateMesh->colorsVbo,
                privateMesh->model->colors, privateMesh->usage, oldVertexCount);
            privateMesh->flags &= ~PrivateMesh::Flag::DIRTY;
        }
        if (!privateMesh->vertexCount)
        {
            continue;
        }

        // Vertex attribute assignments are stored inside the bound VAO
        // ==> Think about creating one VAO per renderable mesh
        {
            funcs->glBindBuffer(GL_ARRAY_BUFFER, privateMesh->positionsVbo);
            funcs->glVertexAttribPointer(
                state->defProgAttribPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
            funcs->glEnableVertexAttribArray(state->defProgAttribPosition);

            funcs->glBindBuffer(GL_ARRAY_BUFFER, privateMesh->normalsVbo);
            funcs->glVertexAttribPointer(
                state->defProgAttribNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
            funcs->glEnableVertexAttribArray(state->defProgAttribNormal);

            funcs->glBindBuffer(GL_ARRAY_BUFFER, privateMesh->colorsVbo);
            funcs->glVertexAttribPointer(
                state->defProgAttribColor, 3, GL_FLOAT, GL_FALSE, 0, 0);
            funcs->glEnableVertexAttribArray(state->defProgAttribColor);
        }

        glm::fmat4 modelToWorld;
        modelToWorld = glm::translate(glm::fmat4(), mesh->translation);
        modelToWorld = modelToWorld * glm::mat4_cast(mesh->rotation);
        funcs->glUniformMatrix4fv(
            state->defProgUniformModelWorldMatrix, 1, GL_FALSE, glm::value_ptr(modelToWorld));

        glm::fmat4 modelToView = worldToView * modelToWorld;
        funcs->glUniformMatrix4fv(
            state->defProgUniformModelViewMatrix, 1, GL_FALSE, glm::value_ptr(modelToView));

        funcs->glDrawArrays(GL_TRIANGLES, 0, privateMesh->vertexCount);
    }
}

/*
// -------------------------------------------------------------------------------------------------
void Renderer::updateTransforms(StateDb &sdb)
{
    auto transforms = sdb.stateAll< Transform::Info >();
    for (auto transform : transforms)
    {
        // ...
    }
}
*/

// -------------------------------------------------------------------------------------------------
#define RENDERER_GL_FUNC(Name) \
    { \
        funcs->Name = (decltype(funcs->Name))(SDL_GL_GetProcAddress(#Name)); \
        if (funcs->Name == nullptr) \
        { \
            Logging::debug("ERROR: Failed to resolve '%s'", #Name); \
            return false; \
        } \
    }
bool Renderer::initializeGl()
{
    RENDERER_GL_FUNC(glEnable);
    RENDERER_GL_FUNC(glCullFace);
    RENDERER_GL_FUNC(glFinish);

    RENDERER_GL_FUNC(glClearColor);
    RENDERER_GL_FUNC(glClear);

    RENDERER_GL_FUNC(glCreateShader);
    RENDERER_GL_FUNC(glShaderSource);
    RENDERER_GL_FUNC(glCompileShader);
    RENDERER_GL_FUNC(glDeleteShader);
    RENDERER_GL_FUNC(glGetShaderiv);
    RENDERER_GL_FUNC(glGetShaderInfoLog);

    RENDERER_GL_FUNC(glCreateProgram);
    RENDERER_GL_FUNC(glAttachShader);
    RENDERER_GL_FUNC(glDetachShader);
    RENDERER_GL_FUNC(glLinkProgram);
    RENDERER_GL_FUNC(glUseProgram);
    RENDERER_GL_FUNC(glDeleteProgram);
    RENDERER_GL_FUNC(glGetProgramiv);
    RENDERER_GL_FUNC(glGetProgramInfoLog);

    RENDERER_GL_FUNC(glGenVertexArrays);
    RENDERER_GL_FUNC(glDeleteVertexArrays);
    RENDERER_GL_FUNC(glBindVertexArray);

    RENDERER_GL_FUNC(glGenBuffers);
    RENDERER_GL_FUNC(glDeleteBuffers);
    RENDERER_GL_FUNC(glBindBuffer);
    RENDERER_GL_FUNC(glBufferData);
    RENDERER_GL_FUNC(glBufferSubData);

    RENDERER_GL_FUNC(glGetUniformLocation);
    RENDERER_GL_FUNC(glUniform4fv);
    RENDERER_GL_FUNC(glUniformMatrix4fv);

    RENDERER_GL_FUNC(glGetAttribLocation);
    RENDERER_GL_FUNC(glVertexAttribPointer);
    RENDERER_GL_FUNC(glEnableVertexAttribArray);

    RENDERER_GL_FUNC(glDrawArrays);

    if (SDL_GL_ExtensionSupported("GL_ARB_debug_output"))
    {
        RENDERER_GL_FUNC(glDebugMessageCallbackARB);
    }

    return true;
}
#undef RENDERER_GL_FUNC
