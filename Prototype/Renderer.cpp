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

#include "Platform.hpp"
#include "StateDb.hpp"
#include "Assets.hpp"

// Include file containing all OpenGL declarations and definitions
// --> This will never be included outside of this class
#include "CoreGl/glcorearb.h"

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::GlFuncs
{
    // Basic functions
    PFNGLENABLEPROC glEnable = nullptr;
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
struct Renderer::GlMesh
{
    GLuint positionsVbo = 0;
    GLuint normalsVbo = 0;
    GLsizei vertexCount = 0;
};

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::GlState
{
    GLuint defVs = 0;
    GLuint defFs = 0;
    GLuint defProg = 0;

    GLint defProgUniformModelViewMatrix;
    GLint defProgUniformProjectionMatrix;

    GLint defProgAttribPosition;
    GLint defProgAttribNormal;

    GLuint defVao = 0;

    GLuint cubePositionsVbo;
    GLuint cubeNormalsVbo;

    std::map< u32, GlMesh > meshes;
};

// Declared here because we do not want to expose OpenGL implementation details in header
struct Renderer::GlHelpers
{
    typedef void (APIENTRYP GetProc)
        (GLuint shader, GLenum pname, GLint *params);
    typedef void (APIENTRYP InfoLogProc)
        (GLuint object, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

    GlHelpers(GlFuncs *funcs);

    bool createAndCompileShader(GLuint &shader, GLenum type, const std::string &source);
    bool createAndLinkSimpleProgram(GLuint &program, GLuint vertexShader, GLuint fragmentShader);
    void printInfoLog(GLuint object, GetProc getProc, InfoLogProc infoLogProc);

private:
    GlFuncs *funcs = nullptr;
};

u64 Renderer::Mesh::TYPE = 0;
u64 Renderer::Mesh::Info::STATE = 0;
u64 Renderer::Camera::TYPE = 0;
u64 Renderer::Camera::Info::STATE = 0;

// -------------------------------------------------------------------------------------------------
Renderer::GlHelpers::GlHelpers(GlFuncs *funcs) : funcs(funcs)
{
}

// -------------------------------------------------------------------------------------------------
bool Renderer::GlHelpers::createAndCompileShader(
    GLuint &shader, GLenum type, const std::string &src)
{
    shader = funcs->glCreateShader(type);
    if (!shader)
    {
        SDL_Log("ERROR: Failed to create shader object");
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
        SDL_Log("ERROR: Failed to compile shader");
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
bool Renderer::GlHelpers::createAndLinkSimpleProgram(
    GLuint &program, GLuint vertexShader, GLuint fragmentShader)
{
    program = funcs->glCreateProgram();
    if (!program)
    {
        SDL_Log("ERROR: Failed to create program object");
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
        SDL_Log("ERROR: Failed to link program");
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::GlHelpers::printInfoLog(GLuint object, GetProc getProc, InfoLogProc infoLogProc)
{
    // TODO(martinmo): Refactor into 'getInfoLog' and separate print
    GLint infoLogLength = 0;
    getProc(object, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 1)
    {
        std::string infoLog(infoLogLength, 'x');
        infoLogProc(object, GLsizei(infoLog.length()), nullptr, &infoLog[0]);
        SDL_Log("----------------------------------------");
        SDL_Log("%s", infoLog.c_str());
        SDL_Log("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    }
}

// -------------------------------------------------------------------------------------------------
void APIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const void *userParam)
{
    SDL_Log("GL: %s", message);
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

    Camera::TYPE = stateDb.registerType("Camera", 64);
    Camera::Info::STATE = stateDb.registerState(
        Camera::TYPE, "Info", sizeof(Camera::Info));
}

// -------------------------------------------------------------------------------------------------
bool Renderer::initialize(Platform &platform)
{
    funcs = std::make_shared< GlFuncs >();
    state = std::make_shared< GlState >();
    helpers = std::make_shared< GlHelpers >(funcs.get());

    if (!initializeGl())
    {
        return false;
    }

    if (SDL_GL_ExtensionSupported("GL_ARB_debug_output"))
    {
        funcs->glDebugMessageCallbackARB(debugMessageCallback, this);
        funcs->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    }

    std::string defVsSrc =
        "#version 150\n"
        "\n"
        "uniform mat4 ModelViewMatrix;\n"
        "uniform mat4 ProjectionMatrix;\n"
        "\n"
        "in vec4 Position;\n"
        "in vec4 Normal;\n"
        "\n"
        "out vec4 vertexColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vertexColor = abs(Normal);\n"
        "   gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(Position.xyz, 1.0);\n"
        "}\n";
    helpers->createAndCompileShader(state->defVs, GL_VERTEX_SHADER, defVsSrc);

    std::string defFsSrc =
        "#version 150\n"
        "\n"
        "in vec4 vertexColor;\n"
        "\n"
        "out vec4 fragmentColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    float scale = 1;//gl_FrontFacing ? 1.0 : 0.5;\n"
        "    fragmentColor = vec4(vertexColor.rgb * scale, 1.0);\n"
        "}\n";
    helpers->createAndCompileShader(state->defFs, GL_FRAGMENT_SHADER, defFsSrc);

    helpers->createAndLinkSimpleProgram(state->defProg, state->defVs, state->defFs);

    // TODO(martinmo): Use Uniform Buffer Objects to pass uniforms to shaders

    state->defProgUniformModelViewMatrix =
        funcs->glGetUniformLocation(state->defProg, "ModelViewMatrix");
    state->defProgUniformProjectionMatrix =
        funcs->glGetUniformLocation(state->defProg, "ProjectionMatrix");

    // TODO(martinmo): Use 'glBindAttribLocation()' before linking the program to force
    // TODO(martinmo): assignment of attributes to specific fixed locations
    // TODO(martinmo): (e.g. position always 0, color always 1 etc.)
    // TODO(martinmo): --> This allows us to use the same VAO for different shader programs

    state->defProgAttribPosition =
        funcs->glGetAttribLocation(state->defProg, "Position");
    state->defProgAttribNormal =
        funcs->glGetAttribLocation(state->defProg, "Normal");

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

    funcs->glGenBuffers(1, &state->cubePositionsVbo);
    funcs->glBindBuffer(GL_ARRAY_BUFFER, state->cubePositionsVbo);
    funcs->glBufferData(GL_ARRAY_BUFFER, sizeof(cubePositions), cubePositions, GL_STATIC_DRAW);

    funcs->glGenBuffers(1, &state->cubeNormalsVbo);
    funcs->glBindBuffer(GL_ARRAY_BUFFER, state->cubeNormalsVbo);
    funcs->glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNormals), cubeNormals, GL_STATIC_DRAW);

    funcs->glClearColor(0.15f, 0.15f, 0.15f, 1.0);
    funcs->glEnable(GL_DEPTH_TEST);

    return true;
}

// -------------------------------------------------------------------------------------------------
void Renderer::shutdown(Platform &platform)
{
    if (state->cubeNormalsVbo) funcs->glDeleteBuffers(1, &state->cubeNormalsVbo);
    if (state->cubePositionsVbo) funcs->glDeleteBuffers(1, &state->cubePositionsVbo);

    if (state->defVao) funcs->glDeleteVertexArrays(1, &state->defVao);

    if (state->defProg) funcs->glDeleteProgram(state->defProg);
    if (state->defFs) funcs->glDeleteShader(state->defFs);
    if (state->defVs) funcs->glDeleteShader(state->defVs);

    for (auto &meshIter : state->meshes)
    {
        // After restructuring 'meshes' into several vectors we could
        // actually use 'glDeleteBuffers' SIMD style...
        if (meshIter.second.positionsVbo) funcs->glDeleteBuffers(1, &meshIter.second.positionsVbo);
        if (meshIter.second.normalsVbo)   funcs->glDeleteBuffers(1, &meshIter.second.normalsVbo);
    }

    helpers = std::make_shared< GlHelpers >(funcs.get());
    state = std::make_shared< GlState >();
    funcs = std::make_shared< GlFuncs >();
}

// -------------------------------------------------------------------------------------------------
void Renderer::update(Platform &platform, double deltaTimeInS)
{
    funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = 16.0f / 9.0f;
    glm::fmat4 projection = glm::perspective(glm::radians(30.0f * aspect), aspect, 0.1f, 200.0f);
    funcs->glUniformMatrix4fv(
        state->defProgUniformProjectionMatrix, 1, GL_FALSE, glm::value_ptr(projection));

    glm::fmat4 view;
    if (activeCameraHandle)
    {
        Camera::Info *cameraInfo = nullptr;
        platform.stateDb.refState(Camera::Info::STATE, activeCameraHandle, &cameraInfo);
        view = glm::lookAt(cameraInfo->position.xyz(),
            cameraInfo->target.xyz(), glm::fvec3(0.0f, 0.0f, 1.0f));
    }

    glm::fmat4 model, modelView;

    // Pseudo-instanced rendering of meshes
    Mesh::Info *meshBegin = nullptr, *meshEnd = nullptr;
    platform.stateDb.refStateAll(Mesh::Info::STATE, &meshBegin, &meshEnd);
    for (Mesh::Info *mesh = meshBegin; mesh != meshEnd; ++mesh)
    {
        // TODO(MARTINMO): Replace map by array and store index in mesh private state
        GlMesh &glMesh = state->meshes[mesh->modelAsset];
        if (!glMesh.positionsVbo)
        {
            const Assets::Model *model = platform.assets.refModel(mesh->modelAsset);
            COMMON_ASSERT(model);
            if (!model->positions.empty())
            {
                funcs->glGenBuffers(1, &glMesh.positionsVbo);
                funcs->glBindBuffer(GL_ARRAY_BUFFER, glMesh.positionsVbo);
                funcs->glBufferData(GL_ARRAY_BUFFER,
                    model->positions.size() * sizeof(glm::fvec3),
                    &model->positions[0].x, GL_STATIC_DRAW);
                funcs->glGenBuffers(1, &glMesh.normalsVbo);
                funcs->glBindBuffer(GL_ARRAY_BUFFER, glMesh.normalsVbo);
                funcs->glBufferData(GL_ARRAY_BUFFER,
                    model->normals.size() * sizeof(glm::fvec3),
                    &model->normals[0].x, GL_STATIC_DRAW);
                glMesh.vertexCount = GLsizei(model->positions.size());
            }
        }

        // Vertex attribute assignments are stored inside the bound VAO
        // --> Think about creating one VAO per renderable mesh
        funcs->glBindBuffer(GL_ARRAY_BUFFER, glMesh.positionsVbo);
        funcs->glVertexAttribPointer(state->defProgAttribPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
        funcs->glEnableVertexAttribArray(state->defProgAttribPosition);
        funcs->glBindBuffer(GL_ARRAY_BUFFER, glMesh.normalsVbo);
        funcs->glVertexAttribPointer(state->defProgAttribNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
        funcs->glEnableVertexAttribArray(state->defProgAttribNormal);

        model = glm::translate(glm::fmat4(), mesh->translation.xyz());
        model = model * glm::mat4_cast(mesh->rotation);
        modelView = view * model;
        funcs->glUniformMatrix4fv(
            state->defProgUniformModelViewMatrix, 1, GL_FALSE, glm::value_ptr(modelView));

        funcs->glDrawArrays(GL_TRIANGLES, 0, glMesh.vertexCount);
    }
}

// -------------------------------------------------------------------------------------------------
#define RENDERER_GL_FUNC(Name) \
    { \
        funcs->Name = (decltype(funcs->Name))(SDL_GL_GetProcAddress(#Name)); \
        if (funcs->Name == nullptr) \
        { \
            SDL_Log("ERROR: Failed to resolve '%s'", #Name); \
            return false; \
        } \
    }
bool Renderer::initializeGl()
{
    RENDERER_GL_FUNC(glEnable);

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
