#include "BoxColliderComponent.h"
#include "GameObject.h"
#include "DebugSettings.h"

#include <algorithm>
#include <iostream>

#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/LineDrawer.h>
#include <tge/primitives/LinePrimitive.h>
#include <tge/math/color.h>

BoxColliderComponent::BoxColliderComponent(const Vector3f& aSize, const Vector3f& aOffset, bool anIsTrigger, bool aConstantUpdate)
    : myIsTrigger(anIsTrigger)
    , myIsInside(false)
    , myConstantUpdate(aConstantUpdate)
    , mySize(aSize)
    , myOffset(aOffset)
{
}

bool BoxColliderComponent::OnTriggerEnter()
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

bool BoxColliderComponent::OnTriggerExit()
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

bool BoxColliderComponent::IsInside() const
{
    if (myIsInside && GameDebugSettings::ShowColliderDebugLines())
    {
        std::cout << "[" + GetOwner()->GetName() + "] Execute Trigger: Player Inside \n";
    }

    return myIsInside;
}

void BoxColliderComponent::SetIsTrigger(bool anIsTrigger)
{
    myIsTrigger = anIsTrigger;
    if (!myIsTrigger)
    {
        myIsInside = false;
    }
}

bool BoxColliderComponent::IsTrigger() const
{
    return myIsTrigger;
}

void BoxColliderComponent::Init(Tga::Engine& /*anEngine*/)
{
    UpdateAABB();
}

void BoxColliderComponent::Update(float /*aDeltaTime*/)
{
    if (!myIsTrigger || myConstantUpdate)
    {
        UpdateAABB();
    }
}

void BoxColliderComponent::Render()
{
#ifndef _RETAIL
    if (!GameDebugSettings::ShowColliderDebugLines())
    {
        return;
    }

    UpdateAABB();

    const CommonUtilities::AABB3D<float>& debugAabb = GetOwner()
        ? GetOwner()->GetHitbox()
        : myAABB;

    const auto& min = debugAabb.GetMin();
    const auto& max = debugAabb.GetMax();

    const Tga::Vector3f minCorner = { min.x, min.y, min.z };
    const Tga::Vector3f maxCorner = { max.x, max.y, max.z };

    const Tga::Vector3f bottomNearLeft = { minCorner.x, minCorner.y, minCorner.z };
    const Tga::Vector3f bottomNearRight = { maxCorner.x, minCorner.y, minCorner.z };
    const Tga::Vector3f bottomFarRight = { maxCorner.x, minCorner.y, maxCorner.z };
    const Tga::Vector3f bottomFarLeft = { minCorner.x, minCorner.y, maxCorner.z };

    const Tga::Vector3f topNearLeft = { minCorner.x, maxCorner.y, minCorner.z };
    const Tga::Vector3f topNearRight = { maxCorner.x, maxCorner.y, minCorner.z };
    const Tga::Vector3f topFarRight = { maxCorner.x, maxCorner.y, maxCorner.z };
    const Tga::Vector3f topFarLeft = { minCorner.x, maxCorner.y, maxCorner.z };

    Tga::Color col = myIsTrigger
        ? Tga::Color{ 1.f, 0.f, 1.f, 1.f }
        : Tga::Color{ 0.f, 1.f, 0.f, 1.f };

    Tga::LineDrawer& drawer = Tga::Engine::GetInstance()->GetGraphicsEngine().GetLineDrawer();

    auto drawEdge = [&](const Tga::Vector3f& a, const Tga::Vector3f& b)
    {
        Tga::LinePrimitive lp;
        lp.fromPosition = a;
        lp.toPosition = b;
        lp.color = col.AsVec4();
        drawer.Draw(lp);
    };

    drawEdge(bottomNearLeft, bottomNearRight);
    drawEdge(bottomNearRight, bottomFarRight);
    drawEdge(bottomFarRight, bottomFarLeft);
    drawEdge(bottomFarLeft, bottomNearLeft);

    drawEdge(topNearLeft, topNearRight);
    drawEdge(topNearRight, topFarRight);
    drawEdge(topFarRight, topFarLeft);
    drawEdge(topFarLeft, topNearLeft);

    drawEdge(bottomNearLeft, topNearLeft);
    drawEdge(bottomNearRight, topNearRight);
    drawEdge(bottomFarRight, topFarRight);
    drawEdge(bottomFarLeft, topFarLeft);

    const Tga::Color centerColor = Tga::Color{ 1.f, 1.f, 0.f, 1.f };
    const Tga::Vector3f colliderCenter = {
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    };
    const float centerHalfExtent = std::max(10.0f, std::max(mySize.x, std::max(mySize.y, mySize.z)) * 0.15f);

    auto drawMarkerAxis = [&](const Tga::Vector3f& aCenter, const Tga::Vector3f& anAxis, const float aHalfExtent, const Tga::Color& aColor)
    {
        Tga::LinePrimitive lp;
        lp.fromPosition = aCenter - anAxis * aHalfExtent;
        lp.toPosition = aCenter + anAxis * aHalfExtent;
        lp.color = aColor.AsVec4();
        drawer.Draw(lp);
    };

    drawMarkerAxis(colliderCenter, { 1.f, 0.f, 0.f }, centerHalfExtent, centerColor);
    drawMarkerAxis(colliderCenter, { 0.f, 1.f, 0.f }, centerHalfExtent, centerColor);
    drawMarkerAxis(colliderCenter, { 0.f, 0.f, 1.f }, centerHalfExtent, centerColor);

    if (const GameObject* owner = GetOwner())
    {
        const Vector3f ownerPosition = owner->GetTransform().GetPosition();
        const Tga::Vector3f objectOrigin = { ownerPosition.x, ownerPosition.y, ownerPosition.z };
        constexpr float originHalfExtent = 35.0f;
        const Tga::Color originColor = Tga::Color{ 1.f, 0.35f, 0.f, 1.f };
        drawMarkerAxis(objectOrigin, { 1.f, 0.f, 0.f }, originHalfExtent, originColor);
        drawMarkerAxis(objectOrigin, { 0.f, 1.f, 0.f }, originHalfExtent, originColor);
        drawMarkerAxis(objectOrigin, { 0.f, 0.f, 1.f }, originHalfExtent, originColor);
    }
#endif
}

void BoxColliderComponent::SetSize(const Vector3f& aSize)
{
    mySize = aSize;
    UpdateAABB();
}

const Vector3f& BoxColliderComponent::GetSize() const
{
    return mySize;
}

void BoxColliderComponent::SetOffset(const Vector3f& aOffset)
{
    myOffset = aOffset;
    UpdateAABB();
}

const Vector3f& BoxColliderComponent::GetOffset() const
{
    return myOffset;
}

const CommonUtilities::AABB3D<float>& BoxColliderComponent::GetAABB() const
{
    return myAABB;
}

void BoxColliderComponent::UpdateAABB()
{
    auto* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    const Vector3f pos = owner->GetTransform().GetPosition() + myOffset;
    const Vector3f half = mySize * 0.5f;

    myAABB = CommonUtilities::AABB3D<float>(pos - half, pos + half);
    owner->SetHitbox(myAABB);
}
