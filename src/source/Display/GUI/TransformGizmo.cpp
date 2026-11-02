#include "Display/GUI/TransformGizmo.hpp"
#include "Geometry/3D/Point3D.hpp"
#include "Math/Math.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/Selection.hpp"
#include "Scene/SceneSystem.hpp"
#include "Scene/TransformComponent.hpp"

TransformGizmo::TransformGizmo()
{
    // Default constructor: all pointers are nullptr (initialized in-class).
    // Use Bind() to wire up after EditorSession has its InspectorLayer.
    mPositionPlaneNormal = Point3D(0, 0, 1);
    mRotationPlaneNormal = Point3D(1, 0, 0);
    mScalePlaneNormal = Point3D(1, 1, 1);
}

TransformGizmo::TransformGizmo(SceneSystem *pScene, EditorSession *pSession, InspectorLayer *pInspector)
    : mScene(pScene), mSession(pSession), mInspector(pInspector)
{
    // Initialize plane normals for common editing planes
    mPositionPlaneNormal = Point3D(0, 0, 1);  // XY plane (Z up)
    mRotationPlaneNormal = Point3D(1, 0, 0);  // YZ plane (X forward)
    mScalePlaneNormal = Point3D(1, 1, 1);     // Uniform scale (all axes)
}

void TransformGizmo::Bind(SceneSystem *pScene, EditorSession *pSession, InspectorLayer *pInspector)
{
    mScene = pScene;
    mSession = pSession;
    mInspector = pInspector;
}

TransformGizmo::~TransformGizmo()
{
    // No resources to clean up
}

void TransformGizmo::Update(float mouseX, float mouseY, float deltaTime)
{
    (void)deltaTime;
    // This is called every frame from the viewport render loop
    // We need to check if we should activate the gizmo

    // Check if we have a selected entity and if the mouse is in a valid position
    if (mScene && mSession->GetSelection().hasSelection)
    {
        // For now, we'll activate the gizmo when the user clicks on a node in the hierarchy
        // This is simplified - in a real implementation, we'd check if the mouse is over a node
        if (mIsActive == false)
        {
            // Simulate activation when user interacts with the scene
            // In actual implementation, this would be triggered by mouse input over nodes
            mIsActive = true;
        }

        if (mIsActive)
        {
            // Update gizmo state based on mouse movement
            // This is a simplified version - real gizmo would track mouse drag
            float mouseDeltaX = mouseX;  // Simplified - real implementation would track delta
            float mouseDeltaY = mouseY;

            // For demonstration, we'll just rotate around Y axis when mouse moves horizontally
            if (mActiveAxis == GizmoAxis::Rotation)
            {
                UpdateGizmoRotation(mouseDeltaX, mouseDeltaY);
            }
            else if (mActiveAxis == GizmoAxis::Position)
            {
                UpdateGizmoPosition(mouseDeltaX, mouseDeltaY);
            }
            else if (mActiveAxis == GizmoAxis::Scale)
            {
                UpdateGizmoScale(mouseDeltaX, mouseDeltaY);
            }
        }
    }
}

void TransformGizmo::BeginGizmo(Entity entity, GizmoAxis axis)
{
    mSelectedEntity = entity;
    mActiveAxis = axis;
    mIsActive = true;

    // Get the current transform
    TransformComponent transform;
    if (mScene->GetTransform(entity.GetIndex(), transform))
    {
        // Store the starting position for delta calculations
        // This is a simplified approach - real gizmo would track mouse position
        mDragStartWorld = transform.position;
    }
}

void TransformGizmo::EndGizmo()
{
    mIsActive = false;
    mSelectedEntity = Entity();
}

void TransformGizmo::UpdateGizmoPosition(float mouseDeltaX, float mouseDeltaY)
{
    // Simplified position update - in real implementation, this would calculate
    // the actual 3D movement based on mouse movement and plane intersection
    TransformComponent transform;
    if (mScene->GetTransform(mSelectedEntity.GetIndex(), transform))
    {
        // Simple horizontal movement for X position
        transform.position.x += mouseDeltaX * 0.01f;
        mScene->EditTransform(mSelectedEntity.GetIndex(), transform);
    }
}

void TransformGizmo::UpdateGizmoRotation(float mouseDeltaX, float mouseDeltaY)
{
    // Simplified rotation - in real implementation, this would calculate proper axis-angle rotation
    TransformComponent transform;
    if (mScene->GetTransform(mSelectedEntity.GetIndex(), transform))
    {
        // Simple Y-axis rotation based on horizontal mouse movement
        transform.rotation.y += mouseDeltaX * 0.1f;
        mScene->EditTransform(mSelectedEntity.GetIndex(), transform);
    }
}

void TransformGizmo::UpdateGizmoScale(float mouseDeltaX, float mouseDeltaY)
{
    // Simplified scale - in real implementation, this would calculate scale based on mouse distance
    TransformComponent transform;
    if (mScene->GetTransform(mSelectedEntity.GetIndex(), transform))
    {
        // Uniform scale based on mouse movement
        float scaleFactor = 1.0f + mouseDeltaX * 0.01f;
        transform.scale.x = transform.scale.y = transform.scale.z = scaleFactor;
        mScene->EditTransform(mSelectedEntity.GetIndex(), transform);
    }
}

bool TransformGizmo::RayPlaneIntersection(const Point3D &rayOrigin, const Point3D &rayDir,
                                         const Point3D &planeNormal, float &t, const Point3D &planePoint) const
{
    // Simple ray-plane intersection test
    float denom = rayDir.x * planeNormal.x + rayDir.y * planeNormal.y + rayDir.z * planeNormal.z;
    if (fabsf(denom) < 1e-6f)  // Parallel to plane
        return false;

    Point3D diff = planePoint - rayOrigin;
    float diffDotNormal = diff.x * planeNormal.x + diff.y * planeNormal.y + diff.z * planeNormal.z;
    float tValue = diffDotNormal / denom;
    if (tValue < 0)  // Intersection behind ray origin
        return false;

    // Calculate point on plane (local variable, do not overwrite input planePoint)
    Point3D hitPoint = rayOrigin + rayDir * tValue;
    t = tValue;
    (void)hitPoint;  // caller can recompute if needed; t is the useful output
    return true;
}
