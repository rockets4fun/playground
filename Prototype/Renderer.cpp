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

// Structs declared in implementaion file because we do not want to
// expose any OpenGL implementation details in header

// -------------------------------------------------------------------------------------------------
struct Renderer::PrivateFuncs
{
    // Basic functions
    PFNGLENABLEPROC     glEnable = nullptr;
    PFNGLDISABLEPROC    glDisable = nullptr;
    PFNGLCULLFACEPROC   glCullFace = nullptr;
    PFNGLFINISHPROC     glFinish = nullptr;
    PFNGLBLENDFUNCPROC  glBlendFunc = nullptr;
    PFNGLCLEARCOLORPROC glClearColor = nullptr;
    PFNGLCLEARPROC      glClear = nullptr;
    PFNGLVIEWPORTPROC   glViewport = nullptr;
    PFNGLDEPTHMASKPROC  glDepthMask = nullptr;
    // Texturing
    PFNGLGENTEXTURESPROC    glGenTextures = nullptr;
    PFNGLDELETETEXTURESPROC glDeleteTextures = nullptr;
    PFNGLBINDTEXTUREPROC    glBindTexture = nullptr;
    PFNGLTEXPARAMETERFPROC  glTexParameterf = nullptr;
    PFNGLTEXPARAMETERIPROC  glTexParameteri = nullptr;
    PFNGLTEXIMAGE2DPROC     glTexImage2D = nullptr;
    PFNGLACTIVETEXTUREPROC  glActiveTexture = nullptr;
    // Frame buffer object
    PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D = nullptr;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus = nullptr;
    // Render buffers
    PFNGLGENRENDERBUFFERSPROC    glGenRenderbuffers = nullptr;
    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
    PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
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
    PFNGLUNIFORM1IPROC          glUniform1i = nullptr;
    PFNGLUNIFORM4FVPROC         glUniform4fv = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv = nullptr;
    // Vertex shader attribute functions
    PFNGLGETATTRIBLOCATIONPROC        glGetAttribLocation = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray = nullptr;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
    // Drawing command functions
    PFNGLDRAWARRAYSPROC glDrawArrays = nullptr;
    // ARB_debug_output (extension to 3.2 core)
    PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = nullptr;
};

// -------------------------------------------------------------------------------------------------
struct Renderer::PrivateState
{
    GLuint defVao = 0;

    GLuint defFbo = 0;
    GLuint colorTex = 0;
    GLuint depthTex = 0;

    u64 defaultProgramHandle      = 0;
    u64 emissionProgramHandle     = 0;
    u64 emissionPostProgramHandle = 0;

    std::map< u32, PrivateMesh > meshes;
};

// -------------------------------------------------------------------------------------------------
struct Renderer::PrivateHelpers
{
    typedef void (APIENTRYP GetProc)
        (GLuint shader, GLenum pname, GLint *params);
    typedef void (APIENTRYP InfoLogProc)
        (GLuint object, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

    PrivateHelpers(PrivateFuncs *funcs);

    bool updateShader(GLuint &shader, GLenum type, const std::string &source);
    bool updateProgram(GLuint &program, GLuint vertexShader, GLuint fragmentShader);

    void printInfoLog(GLuint object, GetProc getProc, InfoLogProc infoLogProc);

    void updateVertexBufferData(GLuint vbo,
            const std::vector< glm::fvec3 > &data, GLenum usage, int prevVertexCount = 0);

private:
    PrivateFuncs *funcs = nullptr;
};

// -------------------------------------------------------------------------------------------------
struct Renderer::PrivateMesh
{
    enum Flag
    {
        DYNAMIC = 0x1,
        DIRTY   = 0x2
    };

    Assets::Model *model = nullptr;
    const Assets::Info *assetInfo = nullptr;

    GLuint positionsVbo = 0;
    GLuint normalsVbo   = 0;
    GLuint diffuseVbo = 0;
    GLuint ambientVbo = 0;

    GLenum usage = GL_STATIC_DRAW;

