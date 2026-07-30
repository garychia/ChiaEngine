#include "Display/OpenGLHelper.hpp"
#include "System/Debug/Debug.hpp"
#include "System/IO/IO.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"

#ifdef OPENGL_ENABLED

bool OpenGLHelper::InitGLFW()
{
    if (!glfwInit())
    {
        PRINTLN_ERR("OpenGLHelper: failed to initialize GLFW.");
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    return true;
}

void OpenGLHelper::CreateWindow(GLFWwindow *&pWindow, int width, int height, const char *title)
{
    pWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!pWindow)
    {
        PRINTLN_ERR("OpenGLHelper: failed to create GLFW window.");
        return;
    }
    glfwMakeContextCurrent(pWindow);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        PRINTLN_ERR("OpenGLHelper: failed to initialize GLAD.");
        glfwDestroyWindow(pWindow);
        pWindow = nullptr;
        return;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void OpenGLHelper::DestroyWindow(GLFWwindow *&pWindow)
{
    if (pWindow)
    {
        glfwDestroyWindow(pWindow);
        pWindow = nullptr;
    }
    glfwTerminate();
}

static bool ReadFileToString(const String &path, DynamicArray<char> &buf)
{
    IO::File file;
    if (!file.Open(path, IO::FileAccessMode::Read))
    {
        PRINTLN_ERR("OpenGLHelper: cannot open shader file: " << path.CStr());
        return false;
    }
    const auto fileSize = file.Length();
    buf.Resize(fileSize + 1);
    file.Read(buf.begin(), fileSize);
    buf[fileSize] = '\0';
    file.Close();
    return true;
}

GLuint OpenGLHelper::CompileShader(const String &filePath, GLenum shaderType)
{
    DynamicArray<char> source;
    if (!ReadFileToString(filePath, source))
        return 0;

    const char *srcPtr = source.begin();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        PRINTLN_ERR("OpenGLHelper: shader compilation failed:\n" << infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint OpenGLHelper::LinkProgram(GLuint vertShader, GLuint fragShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        PRINTLN_ERR("OpenGLHelper: program linking failed:\n" << infoLog);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

GLuint OpenGLHelper::CreateTexture(const String &filePath, int &outW, int &outH)
{
    int channels = 0;
    unsigned char *data = stbi_load(filePath.CStr(), &outW, &outH, &channels, STBI_rgb_alpha);
    if (!data)
    {
        PRINTLN_ERR("OpenGLHelper: failed to load texture: " << filePath.CStr());
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outW, outH, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return tex;
}

glm::mat4 OpenGLHelper::BuildViewMatrix(const Point3D &pos, const Point3D &rot)
{
    glm::vec3 position(pos.X, pos.Y, pos.Z);
    glm::vec3 rotation(rot.X, rot.Y, rot.Z);
    glm::vec3 front;
    front.x = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
    front.y = sin(glm::radians(rotation.x));
    front.z = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
    return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 OpenGLHelper::BuildProjMatrix(float fovDeg, float aspect, float nearP, float farP)
{
    return glm::perspective(glm::radians(fovDeg), aspect, nearP, farP);
}

glm::mat4 OpenGLHelper::BuildWorldMatrix(const Point3D &pos, const Point3D &rot, const Point3D &scale)
{
    glm::mat4 world(1.0f);
    world = glm::translate(world, glm::vec3(pos.X, pos.Y, pos.Z));
    world = glm::rotate(world, glm::radians(rot.X), glm::vec3(1.0f, 0.0f, 0.0f));
    world = glm::rotate(world, glm::radians(rot.Y), glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, glm::radians(rot.Z), glm::vec3(0.0f, 0.0f, 1.0f));
    world = glm::scale(world, glm::vec3(scale.X, scale.Y, scale.Z));
    return world;
}

#endif // OPENGL_ENABLED