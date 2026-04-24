#include "SphereColliderComponent.h"
#include "GameObject.h"
#include "DebugSettings.h"

#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/LineDrawer.h>
#include <tge/primitives/LinePrimitive.h>
#include <tge/math/color.h>

#include <algorithm>
#include <cmath>
#include <iostream>

static constexpr int kCircleSegments = 32;
static constexpr float kPi = 3.14159265358979323846f;

SphereColliderComponent::SphereColliderComponent(float aRadius, const Vector3f& aOffset, bool anIsTrigger)
    : myIsTrigger(anIsTrigger)
    , myIsInside(false)
    , myRadius(aRadius)
    , myOffset(aOffset)
{
}

bool SphereColliderComponent::OnTriggerEnter()
{
    if (!myIsTrigger)
    {
        return false;
    }

    if (!myIsInside)
    {
        myIsInside = true;
        if (GameDebugSettings::ShowColliderDebugLines())
        {
            std::cout << "[" + GetOwner()->GetName() + "] Execute Trigger: Enter \n";
        }
        return true;
    }

    return false;
}

bool SphereColliderComponent::OnTriggerExit()
{
    if (!myIsTrigger)
    {
        return false;
    }

    if (myIsInside)
    {
        myIsInside = false;
        if (GameDebugSettings::ShowColliderDebugLines())
        {
            std::cout << "[" + GetOwner()->GetName() + "] Execute Trigger: Exit \n";
        }
        return true;
    }

    return false;
}

bool SphereColliderComponent::IsInside() const
{
    if (myIsInside && GameDebugSettings::ShowColliderDebugLines())
    {
        std::cout << "[" + GetOwner()->GetName() + "] Execute Trigger: Player Inside \n";
    }

    return myIsInside;
}

void SphereColliderComponent::SetIsTrigger(bool anIsTrigger)
{
    myIsTrigger = anIsTrigger;
    if (!myIsTrigger)
    {
        myIsInside = false;
    }
}

bool SphereColliderComponent::IsTrigger() const
{
    return myIsTrigger;
}

void SphereColliderComponent::Init(Tga::Engine& /*anEngine*/)
{
    UpdateSphere();
}

void SphereColliderComponent::Update(float /*aDeltaTime*/)
{
    UpdateSphere();
}

void SphereColliderComponent::Render()
{
#ifndef _RETAIL
    if (!GameDebugSettings::ShowColliderDebugLines())
    {
        return;
    }

    UpdateSphere();

    const Vector3f& center = mySphere.GetCenter();
    const float r = mySphere.GetRadius();

    Tga::Color col = myIsTrigger
        ? Tga::Color{ 1.f, 0.f, 1.f, 1.f }
        : Tga::Color{ 0.f, 0.5f, 1.f, 1.f };

    Tga::LineDrawer& drawer = Tga::Engine::GetInstance()->GetGraphicsEngine().GetLineDrawer();

    // Draw 3 axis-aligned circles (XY, XZ, YZ planes)
    auto drawCircle = [&](int axisA, int axisB)
    {
        // axisA/B are the two plane axes, axisC is the constant axis
        Tga::Vector3f prev;
        for (int i = 0; i <= kCircleSegments; ++i)
        {
            float angle = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(kCircleSegments);
            Tga::Vector3f pt = { center.x, center.y, center.z };

            // We treat the float* as an array to set axes generically
            float* ptArr = &pt.x;
            const float* cArr = &center.x;
            ptArr[axisA] = cArr[axisA] + r * std::cos(angle);
            ptArr[axisB] = cArr[axisB] + r * std::sin(angle);
            // axisC stays at center

            if (i > 0)
            {
                Tga::LinePrimitive lp;
                lp.fromPosition = prev;
                lp.toPosition = pt;
                lp.color = col.AsVec4();
                drawer.Draw(lp);
            }
            prev = pt;
        }
    };

    drawCircle(0, 1); // XY circle
    drawCircle(0, 2); // XZ circle
    drawCircle(1, 2); // YZ circle

    const Tga::Color pivotColor = Tga::Color{ 1.f, 1.f, 0.f, 1.f };
    const float pivotHalfExtent = std::max(10.0f, r * 0.25f);
    const Vector3f pivot = center;

    auto drawPivotAxis = [&](const Vector3f& anAxis)
    {
        const Vector3f scaledAxis(
            anAxis.x * pivotHalfExtent,
            anAxis.y * pivotHalfExtent,
            anAxis.z * pivotHalfExtent);

        const Vector3f from(
            pivot.x - scaledAxis.x,
            pivot.y - scaledAxis.y,
            pivot.z - scaledAxis.z);

        const Vector3f to(
            pivot.x + scaledAxis.x,
            pivot.y + scaledAxis.y,
            pivot.z + scaledAxis.z);

        Tga::LinePrimitive lp;
        lp.fromPosition = { from.x, from.y, from.z };
        lp.toPosition = { to.x, to.y, to.z };
        lp.color = pivotColor.AsVec4();
        drawer.Draw(lp);
    };

    const Vector3f axisX(1.f, 0.f, 0.f);
    const Vector3f axisY(0.f, 1.f, 0.f);
    const Vector3f axisZ(0.f, 0.f, 1.f);
    drawPivotAxis(axisX);
    drawPivotAxis(axisY);
    drawPivotAxis(axisZ);
#endif
}

void SphereColliderComponent::SetRadius(float aRadius)
{
    myRadius = aRadius;
    UpdateSphere();
}

float SphereColliderComponent::GetRadius() const
{
    return myRadius;
}

void SphereColliderComponent::SetOffset(const Vector3f& aOffset)
{
    myOffset = aOffset;
    UpdateSphere();
}

const Vector3f& SphereColliderComponent::GetOffset() const
{
    return myOffset;
}

const CommonUtilities::Sphere<float>& SphereColliderComponent::GetSphere() const
{
    return mySphere;
}

void SphereColliderComponent::UpdateSphere()
{
    auto* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    const Vector3f pos = owner->GetTransform().GetPosition() + myOffset;
    mySphere.InitWithCenterAndRadius(pos, myRadius);

    // Also update the owner's AABB hitbox to be a bounding box of the sphere
    const Vector3f half(myRadius, myRadius, myRadius);
    owner->SetHitbox(CommonUtilities::AABB3D<float>(pos - half, pos + half));
}
