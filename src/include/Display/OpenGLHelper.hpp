#ifndef OPENGL_HELPER_HPP
#define OPENGL_HELPER_HPP

// DEPRECATED — OpenGL backend helper. Used only by OpenGLRenderer (itself
// deprecated); kept as the legacy non-Vulkan CMake fallback. See
// docs/agents/opengl-backend-assessment.md.

#include "Data/String.hpp"
#include "Data/DynamicArray.hpp"
#include "Display/Color.hpp"
#include "Geometry/3D/Point3D.hpp"

#ifdef OPENGL_ENABLED
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

#include <cstdint>

class OpenGLHelper
{
  public:
    static bool InitGLFW();
    static void CreateWindow(GLFWwindow *&pWindow, int width, int height, const char *title);
    static void DestroyWindow(GLFWwindow *&pWindow);

    static GLuint CompileShader(const String &filePath, GLenum shaderType);
    static GLuint LinkProgram(GLuint vertShader, GLuint fragShader);

    static GLuint CreateTexture(const String &filePath, int &outW, int &outH);

    static glm::mat4 BuildViewMatrix(const Point3D &pos, const Point3D &rot);
    static glm::mat4 BuildProjMatrix(float fovDeg, float aspect, float nearP, float farP);
    static glm::mat4 BuildWorldMatrix(const Point3D &pos, const Point3D &rot, const Point3D &scale);
};

#endif