#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Geometry/3D/Point3D.hpp"
#include "System/Operation/Event.hpp"
#include "Data/Pointers.hpp"

class Camera
{
  private:
    Point3D position;
    Point3D rotation;
    Point3D upVector = Point3D(0, 1);
    float angleOfView = 70.f;
    float nearPlane = 0.001f;
    float farPlane = 100.f;
    float toFocusDistance = 1.5f;

  public:
    Event<void(const SharedPtr<Camera>)> onChanged;

    Camera(const Point3D &position = Point3D(), const Point3D &rotation = Point3D());

    Point3D GetPosition() const;

    void Translate(float deltaX, float deltaY = 0.f, float deltaZ = 0.f);

    void Translate(const Point3D &delta);

    void Rotate(float deltaPitch, float deltaYaw = 0.f, float deltaRoll = 0.f);

    void Rotate(const Point3D &delta);

    Point3D GetFocalPointPosition() const;

    Point3D GetUpVector() const;

    Point3D GetFrontVector() const;

    float GetAngleOfView() const;

    float GetDistanceToNearPlane() const;

    float GetDistanceToFarPlane() const;

    void SetAngleOfView(float angleInDegrees);

    void SetDistanceToNearPlane(float distance);

    void SetDistanceToFarPlane(float distance);
};

#endif
