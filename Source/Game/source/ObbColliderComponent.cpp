#include "ObbColliderComponent.h"

#include "DebugSettings.h"
#include "GameObject.h"

#include <algorithm>
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
    void DrawLine(Tga::LineDrawer& aDrawer, const Vector3f& aFrom, const Vector3f& aTo, const Tga::Color& aColor)
    {
        Tga::LinePrimitive line;
        line.fromPosition = { aFrom.x, aFrom.y, aFrom.z };
        line.toPosition = { aTo.x, aTo.y, aTo.z };
        line.color = aColor.AsVec4();
        aDrawer.Draw(line);
    }

    void LogDrawerDebugLine(const std::string& aText)
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
    }
}

ObbColliderComponent::ObbColliderComponent(
    const Vector3f& aSize,
    const Vector3f& aOffset,
    bool anIsTrigger,
    bool aPivotBottomMiddle)
    : myIsTrigger(anIsTrigger)
    , myIsInside(false)
    , myPivotBottomMiddle(aPivotBottomMiddle)
    , mySize(aSize)
    , myOffset(aOffset)
    , myCenter(Vector3f::Zero)
{
    myAxes[0] = Vector3f::UnitX;
    myAxes[1] = Vector3f::UnitY;
    myAxes[2] = Vector3f::UnitZ;
}

bool ObbColliderComponent::OnTriggerEnter()
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

bool ObbColliderComponent::OnTriggerExit()
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

bool ObbColliderComponent::IsInside() const
{
    return myIsInside;
}

void ObbColliderComponent::SetIsTrigger(bool anIsTrigger)
{
    myIsTrigger = anIsTrigger;
    if (!myIsTrigger)
    {
        myIsInside = false;
    }
}

bool ObbColliderComponent::IsTrigger() const
{
    return myIsTrigger;
}

void ObbColliderComponent::Init(Tga::Engine& /*anEngine*/)
{
    UpdateObb();
}

void ObbColliderComponent::Update(float /*aDeltaTime*/)
{
    UpdateObb();
}

void ObbColliderComponent::Render()
{
#ifndef _RETAIL
    if (!GameDebugSettings::ShowColliderDebugLines())
    {
        return;
    }

    UpdateObb();

    auto& graphicsEngine = Tga::Engine::GetInstance()->GetGraphicsEngine();
    Tga::GraphicsStateStack& graphicsStateStack = graphicsEngine.GetGraphicsStateStack();
    Tga::LineDrawer& drawer = graphicsEngine.GetLineDrawer();
    const Tga::Color color = myIsTrigger
        ? Tga::Color{ 1.f, 0.f, 1.f, 1.f }
        : Tga::Color{ 1.f, 0.85f, 0.f, 1.f };

    const Vector3f half = GetHalfExtents();
    const Vector3f corners[8] =
    {
        myCenter - myAxes[0] * half.x - myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x - myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x - myAxes[1] * half.y + myAxes[2] * half.z,
        myCenter - myAxes[0] * half.x - myAxes[1] * half.y + myAxes[2] * half.z,
        myCenter - myAxes[0] * half.x + myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x + myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x + myAxes[1] * half.y + myAxes[2] * half.z,
        myCenter - myAxes[0] * half.x + myAxes[1] * half.y + myAxes[2] * half.z,
    };

    if (GameDebugSettings::EnableColliderDrawerDebugLog())
    {
        const Vector3f ownerPosition = GetOwner()
            ? GetOwner()->GetTransform().GetPosition()
            : Vector3f(0.0f, 0.0f, 0.0f);

        std::ostringstream stream;
        stream << "OBB owner='" << (GetOwner() ? GetOwner()->GetName() : "<none>") << "'"
            << " ownerOrigin=(" << ownerPosition.x << ", " << ownerPosition.y << ", " << ownerPosition.z << ")"
            << " size=(" << mySize.x << ", " << mySize.y << ", " << mySize.z << ")"
            << " offset=(" << myOffset.x << ", " << myOffset.y << ", " << myOffset.z << ")"
            << " pivotBottomMiddle=" << (myPivotBottomMiddle ? "true" : "false")
            << " center=(" << myCenter.x << ", " << myCenter.y << ", " << myCenter.z << ")"
            << " axisX=(" << myAxes[0].x << ", " << myAxes[0].y << ", " << myAxes[0].z << ")"
            << " axisY=(" << myAxes[1].x << ", " << myAxes[1].y << ", " << myAxes[1].z << ")"
            << " axisZ=(" << myAxes[2].x << ", " << myAxes[2].y << ", " << myAxes[2].z << ")"
            << " aabbMin=(" << myAABB.GetMin().x << ", " << myAABB.GetMin().y << ", " << myAABB.GetMin().z << ")"
            << " aabbMax=(" << myAABB.GetMax().x << ", " << myAABB.GetMax().y << ", " << myAABB.GetMax().z << ")";
        LogDrawerDebugLine(stream.str());
    }

    graphicsStateStack.Push();
    graphicsStateStack.SetTransform(Tga::Matrix4x4f::CreateIdentityMatrix());

    DrawLine(drawer, corners[0], corners[1], color);
    DrawLine(drawer, corners[1], corners[2], color);
    DrawLine(drawer, corners[2], corners[3], color);
    DrawLine(drawer, corners[3], corners[0], color);
    DrawLine(drawer, corners[4], corners[5], color);
    DrawLine(drawer, corners[5], corners[6], color);
    DrawLine(drawer, corners[6], corners[7], color);
    DrawLine(drawer, corners[7], corners[4], color);
    DrawLine(drawer, corners[0], corners[4], color);
    DrawLine(drawer, corners[1], corners[5], color);
    DrawLine(drawer, corners[2], corners[6], color);
    DrawLine(drawer, corners[3], corners[7], color);

    const float markerExtent = (std::max)(10.0f, (std::max)(mySize.x, (std::max)(mySize.y, mySize.z)) * 0.15f);
    const Tga::Color centerColor = Tga::Color{ 1.f, 1.f, 0.f, 1.f };
    DrawLine(drawer, myCenter - myAxes[0] * markerExtent, myCenter + myAxes[0] * markerExtent, centerColor);
    DrawLine(drawer, myCenter - myAxes[1] * markerExtent, myCenter + myAxes[1] * markerExtent, centerColor);
    DrawLine(drawer, myCenter - myAxes[2] * markerExtent, myCenter + myAxes[2] * markerExtent, centerColor);

    graphicsStateStack.Pop();
#endif
}

