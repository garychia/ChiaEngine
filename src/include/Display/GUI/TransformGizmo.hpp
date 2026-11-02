#ifndef TRANSFORM_GIZMO_HPP
#define TRANSFORM_GIZMO_HPP

#include "Geometry/3D/Point3D.hpp"
#include "Scene/SceneSystem.hpp"
#include "Scene/TransformComponent.hpp"

class EditorSession;
class InspectorLayer;

enum class GizmoAxis { Position, Rotation, Scale };

// Pure math gizmo class — handles gizmo state, ray-plane intersection,
// and generates EditorCommand actions. The renderer draws lines
// separately; this class has no GPU/window coupling.
class TransformGizmo
{
public:
    TransformGizmo();
    TransformGizmo(SceneSystem *pScene, EditorSession *pSession, InspectorLayer *pInspector);
    void Bind(SceneSystem *pScene, EditorSession *pSession, InspectorLayer *pInspector);
    ~TransformGizmo();

    // Called from viewport render loop - handles input state
    void Update(float mouseX, float mouseY, float deltaTime);

    // Check if gizmo is active and ready for manipulation
    bool IsGizmoActive() const { return mIsActive; }

    // Begin manipulating the given entity along the specified axis
    void BeginGizmo(Entity entity, GizmoAxis axis);

    // End current manipulation
    void EndGizmo();

    // Update position based on mouse delta
    void UpdateGizmoPosition(float mouseDeltaX, float mouseDeltaY);

    // Update rotation based on mouse delta
    void UpdateGizmoRotation(float mouseDeltaX, float mouseDeltaY);

    // Update scale based on mouse delta
    void UpdateGizmoScale(float mouseDeltaX, float mouseDeltaY);

    // Ray-plane intersection helper
    bool RayPlaneIntersection(const Point3D &rayOrigin, const Point3D &rayDir,
                             const Point3D &planeNormal, float &t, const Point3D &planePoint) const;

private:
    // Core gizmo state
    SceneSystem *mScene = nullptr;
    EditorSession *mSession = nullptr;
    InspectorLayer *mInspector = nullptr;

    // Gizmo manipulation state
    bool mIsActive = false;
    Entity mSelectedEntity = Entity();
    GizmoAxis mActiveAxis = GizmoAxis::Position;
    Point3D mDragStartWorld;
    Point3D mDragStartMouse;

    // Plane normals for common editing planes
    Point3D mPositionPlaneNormal;
    Point3D mRotationPlaneNormal;
    Point3D mScalePlaneNormal;
};

#endif // TRANSFORM_GIZMO_HPP
