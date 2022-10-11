#include "Camera.hpp"
#include "Math/Math.hpp"

Camera::Camera(const Point3D &position, const Point3D &rotation) : position(position), rotation(rotation), onChanged()
{
}

Point3D Camera::GetPosition() const
{
    return position;
}

void Camera::Translate(float deltaX, float deltaY, float deltaZ)
{
    position += Point3D(deltaX, deltaY, deltaZ);
    onChanged.Invoke(*this);
}

void Camera::Translate(const Point3D &delta)
{
    position += delta;
    onChanged.Invoke(*this);
}

void Camera::Rotate(float deltaPitch, float deltaYaw, float deltaRoll)
{
    Camera::Rotate(Point3D(deltaPitch, deltaYaw, deltaRoll));
    onChanged.Invoke(*this);
}

void Camera::Rotate(const Point3D &delta)
{
    rotation += delta;
    if (rotation.x >= 89.5f)
        rotation.x = 89.5f;
    else if (rotation.x <= -89.5f)
        rotation.x = -89.5f;
    onChanged.Invoke(*this);
}

Point3D Camera::GetFocalPointPosition() const
{
    return position + GetFrontVector() * toFocusDistance;
}

Point3D Camera::GetUpVector() const
{
    return upVector;
}

Point3D Camera::GetFrontVector() const
{
    Point3D frontVector;
    const auto pitch = rotation.x;
    const auto yaw = rotation.y;
    const auto roll = rotation.z;
    frontVector.x = Math::Sine(Math::ToRadians(yaw)) * Math::Cosine(Math::ToRadians(pitch));
    frontVector.y = Math::Sine(Math::ToRadians(pitch));
    frontVector.z = Math::Cosine(Math::ToRadians(yaw)) * Math::Cosine(Math::ToRadians(pitch));
    return frontVector;
}

float Camera::GetAngleOfView() const
{
    return angleOfView;
}

float Camera::GetDistanceToNearPlane() const
{
    return nearPlane;
}

float Camera::GetDistanceToFarPlane() const
{
    return farPlane;
}

void Camera::SetAngleOfView(float angleInDegrees)
{
    angleOfView = angleInDegrees;
    onChanged.Invoke(*this);
}

void Camera::SetDistanceToNearPlane(float distance)
{
    nearPlane = distance;
    onChanged.Invoke(*this);
}

void Camera::SetDistanceToFarPlane(float distance)
{
    farPlane = distance;
    onChanged.Invoke(*this);
}
