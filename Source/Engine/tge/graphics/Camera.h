#pragma once

#include <tge/Math/Matrix4x4.h>

namespace Tga
{

class Camera
{
private:

	Matrix4x4f myTransform{};
	
	Matrix4x4f myProjection{};
	float myNearPlane = 1.0f;
	float myFarPlane = 10000.0f;
	
public:
	Camera();
	~Camera();
	
	void SetOrtographicProjection(float aWidth, float aHeight, float aDepth);
	void SetOrtographicProjection(float aLeft, float aRight, float aTop, float aBottom, float aNear, float aFar);
	void SetPerspectiveProjection(float aHorizontalFoV, Vector2f aResolution, float aNearPlane, float aFarPlane);

	void SetTransform(Matrix4x4f someTransform);

	Matrix4x4f& GetTransform() { return myTransform; }
	const Matrix4x4f& GetTransform() const { return myTransform; }
	const Matrix4x4f& GetProjection() const { return myProjection; }
	void GetProjectionPlanes(float& aNearPlane, float& aFarPlane) const { aNearPlane = myNearPlane; aFarPlane = myFarPlane; }
};

} // namespace Tga
