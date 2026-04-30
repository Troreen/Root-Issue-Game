#pragma once
#include "ScriptComponent.h"
#include "CommonUtilities/Vector.hpp"
#include "Bullet.h"
#include <vector>
#include <memory>


class BulletComponent : public ScriptComponent
{
public:

	BulletComponent();

	void OnUpdate(float aDeltaTime) override;
	void Render() override;

	void SetSpeedDirectionPosition(float aSpeed, CommonUtilities::Vector3<float> aDirection);
	void SpawnBullet();

private:

	Bullet myBullet;
	std::vector<Bullet> myBullets;
};

