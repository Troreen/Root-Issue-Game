#pragma once
#include <tge/model/ModelInstance.h>
#include "CommonUtilities/Transform.hpp"
#include <tge/drawers/ModelDrawer.h>

class Bullet
{
public:
	Bullet() = default;
	Bullet(Tga::ModelInstance aInstance);
	~Bullet();

	void Init(CommonUtilities::Transform<float> aTransform);
	void Update(float aDeltaTime);
	void Render(Tga::ModelDrawer& aDrawer);

	bool IsDelete();

private:
	bool myIsDelete;
	float myTimer;
	float mySpeed;
	Tga::ModelInstance myInstance;
};