    int vertexCount = 0;
    u32 flags = 0;
};

// -------------------------------------------------------------------------------------------------
u64 Renderer::Program::TYPE = 0;
u64 Renderer::Program::Info::STATE = 0;
struct Renderer::Program::PrivateInfo
{
    static u64 STATE;
    const Assets::Info *assetInfo = nullptr;
    u32 assetVersionLoaded = 0;
    // Shaders and program
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    GLuint program = 0;
    // Uniforms
    GLint uModelToWorldMatrix = 0;
    GLint uModelToViewMatrix = 0;
    GLint uProjectionMatrix = 0;
    GLint uDiffuseMul = 0;
    GLint uAmbientAdd = 0;
    GLint uRenderParams = 0;
    GLint uColorTex = 0;
    GLint uDepthTex = 0;
    // Attributes
    GLint aPosition = 0;
    GLint aNormal = 0;
    GLint aDiffuse = 0;
    GLint aAmbient = 0;
};
u64 Renderer::Program::PrivateInfo::STATE = 0;

// -------------------------------------------------------------------------------------------------
u64 Renderer::Mesh::TYPE = 0;
u64 Renderer::Mesh::Info::STATE = 0;
struct Renderer::Mesh::PrivateInfo
{
    static u64 STATE;
    // Store 1:n relation between mesh data from model ('PrivateMesh') and mesh instance
    PrivateMesh *privateMesh = nullptr;
};
u64 Renderer::Mesh::PrivateInfo::STATE = 0;

// -------------------------------------------------------------------------------------------------
u64 Renderer::Camera::TYPE = 0;
u64 Renderer::Camera::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
u64 Renderer::Transform::TYPE = 0;
u64 Renderer::Transform::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
Renderer::PrivateHelpers::PrivateHelpers(PrivateFuncs *funcs) : funcs(funcs)
{
}

// -------------------------------------------------------------------------------------------------
bool Renderer::PrivateHelpers::updateShader(
    GLuint &shader, GLenum type, const std::string &src)
{
    if (!shader)
    {
        shader = funcs->glCreateShader(type);
        if (!shader)
        {
            Logging::debug("ERROR: Failed to create shader object");
            return false;
        }
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
bool Renderer::PrivateHelpers::updateProgram(
    GLuint &program, GLuint vertexShader, GLuint fragmentShader)
{
    if (!program)
    {
        program = funcs->glCreateProgram();
        if (!program)
        {
            Logging::debug("ERROR: Failed to create program object");
            return false;
        }
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
void Renderer::registerTypesAndStates(StateDb &sdb)
{
    Program::TYPE = sdb.registerType("Program", 16);
    Program::Info::STATE = sdb.registerState(
        Program::TYPE, "Info", sizeof(Program::Info));
    Program::PrivateInfo::STATE = sdb.registerState(
        Program::TYPE, "PrivateInfo", sizeof(Program::PrivateInfo));

    Mesh::TYPE = sdb.registerType("Mesh", 4096);
    Mesh::Info::STATE = sdb.registerState(
        Mesh::TYPE, "Info", sizeof(Mesh::Info));
    Mesh::PrivateInfo::STATE = sdb.registerState(
        Mesh::TYPE, "PrivateInfo", sizeof(Mesh::PrivateInfo));

    Camera::TYPE = sdb.registerType("Camera", 8);
    Camera::Info::STATE = sdb.registerState(
        Camera::TYPE, "Info", sizeof(Camera::Info));

    Transform::TYPE = sdb.registerType("Transform", 4096);
    Transform::Info::STATE = sdb.registerState(
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

    funcs->glGenVertexArrays(1, &state->defVao);
    funcs->glBindVertexArray(state->defVao);

    // Set up frame buffer for playing around...
    {
        // Color texture
        funcs->glGenTextures(1, &state->colorTex);
        funcs->glBindTexture(GL_TEXTURE_2D, state->colorTex);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                  400, 225, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        funcs->glBindTexture(GL_TEXTURE_2D, 0);
        // Depth texture
        funcs->glGenTextures(1, &state->depthTex);
        funcs->glBindTexture(GL_TEXTURE_2D, state->depthTex);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                  400, 225, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
        funcs->glBindTexture(GL_TEXTURE_2D, 0);
        // Frame buffer
        funcs->glGenFramebuffers(1, &state->defFbo);
        funcs->glBindFramebuffer(GL_FRAMEBUFFER, state->defFbo);
        funcs->glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state->colorTex, 0);
        funcs->glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, state->depthTex, 0);
        if (funcs->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Logging::debug("ERROR: Default FBO is incomplete");
        }
        funcs->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Program::Info *defaultProgram = sdb.create< Program::Info >(state->defaultProgramHandle);
    defaultProgram->programAsset = assets.asset("Assets/Programs/Default.program");
    assets.refProgram(defaultProgram->programAsset);

    Program::Info *emissionProgram = sdb.create< Program::Info >(state->emissionProgramHandle);
    emissionProgram->programAsset = assets.asset("Assets/Programs/Emission.program");
    assets.refProgram(emissionProgram->programAsset);

    Program::Info *emissionPostProgram = sdb.create< Program::Info >(state->emissionPostProgramHandle);
    emissionPostProgram->programAsset = assets.asset("Assets/Programs/EmissionPost.program");
    assets.refProgram(emissionPostProgram->programAsset);

    funcs->glEnable(GL_DEPTH_TEST);

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::shutdown(StateDb &sdb)
{
    if (state->defVao) funcs->glDeleteVertexArrays(1, &state->defVao);

    auto programsPrivate = sdb.stateAll< Program::PrivateInfo >();
    for (auto programPrivate : programsPrivate)
    {
        if (programPrivate->program)        funcs->glDeleteProgram(programPrivate->program);
        if (programPrivate->fragmentShader) funcs->glDeleteShader (programPrivate->fragmentShader);
        if (programPrivate->vertexShader)   funcs->glDeleteShader (programPrivate->vertexShader);
    }

    for (auto &mapIter : state->meshes)
    {
        funcs->glDeleteBuffers(3, &mapIter.second.positionsVbo);
    }

    helpers = nullptr;
    state = nullptr;
    funcs = nullptr;
}

// -------------------------------------------------------------------------------------------------
void Renderer::update(StateDb &sdb, Assets &assets, Renderer &renderer, double deltaTimeInS)
{
    PROFILING_SECTION(Renderer, glm::fvec3(0.0f, 0.0f, 1.0f))
    {
        PROFILING_SECTION(FinishBegin, glm::fvec3(1.0f, 0.5f, 0.0f))
        funcs->glFinish();
    }

    // Prepare/update programs
    auto programs = sdb.stateAll< Program::Info >();
    auto programsPrivate = sdb.stateAll< Program::PrivateInfo >();
    for (auto programPrivate : programsPrivate)
    {
        if (!programPrivate->assetInfo)
        {
            auto program = programs.rel(programsPrivate, programPrivate);
            programPrivate->assetInfo = assets.assetInfo(program->programAsset);
            COMMON_ASSERT(programPrivate->assetInfo)
        }
        u32 assetVersion = programPrivate->assetInfo->version;
        if (programPrivate->assetVersionLoaded != assetVersion)
        {
            if (assetVersion > 1)
            {
                funcs->glUseProgram(0);
                funcs->glDetachShader(programPrivate->program, programPrivate->fragmentShader);
                funcs->glDetachShader(programPrivate->program, programPrivate->vertexShader);
            }
            Assets::Program *programAsset = assets.refProgram(programPrivate->assetInfo->hash);
            helpers->updateShader(programPrivate->vertexShader, GL_VERTEX_SHADER,
                programAsset->sourceByType[Assets::Program::VERTEX_SHADER]);
            helpers->updateShader(programPrivate->fragmentShader, GL_FRAGMENT_SHADER,
                programAsset->sourceByType[Assets::Program::FRAGMENT_SHADER]);
            helpers->updateProgram(programPrivate->program,
                programPrivate->vertexShader, programPrivate->fragmentShader);

            // TODO(martinmo): Use Uniform Buffer Objects to pass uniforms to shaders

            programPrivate->uModelToWorldMatrix =
                funcs->glGetUniformLocation(programPrivate->program, "ModelToWorldMatrix");
            programPrivate->uModelToViewMatrix =
                funcs->glGetUniformLocation(programPrivate->program, "ModelToViewMatrix");
            programPrivate->uProjectionMatrix =
                funcs->glGetUniformLocation(programPrivate->program, "ProjectionMatrix");
            programPrivate->uDiffuseMul =
                funcs->glGetUniformLocation(programPrivate->program, "DiffuseMul");
            programPrivate->uAmbientAdd =
                funcs->glGetUniformLocation(programPrivate->program, "AmbientAdd");
            programPrivate->uRenderParams =
                funcs->glGetUniformLocation(programPrivate->program, "RenderParams");
            programPrivate->uColorTex =
                funcs->glGetUniformLocation(programPrivate->program, "ColorTex");
            programPrivate->uDepthTex =
                funcs->glGetUniformLocation(programPrivate->program, "DepthTex");

            // TODO(martinmo): Use 'glBindAttribLocation()' before linking the program to force
            // TODO(martinmo): assignment of attributes to specific fixed locations
            // TODO(martinmo): (e.g. position always 0, color always 1 etc.)
            // TODO(martinmo): ==> This allows us to use the same VAO for different shader programs

            programPrivate->aPosition =
                funcs->glGetAttribLocation(programPrivate->program, "Position");
            programPrivate->aNormal =
                funcs->glGetAttribLocation(programPrivate->program, "Normal");
            programPrivate->aDiffuse =
                funcs->glGetAttribLocation(programPrivate->program, "Diffuse");
            programPrivate->aAmbient =
                funcs->glGetAttribLocation(programPrivate->program, "Ambient");

            programPrivate->assetVersionLoaded = assetVersion;
        }
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
            privateMesh->assetInfo = assets.assetInfo(modelAsset);
            COMMON_ASSERT(privateMesh->assetInfo);
            // TODO(martinmo): Add way of getting asset and flags in one call/lookup
            if (privateMesh->assetInfo->flags & Assets::Flag::DYNAMIC)
            {
                privateMesh->flags |= PrivateMesh::Flag::DYNAMIC;
                privateMesh->usage = GL_DYNAMIC_DRAW;
            }
            funcs->glGenBuffers(1, &privateMesh->positionsVbo);
            funcs->glGenBuffers(1, &privateMesh->normalsVbo);
            funcs->glGenBuffers(1, &privateMesh->diffuseVbo);
            funcs->glGenBuffers(1, &privateMesh->ambientVbo);
            privateMesh->flags |= PrivateMesh::Flag::DIRTY;
        }
        else if (privateMesh->flags & PrivateMesh::Flag::DYNAMIC)
        {
            privateMesh->flags |= PrivateMesh::Flag::DIRTY;
        }
    }

    auto defaultProgram      = sdb.state< Program::PrivateInfo >(state->defaultProgramHandle);
    auto emissionProgram     = sdb.state< Program::PrivateInfo >(state->emissionProgramHandle);
    auto emissionPostProgram = sdb.state< Program::PrivateInfo >(state->emissionPostProgramHandle);

    // Default render pass
    {
        const float aspect = 16.0f / 9.0f;
        glm::fmat4 projection = glm::perspective(
                    glm::radians(30.0f * aspect), aspect, 0.5f, 200.0f);
        glm::fmat4 worldToView;
        if (activeCameraHandle)
        {
            auto camera = sdb.state< Camera::Info >(activeCameraHandle);
            worldToView = glm::lookAt(
                camera->position, camera->target, glm::fvec3(0.0f, 0.0f, 1.0f));
        }
        glm::fvec4 renderParams = glm::fvec4(debugNormals ? 1.0f : 0.0f, 0.0, 0.0, 0.0);

        funcs->glClearColor(0.15f, 0.15f, 0.15f, 1.0);
        funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderPass(sdb, Group::DEFAULT, defaultProgram, projection, worldToView, renderParams);

        /*
        // Render transparent items with Z-writes off
        funcs->glEnable(GL_CULL_FACE);
        funcs->glDepthMask(GL_FALSE);
        funcs->glEnable(GL_BLEND);
        funcs->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        renderPass(sdb, Group::DEFAULT_TRANSPARENT, defaultProgram, projection, worldToView, renderParams);
        funcs->glDisable(GL_BLEND);
        funcs->glDepthMask(GL_TRUE);
        funcs->glDisable(GL_CULL_FACE);
        */

        // Now render ambient as emissive into half-res FBO (will be blurred later...)
        funcs->glBindFramebuffer(GL_FRAMEBUFFER, state->defFbo);
        funcs->glClearColor(0.0f, 0.0f, 0.0f, 1.0);
        funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        funcs->glViewport(0, 0, 400, 225);
        renderPass(sdb, Group::DEFAULT | Group::DEFAULT_TRANSPARENT, emissionProgram, projection, worldToView, renderParams);
        funcs->glViewport(0, 0, 800, 450);
        funcs->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Emission post-processing pass
    if (!debugNormals)
    {
        glm::fmat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 450.0f, -10.0f, 10.0f);
        glm::fmat4 worldToView;
        glm::fvec4 renderParams = glm::fvec4(debugNormals ? 1.0f : 0.0f, 1.0, 0.0, 0.0);

        funcs->glUseProgram(emissionPostProgram->program);
        GLint colorTex = 0; GLint depthTex = 1;
        if (emissionPostProgram->uColorTex >= 0) funcs->glUniform1i(emissionPostProgram->uColorTex, colorTex);
        if (emissionPostProgram->uDepthTex >= 0) funcs->glUniform1i(emissionPostProgram->uDepthTex, depthTex);

        funcs->glEnable(GL_BLEND);
        funcs->glBlendFunc(GL_ONE, GL_ONE);
        funcs->glActiveTexture(GL_TEXTURE0); funcs->glBindTexture(GL_TEXTURE_2D, state->colorTex);
        funcs->glActiveTexture(GL_TEXTURE1); funcs->glBindTexture(GL_TEXTURE_2D, state->depthTex);
        for (int repetitionIdx = 0; repetitionIdx < 1; ++repetitionIdx)
        {
            renderPass(sdb, Group::DEFAULT_POST, emissionPostProgram, projection, worldToView, renderParams);
        }
        funcs->glActiveTexture(GL_TEXTURE1); funcs->glBindTexture(GL_TEXTURE_2D, 0);
        funcs->glActiveTexture(GL_TEXTURE0); funcs->glBindTexture(GL_TEXTURE_2D, 0);
        funcs->glDisable(GL_BLEND);
    }

    // UI render pass
    {
        glm::fmat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 450.0f, -10.0f, 10.0f);
        glm::fmat4 worldToView;
        glm::fvec4 renderParams = glm::fvec4(debugNormals ? 1.0f : 0.0f, 1.0, 0.0, 0.0);

        funcs->glClear(GL_DEPTH_BUFFER_BIT);
        renderPass(sdb, Group::DEFAULT_UI, defaultProgram, projection, worldToView, renderParams);
    }

    // Seems like 'glFinish()' implicitly waits for v-sync on OS X
#ifndef COMMON_OSX
    {
        PROFILING_SECTION(FinishEnd, glm::fvec3(1.0f, 0.5f, 0.0f))
        funcs->glFinish();
    }
#endif
}

// -------------------------------------------------------------------------------------------------
void Renderer::renderPass(StateDb &sdb, u32 renderMask, const Program::PrivateInfo *program,
    const glm::fmat4 &projection, const glm::fmat4 &worldToView, const glm::fvec4 &renderParams)
{
    funcs->glUseProgram(program->program);

    glm::fvec4 defaultDiffuseMul(1.0f, 1.0f, 1.0f, 1.0f);
    glm::fvec4 defaultAmbientAdd(0.0f, 0.0f, 0.0f, 0.0f);

    funcs->glUniform4fv(program->uRenderParams, 1, glm::value_ptr(renderParams));
    funcs->glUniformMatrix4fv(program->uProjectionMatrix, 1, GL_FALSE, glm::value_ptr(projection));

    auto meshes = sdb.stateAll< Mesh::Info >();
    auto meshesPrivate = sdb.stateAll< Mesh::PrivateInfo >();

    // Pseudo-instanced rendering of meshes
    for (auto mesh : meshes)
    {
        if (mesh->flags & Mesh::Flag::HIDDEN)
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
            helpers->updateVertexBufferData(privateMesh->diffuseVbo,
                privateMesh->model->diffuse, privateMesh->usage, oldVertexCount);
            helpers->updateVertexBufferData(privateMesh->ambientVbo,
                privateMesh->model->ambient, privateMesh->usage, oldVertexCount);
            privateMesh->flags &= ~PrivateMesh::Flag::DIRTY;
        }
        if (!privateMesh->vertexCount)
        {
            continue;
        }

        // Vertex attribute assignments are stored inside the bound VAO
        // ==> Think about creating one VAO per renderable mesh/program combo?
        {
            if (program->aPosition >= 0)
            {
                funcs->glBindBuffer(GL_ARRAY_BUFFER, privateMesh->positionsVbo);
                funcs->glVertexAttribPointer(program->aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
                funcs->glEnableVertexAttribArray(program->aPosition);
            }
            if (program->aNormal >= 0)
            {
                funcs->glBindBuffer(GL_ARRAY_BUFFER, privateMesh->normalsVbo);
                funcs->glVertexAttribPointer(program->aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
                funcs->glEnableVertexAttribArray(program->aNormal);
            }
            if (program->aDiffuse >= 0)
            {
                funcs->glBindBuffer(GL_ARRAY_BUFFER, privateMesh->diffuseVbo);
                funcs->glVertexAttribPointer(program->aDiffuse, 3, GL_FLOAT, GL_FALSE, 0, 0);
                funcs->glEnableVertexAttribArray(program->aDiffuse);
            }
            if (program->aAmbient >= 0)
            {
                funcs->glBindBuffer(GL_ARRAY_BUFFER, privateMesh->ambientVbo);
                funcs->glVertexAttribPointer(program->aAmbient, 3, GL_FLOAT, GL_FALSE, 0, 0);
                funcs->glEnableVertexAttribArray(program->aAmbient);
            }
        }

        glm::fmat4 modelToWorld;
        modelToWorld = glm::translate(glm::fmat4(), mesh->translation);
        modelToWorld = modelToWorld * glm::mat4_cast(mesh->rotation);
        if (mesh->flags & Mesh::Flag::SCALED)
        {
            modelToWorld = glm::scale(modelToWorld, glm::fvec3(mesh->uniformScale));
        }
        funcs->glUniformMatrix4fv(
            program->uModelToWorldMatrix, 1, GL_FALSE, glm::value_ptr(modelToWorld));

        glm::fmat4 modelToView = worldToView * modelToWorld;
        funcs->glUniformMatrix4fv(
            program->uModelToViewMatrix, 1, GL_FALSE, glm::value_ptr(modelToView));

        glm::fvec4 diffuseMul = defaultDiffuseMul;
        if (mesh->flags & Mesh::Flag::DIFFUSE_MUL) diffuseMul = mesh->diffuseMul;
        funcs->glUniform4fv(program->uDiffuseMul, 1, glm::value_ptr(diffuseMul));

        glm::fvec4 ambientAdd = defaultAmbientAdd;
        if (mesh->flags & Mesh::Flag::AMBIENT_ADD) ambientAdd = mesh->ambientAdd;
        funcs->glUniform4fv(program->uAmbientAdd, 1, glm::value_ptr(ambientAdd));

        funcs->glDrawArrays(GL_TRIANGLES, 0, privateMesh->vertexCount);

        {
            if (program->aPosition  >= 0) funcs->glEnableVertexAttribArray(program->aPosition);
            if (program->aNormal    >= 0) funcs->glEnableVertexAttribArray(program->aNormal);
            if (program->aDiffuse >= 0) funcs->glEnableVertexAttribArray(program->aDiffuse);
            if (program->aAmbient  >= 0) funcs->glEnableVertexAttribArray(program->aAmbient);
        }
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
    RENDERER_GL_FUNC(glDisable);
    RENDERER_GL_FUNC(glCullFace);
    RENDERER_GL_FUNC(glFinish);
    RENDERER_GL_FUNC(glBlendFunc);
    RENDERER_GL_FUNC(glClearColor);
    RENDERER_GL_FUNC(glClear);
    RENDERER_GL_FUNC(glViewport);
    RENDERER_GL_FUNC(glDepthMask);

    RENDERER_GL_FUNC(glGenTextures);
    RENDERER_GL_FUNC(glDeleteTextures);
    RENDERER_GL_FUNC(glBindTexture);
    RENDERER_GL_FUNC(glTexParameterf);
    RENDERER_GL_FUNC(glTexParameteri);
    RENDERER_GL_FUNC(glTexImage2D);
    RENDERER_GL_FUNC(glActiveTexture);

    RENDERER_GL_FUNC(glGenFramebuffers);
    RENDERER_GL_FUNC(glDeleteFramebuffers);
    RENDERER_GL_FUNC(glBindFramebuffer);
    RENDERER_GL_FUNC(glFramebufferTexture2D);
    RENDERER_GL_FUNC(glFramebufferRenderbuffer);
    RENDERER_GL_FUNC(glCheckFramebufferStatus);

    RENDERER_GL_FUNC(glGenRenderbuffers);
    RENDERER_GL_FUNC(glDeleteRenderbuffers);
    RENDERER_GL_FUNC(glRenderbufferStorage);

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
    RENDERER_GL_FUNC(glUniform1i);
    RENDERER_GL_FUNC(glUniform4fv);
    RENDERER_GL_FUNC(glUniformMatrix4fv);

    RENDERER_GL_FUNC(glGetAttribLocation);
    RENDERER_GL_FUNC(glVertexAttribPointer);
    RENDERER_GL_FUNC(glEnableVertexAttribArray);
    RENDERER_GL_FUNC(glDisableVertexAttribArray);

    RENDERER_GL_FUNC(glDrawArrays);

    if (SDL_GL_ExtensionSupported("GL_ARB_debug_output"))
    {
        RENDERER_GL_FUNC(glDebugMessageCallbackARB);
    }

    return true;
}
#undef RENDERER_GL_FUNC
