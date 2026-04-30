#include "CapsuleColliderComponent.h"

#include "DebugSettings.h"
#include "GameObject.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include <tge/drawers/LineDrawer.h>
#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/math/Matrix4x4.h>
#include <tge/math/color.h>
#include <tge/primitives/LinePrimitive.h>

namespace
{
    constexpr int kCircleSegments = 32;
    constexpr float kPi = 3.14159265358979323846f;

    void DrawLine(Tga::LineDrawer& aDrawer, const Vector3f& aFrom, const Vector3f& aTo, const Tga::Color& aColor)
    {
        Tga::LinePrimitive line;
        line.fromPosition = { aFrom.x, aFrom.y, aFrom.z };
        line.toPosition = { aTo.x, aTo.y, aTo.z };
        line.color = aColor.AsVec4();
        aDrawer.Draw(line);
    }
}

CapsuleColliderComponent::CapsuleColliderComponent(
    float aRadius,
    float aHeight,
    const Vector3f& aOffset,
    bool anIsTrigger,
    bool aPivotBottomMiddle)
    : myIsTrigger(anIsTrigger)
    , myIsInside(false)
    , myPivotBottomMiddle(aPivotBottomMiddle)
    , myRadius(aRadius)
    , myHeight(aHeight)
    , myOffset(aOffset)
    , myBottomCenter(Vector3f::Zero)
    , myTopCenter(Vector3f::Zero)
{
}

bool CapsuleColliderComponent::OnTriggerEnter()
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

bool CapsuleColliderComponent::OnTriggerExit()
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

bool CapsuleColliderComponent::IsInside() const
{
    return myIsInside;
}

void CapsuleColliderComponent::SetIsTrigger(bool anIsTrigger)
{
    myIsTrigger = anIsTrigger;
    if (!myIsTrigger)
    {
        myIsInside = false;
    }
}

bool CapsuleColliderComponent::IsTrigger() const
{
    return myIsTrigger;
}

void CapsuleColliderComponent::Init(Tga::Engine& /*anEngine*/)
{
    UpdateCapsule();
}

void CapsuleColliderComponent::Update(float /*aDeltaTime*/)
{
    UpdateCapsule();
}

void CapsuleColliderComponent::Render()
{
#ifndef _RETAIL
    if (!GameDebugSettings::ShowColliderDebugLines())
    {
        return;
    }

    UpdateCapsule();

    auto& graphicsEngine = Tga::Engine::GetInstance()->GetGraphicsEngine();
    Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();
    Tga::LineDrawer& drawer = graphicsEngine.GetLineDrawer();
    const Tga::Color color = myIsTrigger
        ? Tga::Color{ 1.f, 0.f, 1.f, 1.f }
        : Tga::Color{ 0.f, 0.75f, 1.f, 1.f };

    if (GameDebugSettings::EnableColliderDrawerDebugLog())
    {
        const Vector3f ownerPosition = GetOwner()
            ? GetOwner()->GetTransform().GetPosition()
            : Vector3f(0.0f, 0.0f, 0.0f);

        std::ostringstream stream;
        stream << "Capsule owner='" << (GetOwner() ? GetOwner()->GetName() : "<none>") << "'"
            << " ownerOrigin=(" << ownerPosition.x << ", " << ownerPosition.y << ", " << ownerPosition.z << ")"
            << " radius=" << myRadius
            << " height=" << myHeight
            << " offset=(" << myOffset.x << ", " << myOffset.y << ", " << myOffset.z << ")"
            << " pivotBottomMiddle=" << (myPivotBottomMiddle ? "true" : "false")
            << " bottomCenter=(" << myBottomCenter.x << ", " << myBottomCenter.y << ", " << myBottomCenter.z << ")"
            << " topCenter=(" << myTopCenter.x << ", " << myTopCenter.y << ", " << myTopCenter.z << ")"
            << " aabbMin=(" << myAABB.GetMin().x << ", " << myAABB.GetMin().y << ", " << myAABB.GetMin().z << ")"
            << " aabbMax=(" << myAABB.GetMax().x << ", " << myAABB.GetMax().y << ", " << myAABB.GetMax().z << ")";
        std::cout << "[ColliderDrawerDebug] " << stream.str() << "\n";
    }

    graphicsStateStack.Push();
    graphicsStateStack.SetTransform(Tga::Matrix4x4f::CreateIdentityMatrix());

    auto drawRing = [&](const Vector3f& aCenter)
    {
        Vector3f previous;
        for (int i = 0; i <= kCircleSegments; ++i)
        {
            const float angle = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(kCircleSegments);
            const Vector3f current(
                aCenter.x + std::cos(angle) * myRadius,
                aCenter.y,
                aCenter.z + std::sin(angle) * myRadius);
            if (i > 0)
            {
                DrawLine(drawer, previous, current, color);
            }
            previous = current;
        }
    };

    drawRing(myBottomCenter);
    drawRing(myTopCenter);
    DrawLine(drawer, myBottomCenter + Vector3f(myRadius, 0.0f, 0.0f), myTopCenter + Vector3f(myRadius, 0.0f, 0.0f), color);
    DrawLine(drawer, myBottomCenter - Vector3f(myRadius, 0.0f, 0.0f), myTopCenter - Vector3f(myRadius, 0.0f, 0.0f), color);
    DrawLine(drawer, myBottomCenter + Vector3f(0.0f, 0.0f, myRadius), myTopCenter + Vector3f(0.0f, 0.0f, myRadius), color);
    DrawLine(drawer, myBottomCenter - Vector3f(0.0f, 0.0f, myRadius), myTopCenter - Vector3f(0.0f, 0.0f, myRadius), color);

    graphicsStateStack.Pop();
#endif
}

