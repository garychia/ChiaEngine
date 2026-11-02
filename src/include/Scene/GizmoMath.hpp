#ifndef GIZMO_MATH_HPP
#define GIZMO_MATH_HPP

// Pure C++17 gizmo math for Transform Gizmo (Issue #168)
// Reusable, header-only, no GPU/window dependencies
// Reuses existing Point3D type from src/include/Geometry/3D/Point3D.hpp

#include "Geometry/3D/Point3D.hpp"

namespace gizmo {

struct Ray {
    Point3D origin;
    Point3D direction;  // should be normalized for best results
};

struct Plane {
    Point3D point;      // a point on the plane
    Point3D normal;     // plane normal (should be normalized)
};

struct Axis {
    enum Type { X, Y, Z };
};

struct DragResult {
    bool hit;
    Point3D hitPoint;
    float distanceAlongAxis;
};

// Returns unit vector for the specified axis
inline Point3D gizmoAxisDirection(Axis axis) {
    switch (axis.Type) {
        case Axis::X: return Point3D(1, 0, 0);
        case Axis::Y: return Point3D(0, 1, 0);
        case Axis::Z: return Point3D(0, 0, 1);
        default: return Point3D(0, 0, 0); // unreachable
    }
}

// Intersect ray with plane, return false if ray is parallel to plane
// outHit contains the hit point if intersection occurs
inline bool intersectRayPlane(const Ray& ray, const Plane& plane, Point3D& outHit) {
    const float denom = ray.direction.x * plane.normal.x + ray.direction.y * plane.normal.y + ray.direction.z * plane.normal.z;
    if (Math::Abs(denom) < 1e-6f) {
        // Ray is parallel to plane
        return false;
    }

    const float t = ((plane.point.x - ray.origin.x) * plane.normal.x +
                     (plane.point.y - ray.origin.y) * plane.normal.y +
                     (plane.point.z - ray.origin.z) * plane.normal.z) / denom;
    if (t < 0.0f) {
        // Intersection behind the ray origin
        return false;
    }

    outHit = ray.origin + ray.direction * t;
    return true;
}

// Drag along axis: ray-plane intersection with plane normal = axis direction
// Clamp t to [minT, maxT] range
inline DragResult dragAlongAxis(const Ray& ray, Axis axis, Point3D anchor, float minT, float maxT) {
    DragResult result;
    result.hit = false;
    result.distanceAlongAxis = 0.0f;

    // Create plane with normal = axis direction, passing through anchor
    Plane plane;
    plane.normal = gizmoAxisDirection(axis);
    plane.point = anchor;

    Point3D hitPoint;
    if (!intersectRayPlane(ray, plane, hitPoint)) {
        return result;
    }

    // Calculate distance along axis
    Point3D toHit = hitPoint - anchor;
    Point3D axisDir = gizmoAxisDirection(axis);
    result.distanceAlongAxis = toHit.x * axisDir.x + toHit.y * axisDir.y + toHit.z * axisDir.z;

    // Clamp to [minT, maxT]
    if (result.distanceAlongAxis < minT) {
        result.distanceAlongAxis = minT;
    } else if (result.distanceAlongAxis > maxT) {
        result.distanceAlongAxis = maxT;
    }

    // Calculate actual hit point based on clamped distance
    result.hitPoint = anchor + axisDir * result.distanceAlongAxis;
    result.hit = true;

    return result;
}

} // namespace gizmo

#endif // GIZMO_MATH_HPP