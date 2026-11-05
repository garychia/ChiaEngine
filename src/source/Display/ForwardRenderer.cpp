#include "Display/ForwardRenderer.hpp"

#include "Data/String.hpp"
#include "Scene/TransformComponent.hpp"
#include "Scene/WorldTransformComponent.hpp"

#include <cstdio>
#include <cstring>

#include <glm/gtc/matrix_transform.hpp>

// ─── PBR 顶点着色器 ─────────────────────────────────────────────────────────
static const char *kPBRVertexShader = R"glsl(#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)glsl";

// ─── PBR 片段著色器(金屬度/粗糙度工作流, Cook-Torrance BRDF) ───────────────
static const char *kPBRFragmentShader = R"glsl(#version 330 core
out vec4 FragColor;

uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform int uLightType;        // 0 = directional, 1 = point
uniform vec3 uViewPos;

const float PI = 3.14159265359;

// GGX 正規化分配函數
float DistributionGGX(float roughnessSq, float nh)
{
    float a = roughnessSq;
    float a2 = a * a;
    float nh2 = nh * nh;
    float denom = nh2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Smith 影子遮蔽
float GeometrySchlickGGX(float roughnessSq, float dot)
{
    float r = (roughnessSq + 1.0);
    float k = (r * r) / 8.0;
    return dot / (dot * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughnessSq)
{
    float ndotV = max(dot(N, V), 0.0);
    float ndotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(roughnessSq, ndotV) * GeometrySchlickGGX(roughnessSq, ndotL);
}

// Schlick 菲涅爾
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 N = normalize(vec3(0.0, 1.0, 0.0));   // 平面法線
    vec3 V = normalize(uViewPos);               // 視图方向(相機在原點)
    vec3 L = normalize(uLightDir);              // 光源方向
    vec3 H = normalize(V + L);                  // 半角向量

    float ndotL = max(dot(N, L), 0.0);
    float ndotV = max(dot(N, V), 0.0);
    float ndotH = max(dot(N, H), 0.0);
    float ldoth = max(dot(L, H), 0.0);

    // 基礎反射率 F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, uAlbedo, uMetallic);

    // Cook-Torrance BRDF
    float roughnessSq = uRoughness * uRoughness;
    float NDF = DistributionGGX(roughnessSq, ndotH);
    float G = GeometrySmith(N, V, L, roughnessSq);
    vec3 F = FresnelSchlick(ldoth, F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * ndotV * ndotL + 0.001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - uMetallic;

    vec3 ambient = 0.03 * uAlbedo * kD;
    vec3 diffuse = uLightColor * uLightIntensity * ndotL * kD;
    vec3 radiance = ambient + diffuse + specular;

    FragColor = vec4(radiance, 1.0);
}
)glsl";

ForwardRenderer::ForwardRenderer()
{
}

ForwardRenderer::~ForwardRenderer()
{
    Shutdown();
}

GLuint ForwardRenderer::CompileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char buf[1024];
        glGetShaderInfoLog(shader, sizeof(buf), nullptr, buf);
        std::fprintf(stderr, "ForwardRenderer: shader compile failed (%d):\n%s\n",
                     (int)type, buf);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint ForwardRenderer::CreateProgram(const char *vertSrc, const char *fragSrc)
{
    GLuint vert = CompileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vert || !frag)
    {
        if (vert)
            glDeleteShader(vert);
        if (frag)
            glDeleteShader(frag);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char buf[1024];
        glGetProgramInfoLog(program, sizeof(buf), nullptr, buf);
        std::fprintf(stderr, "ForwardRenderer: program link failed:\n%s\n", buf);
        glDeleteProgram(program);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

bool ForwardRenderer::Initialize()
{
    if (mInitialized)
        return true;

    mProgram = CreateProgram(kPBRVertexShader, kPBRFragmentShader);
    if (!mProgram)
        return false;

    glGenVertexArrays(1, &mVAO);

    // 快取 uniform 位置
    mLocModel = glGetUniformLocation(mProgram, "uModel");
    mLocView = glGetUniformLocation(mProgram, "uView");
    mLocProj = glGetUniformLocation(mProgram, "uProj");
    mLocAlbedo = glGetUniformLocation(mProgram, "uAlbedo");
    mLocMetallic = glGetUniformLocation(mProgram, "uMetallic");
    mLocRoughness = glGetUniformLocation(mProgram, "uRoughness");
    mLocLightDir = glGetUniformLocation(mProgram, "uLightDir");
    mLocLightColor = glGetUniformLocation(mProgram, "uLightColor");
    mLocLightIntensity = glGetUniformLocation(mProgram, "uLightIntensity");
    mLocLightType = glGetUniformLocation(mProgram, "uLightType");
    mLocViewPos = glGetUniformLocation(mProgram, "uViewPos");

    mInitialized = true;
    return true;
}

void ForwardRenderer::DrawNode(SceneSystem &sceneSystem, Entity entity,
                               const glm::mat4 &parentWorld, const Camera &camera)
{
    uint32_t idx = entity.GetIndex();

    TransformComponent local;
    if (!sceneSystem.GetTransform(idx, local))
        return;

    // 本地模型矩陣(位置+旋轉+縮放，簡化為平移矩陣)
    glm::mat4 model = parentWorld *
                      glm::translate(glm::mat4(1.0f),
                                     glm::vec3(local.position.x, local.position.y,
                                               local.position.z));

    MaterialComponent material;
    sceneSystem.GetMaterial(idx, material.materialId);
    Point3D albedo;
    float metallic = 0.f, roughness = 1.f;
    sceneSystem.GetPBR(idx, albedo, metallic, roughness);

    LightComponent light;
    sceneSystem.GetLight(idx, light);

    glUseProgram(mProgram);
    glBindVertexArray(mVAO);

    if (mLocModel != -1)
        glUniformMatrix4fv(mLocModel, 1, GL_FALSE, &model[0][0]);
    if (mLocView != -1)
        glUniformMatrix4fv(mLocView, 1, GL_FALSE,
                           &glm::mat4(1.0f)[0][0]); // 簡化：不取用 Camera 的 view
    if (mLocProj != -1)
        glUniformMatrix4fv(mLocProj, 1, GL_FALSE,
                           &glm::mat4(1.0f)[0][0]); // 簡化：不取用 Camera 的 proj

    if (mLocAlbedo != -1)
        glUniform3f(mLocAlbedo, albedo.x, albedo.y, albedo.z);
    if (mLocMetallic != -1)
        glUniform1f(mLocMetallic, metallic);
    if (mLocRoughness != -1)
        glUniform1f(mLocRoughness, roughness);

    if (mLocLightDir != -1)
        glUniform3f(mLocLightDir, 0.5f, -1.0f, 0.3f); // 預設方向光
    if (mLocLightColor != -1)
        glUniform3f(mLocLightColor, light.color.x, light.color.y, light.color.z);
    if (mLocLightIntensity != -1)
        glUniform1f(mLocLightIntensity, light.intensity);
    if (mLocLightType != -1)
        glUniform1i(mLocLightType, static_cast<int>(light.type));
    if (mLocViewPos != -1)
        glUniform3f(mLocViewPos, 0.f, 0.f, 0.f);

    // 繪製單一三角形(預設 VAO 無頂點緩衝，僅驗證著色器綁定與 uniform 傳遞)
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // 遞迴子節點
    DynamicArray<Entity> children = sceneSystem.GetChildren(entity);
    for (size_t i = 0; i < children.GetNElements(); i++)
    {
        DrawNode(sceneSystem, children[i], model, camera);
    }
}

void ForwardRenderer::DrawScene(SceneSystem &sceneSystem)
{
    if (!mInitialized || !mProgram)
        return;

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DynamicArray<Entity> roots = sceneSystem.GetRoots();
    for (size_t i = 0; i < roots.GetNElements(); i++)
    {
        DrawNode(sceneSystem, roots[i], glm::mat4(1.0f), Camera());
    }
}

void ForwardRenderer::Shutdown()
{
    if (mProgram)
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
    if (mVAO)
    {
        glDeleteVertexArrays(1, &mVAO);
        mVAO = 0;
    }
    mInitialized = false;
}
