#ifndef RENDERER_MATH_HPP
#define RENDERER_MATH_HPP

// RendererMath — 渲染層唯一的矩陣慣例模組(issue #84)。
//
// 目的:消除「矩陣慣例複製 3 份」的歷史包袱 — BuildWorldMatrix/投影原本
// 分別複製在 VulkanRenderer.cpp(原標題「與 OpenGLHelper 一致」)、
// OpenGLHelper.hpp,且 ortho 在 RecordDrawCommands / RecordTextDrawCommands
// 各內聯一次。此模組是單一的、繼承 GL/NDC 慣例的權威實作,所有後端共用。
//
// 慣例(工程建立在 IMGui/OpenGL 慣例上,與 Camera 的 Euler rotation 同構):
//   - World:  T * R(deg→rad,順序 X→Y→Z) * S      (同 TransformComponent)
//   - View:   glm::lookAt(pos, pos+front, up), front 由 Euler 推算
//   - Proj:   glm::perspective(radians(fov), aspect, near, far)
//   - GUI:    orthoRH_ZO(-1,1, 1,-1, -1,1) — NDC 空間,GL y-up,深度=(1-z)/2
//             (zNear=-1, zFar=1 → 後加入的 layer/component 得較小 depth → 較近,
//              LESS depth test 下正確蓋住先畫的;y 對調讓 GL「頂端」落到 Vulkan
//              頂端且字形不上下顛倒。詳見 #67 修復記錄。)
//   - 無相機 fallback: perspective(70°, aspect, 0.001, 100)
//
// 此模組為 header-only,只依賴 glm(純 header library);GPU 後端、GUI 投影器、
// 測試各自 include。glm 非引擎 stdlib,移植性由 FetchContent 提供。

#include "Geometry/3D/Point3D.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace RendererMath
{

// View:由 Camera pos/rot(degree Euler)推算 front 向量,lookAt 造 view。
inline glm::mat4 BuildViewMatrix(const Point3D &pos, const Point3D &rot)
{
    glm::vec3 position(pos.x, pos.y, pos.z);
    glm::vec3 rotation(rot.x, rot.y, rot.z);
    glm::vec3 front;
    front.x = std::cos(glm::radians(rotation.y)) * std::cos(glm::radians(rotation.x));
    front.y = std::sin(glm::radians(rotation.x));
    front.z = std::sin(glm::radians(rotation.y)) * std::cos(glm::radians(rotation.x));
    return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0.0f, 1.0f, 0.0f));
}

inline glm::mat4 BuildProjMatrix(float fovDeg, float aspect, float nearP, float farP)
{
    return glm::perspective(glm::radians(fovDeg), aspect, nearP, farP);
}

inline glm::mat4 BuildWorldMatrix(const Point3D &pos, const Point3D &rot, const Point3D &scale)
{
    glm::mat4 world(1.0f);
    world = glm::translate(world, glm::vec3(pos.x, pos.y, pos.z));
    world = glm::rotate(world, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    world = glm::rotate(world, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    world = glm::scale(world, glm::vec3(scale.x, scale.y, scale.z));
    return world;
}

// GUI 正交投影 — NDC 空間,GL y-up + 深度翻轉(詳見上方慣例註解)。
// 是「GUI 幾何直出」的權威投影;文字與按鈕矩形共用同一投影(#67 RFC R1)。
inline glm::mat4 BuildGuiOrtho()
{
    return glm::orthoRH_ZO(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
}

// 無相機時的 3D fallback perspective(70° 視角,與 Camera 預設一致)。
inline glm::mat4 BuildFallbackPerspective(float aspect)
{
    return glm::perspective(glm::radians(70.0f), aspect, 0.001f, 100.0f);
}

} // namespace RendererMath

#endif // RENDERER_MATH_HPP