void ObbColliderComponent::SetSize(const Vector3f& aSize)
{
    mySize = aSize;
    UpdateObb();
}

const Vector3f& ObbColliderComponent::GetSize() const
{
    return mySize;
}

void ObbColliderComponent::SetOffset(const Vector3f& aOffset)
{
    myOffset = aOffset;
    UpdateObb();
}

const Vector3f& ObbColliderComponent::GetOffset() const
{
    return myOffset;
}

void ObbColliderComponent::SetPivotBottomMiddle(bool aPivotBottomMiddle)
{
    myPivotBottomMiddle = aPivotBottomMiddle;
    UpdateObb();
}

bool ObbColliderComponent::IsPivotBottomMiddle() const
{
    return myPivotBottomMiddle;
}

const Vector3f& ObbColliderComponent::GetCenter() const
{
    return myCenter;
}

const Vector3f* ObbColliderComponent::GetAxes() const
{
    return myAxes;
}

Vector3f ObbColliderComponent::GetHalfExtents() const
{
    return Vector3f(mySize.x * 0.5f, mySize.y * 0.5f, mySize.z * 0.5f);
}

const CommonUtilities::AABB3D<float>& ObbColliderComponent::GetAABB() const
{
    return myAABB;
}

void ObbColliderComponent::UpdateObb()
{
    GameObject* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    myAxes[0] = owner->GetTransform().GetRight().GetNormalized();
    myAxes[1] = owner->GetTransform().GetUp().GetNormalized();
    myAxes[2] = owner->GetTransform().GetForward().GetNormalized();

    const Vector3f anchorToCenter = myPivotBottomMiddle
        ? myAxes[1] * (mySize.y * 0.5f)
        : myAxes[0] * (mySize.x * 0.5f) + myAxes[1] * (mySize.y * 0.5f) + myAxes[2] * (mySize.z * 0.5f);
    myCenter = owner->GetTransform().GetPosition() + anchorToCenter + myOffset;

    const Vector3f half = GetHalfExtents();
    const Vector3f corners[8] =
    {
        myCenter - myAxes[0] * half.x - myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x - myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x - myAxes[1] * half.y + myAxes[2] * half.z,
        myCenter - myAxes[0] * half.x - myAxes[1] * half.y + myAxes[2] * half.z,
        myCenter - myAxes[0] * half.x + myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x + myAxes[1] * half.y - myAxes[2] * half.z,
        myCenter + myAxes[0] * half.x + myAxes[1] * half.y + myAxes[2] * half.z,
        myCenter - myAxes[0] * half.x + myAxes[1] * half.y + myAxes[2] * half.z,
    };

    Vector3f minBounds = corners[0];
    Vector3f maxBounds = corners[0];
    for (int i = 1; i < 8; ++i)
    {
        minBounds.x = (std::min)(minBounds.x, corners[i].x);
        minBounds.y = (std::min)(minBounds.y, corners[i].y);
        minBounds.z = (std::min)(minBounds.z, corners[i].z);
        maxBounds.x = (std::max)(maxBounds.x, corners[i].x);
        maxBounds.y = (std::max)(maxBounds.y, corners[i].y);
        maxBounds.z = (std::max)(maxBounds.z, corners[i].z);
    }

    myAABB = CommonUtilities::AABB3D<float>(minBounds, maxBounds);
}