void CapsuleColliderComponent::SetRadius(float aRadius)
{
    myRadius = aRadius;
    UpdateCapsule();
}

float CapsuleColliderComponent::GetRadius() const
{
    return myRadius;
}

void CapsuleColliderComponent::SetHeight(float aHeight)
{
    myHeight = aHeight;
    UpdateCapsule();
}

float CapsuleColliderComponent::GetHeight() const
{
    return myHeight;
}

void CapsuleColliderComponent::SetOffset(const Vector3f& aOffset)
{
    myOffset = aOffset;
    UpdateCapsule();
}

const Vector3f& CapsuleColliderComponent::GetOffset() const
{
    return myOffset;
}

void CapsuleColliderComponent::SetPivotBottomMiddle(bool aPivotBottomMiddle)
{
    myPivotBottomMiddle = aPivotBottomMiddle;
    UpdateCapsule();
}

bool CapsuleColliderComponent::IsPivotBottomMiddle() const
{
    return myPivotBottomMiddle;
}

Vector3f CapsuleColliderComponent::GetBottomCenter() const
{
    return myBottomCenter;
}

Vector3f CapsuleColliderComponent::GetTopCenter() const
{
    return myTopCenter;
}

const CommonUtilities::AABB3D<float>& CapsuleColliderComponent::GetAABB() const
{
    return myAABB;
}

void CapsuleColliderComponent::UpdateCapsule()
{
    GameObject* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    const float radius = (std::max)(0.0f, myRadius);
    const float cylinderHeight = (std::max)(0.0f, myHeight - radius * 2.0f);
    const Vector3f anchorToBottomCenter = myPivotBottomMiddle
        ? Vector3f(0.0f, radius, 0.0f)
        : Vector3f(-radius, radius, radius);

    myBottomCenter = owner->GetTransform().GetPosition() + anchorToBottomCenter + myOffset;
    myTopCenter = myBottomCenter + Vector3f(0.0f, cylinderHeight, 0.0f);

    const float minX = myBottomCenter.x < myTopCenter.x ? myBottomCenter.x : myTopCenter.x;
    const float minY = myBottomCenter.y < myTopCenter.y ? myBottomCenter.y : myTopCenter.y;
    const float minZ = myBottomCenter.z < myTopCenter.z ? myBottomCenter.z : myTopCenter.z;
    const float maxX = myBottomCenter.x > myTopCenter.x ? myBottomCenter.x : myTopCenter.x;
    const float maxY = myBottomCenter.y > myTopCenter.y ? myBottomCenter.y : myTopCenter.y;
    const float maxZ = myBottomCenter.z > myTopCenter.z ? myBottomCenter.z : myTopCenter.z;
    const Vector3f minBounds(
        minX - radius,
        minY - radius,
        minZ - radius);
    const Vector3f maxBounds(
        maxX + radius,
        maxY + radius,
        maxZ + radius);

    myAABB = CommonUtilities::AABB3D<float>(minBounds, maxBounds);
    owner->SetHitbox(myAABB);
}
