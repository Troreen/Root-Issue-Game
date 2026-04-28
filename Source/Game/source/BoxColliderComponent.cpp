#include "BoxColliderComponent.h"
#include "GameObject.h"
#include "DebugSettings.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include <tge/engine.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/drawers/LineDrawer.h>
#include <tge/primitives/LinePrimitive.h>
#include <tge/math/color.h>
#include <tge/math/Matrix4x4.h>

BoxColliderComponent::BoxColliderComponent(
    const Vector3f& aSize,
    const Vector3f& aOffset,
    bool anIsTrigger,
    bool aConstantUpdate,
    bool aPivotBottomMiddle)
    : myIsTrigger(anIsTrigger)
    , myIsInside(false)
    , myConstantUpdate(aConstantUpdate)
    , myPivotBottomMiddle(aPivotBottomMiddle)
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
    const Tga::Vector3f size = { max.x - min.x, max.y - min.y, max.z - min.z };

    const Tga::Vector3f bottomNearLeft = { 0.0f, 0.0f, 0.0f };
    const Tga::Vector3f bottomNearRight = { size.x, 0.0f, 0.0f };
    const Tga::Vector3f bottomFarRight = { size.x, 0.0f, size.z };
    const Tga::Vector3f bottomFarLeft = { 0.0f, 0.0f, size.z };

    const Tga::Vector3f topNearLeft = { 0.0f, size.y, 0.0f };
    const Tga::Vector3f topNearRight = { size.x, size.y, 0.0f };
    const Tga::Vector3f topFarRight = { size.x, size.y, size.z };
    const Tga::Vector3f topFarLeft = { 0.0f, size.y, size.z };

    Tga::Color col = myIsTrigger
        ? Tga::Color{ 1.f, 0.f, 1.f, 1.f }
        : Tga::Color{ 0.f, 1.f, 0.f, 1.f };

    auto& graphicsEngine = Tga::Engine::GetInstance()->GetGraphicsEngine();
    Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();
    Tga::LineDrawer& drawer = graphicsEngine.GetLineDrawer();

    auto logDrawerDebugLine = [](const std::string& aText)
    {
        if (!GameDebugSettings::EnableColliderDrawerDebugLog())
        {
            return;
        }

        static int lastFrameKey = -1;
        static int budget = 0;
        static int skipped = 0;

        const float totalTime = Tga::Engine::GetInstance()
            ? static_cast<float>(Tga::Engine::GetInstance()->GetTotalTime())
            : 0.0f;
        const int frameKey = static_cast<int>(totalTime * 60.0f);
        if (frameKey != lastFrameKey)
        {
            if (skipped > 0)
            {
                std::cout << "[ColliderDrawerDebug] skipped " << skipped
                    << " log lines because Collider Drawer Log Cap / Frame was reached\n";
            }

            lastFrameKey = frameKey;
            budget = (std::max)(1, GameDebugSettings::MaxColliderDrawerDebugLogsPerFrame());
            skipped = 0;
        }

        if (budget <= 0)
        {
            ++skipped;
            return;
        }

        --budget;
        std::cout << "[ColliderDrawerDebug] " << aText << "\n";
    };

    if (GameDebugSettings::EnableColliderDrawerDebugLog())
    {
        std::ostringstream stream;
        stream << "Box owner='" << (GetOwner() ? GetOwner()->GetName() : "<none>") << "'"
            << " ownerOrigin=(" << (GetOwner() ? GetOwner()->GetTransform().GetPosition().x : 0.0f)
            << ", " << (GetOwner() ? GetOwner()->GetTransform().GetPosition().y : 0.0f)
            << ", " << (GetOwner() ? GetOwner()->GetTransform().GetPosition().z : 0.0f) << ")"
            << " size=(" << mySize.x << ", " << mySize.y << ", " << mySize.z << ")"
            << " offset=(" << myOffset.x << ", " << myOffset.y << ", " << myOffset.z << ")"
            << " pivotBottomMiddle=" << (myPivotBottomMiddle ? "true" : "false")
            << " aabbMin=(" << min.x << ", " << min.y << ", " << min.z << ")"
            << " aabbMax=(" << max.x << ", " << max.y << ", " << max.z << ")"
            << " drawLocalMin=(0, 0, 0)"
            << " drawLocalMax=(" << size.x << ", " << size.y << ", " << size.z << ")"
            << " drawTransformPos=(" << minCorner.x << ", " << minCorner.y << ", " << minCorner.z << ")"
            << " graphicsTransformPosBefore=(" << graphicsStateStack.GetPosition().x
            << ", " << graphicsStateStack.GetPosition().y
            << ", " << graphicsStateStack.GetPosition().z << ")";
        logDrawerDebugLine(stream.str());
    }

    graphicsStateStack.Push();
    graphicsStateStack.SetTransform(Tga::Matrix4x4f::CreateIdentityMatrix());
    graphicsStateStack.ApplyTransform(Tga::Matrix4x4f::CreateFromTranslation(minCorner));

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
        size.x * 0.5f,
        size.y * 0.5f,
        size.z * 0.5f
    };
    const float centerHalfExtent = (std::max)(10.0f, (std::max)(mySize.x, (std::max)(mySize.y, mySize.z)) * 0.15f);

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
        const Tga::Vector3f objectOrigin = {
            ownerPosition.x - min.x,
            ownerPosition.y - min.y,
            ownerPosition.z - min.z
        };
        constexpr float originHalfExtent = 35.0f;
        const Tga::Color originColor = Tga::Color{ 1.f, 0.35f, 0.f, 1.f };
        drawMarkerAxis(objectOrigin, { 1.f, 0.f, 0.f }, originHalfExtent, originColor);
        drawMarkerAxis(objectOrigin, { 0.f, 1.f, 0.f }, originHalfExtent, originColor);
        drawMarkerAxis(objectOrigin, { 0.f, 0.f, 1.f }, originHalfExtent, originColor);
    }

    graphicsStateStack.Pop();
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

void BoxColliderComponent::SetPivotBottomMiddle(bool aPivotBottomMiddle)
{
    myPivotBottomMiddle = aPivotBottomMiddle;
    UpdateAABB();
}

bool BoxColliderComponent::IsPivotBottomMiddle() const
{
    return myPivotBottomMiddle;
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

    const Vector3f anchorToMinOffset = myPivotBottomMiddle
        ? Vector3f(-mySize.x * 0.5f, 0.0f, -mySize.z * 0.5f)
        : Vector3f(-mySize.x, 0.0f, 0.0f);

    const Vector3f min = owner->GetTransform().GetPosition() + anchorToMinOffset + myOffset;
    const Vector3f max = min + mySize;

    myAABB = CommonUtilities::AABB3D<float>(min, max);
    owner->SetHitbox(myAABB);
}
