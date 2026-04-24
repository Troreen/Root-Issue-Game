#include "stdafx.h"
#include "Camera.h"
#include <iostream>

using namespace Tga;

Camera::Camera()
{}

Camera::~Camera()
{}

void Camera::SetOrtographicProjection(float aWidth, float aHeight, float aDepth)
{
	myProjection = {};

	myProjection(1, 1) = 2.f/aWidth;
	myProjection(2, 2) = 2.f/aHeight;
	myProjection(3, 3) = 1.f/aDepth;
}

void Camera::SetOrtographicProjection(float aLeft, float aRight, float aTop, float aBottom, float aNear, float aFar)
{
	myProjection = {};

	myProjection(1, 1) = 2.f / (aRight - aLeft);
	myProjection(2, 2) = 2.f / (aBottom - aTop);
	myProjection(3, 3) = 1.f / (aFar - aNear);

	myProjection(4, 1) = -(aRight + aLeft) / (aRight - aLeft);
	myProjection(4, 2) = -(aBottom + aTop) / (aBottom - aTop);
	myProjection(4, 3) = -(aNear) / (aFar - aNear);
}

void Camera::SetPerspectiveProjection(float aHorizontalFoV, Vector2f aResolution, float aNearPlane, float aFarPlane)
{
	myProjection = {};

	assert(aNearPlane < aFarPlane);
	assert(aNearPlane >= FMath::KindaSmallNumber);
	
    // aHorizontalFoV is in Degrees!
	// Convert to Radians
	const float hFoVRad = aHorizontalFoV * (FMath::Pi / 180);

	float xScale = 1 / std::tanf(hFoVRad * 0.5f);
	float yScale = aResolution.x / (aResolution.y * std::tanf(hFoVRad * 0.5f));
	float Q = aFarPlane / (aFarPlane - aNearPlane);

	myProjection(1, 1) = xScale;
	myProjection(2, 2) = yScale;
	myProjection(3, 3) = Q;
	myProjection(3, 4) = 1.0f;
	myProjection(4, 3) = -Q * aNearPlane;
	myProjection(4, 4) = 0.0f;
}

void Camera::SetTransform(Matrix4x4f someTransform)
{
	myTransform = someTransform;
}
