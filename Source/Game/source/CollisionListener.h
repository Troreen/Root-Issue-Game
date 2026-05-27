#pragma once

class GameObject;
struct CollisionContact;

class CollisionListener
{
public:
    virtual ~CollisionListener() = default;

    virtual void OnCollisionEnter(const CollisionContact& aContact, GameObject& anOther) = 0;
    virtual void OnCollisionStay(const CollisionContact& aContact, GameObject& anOther) = 0;
    virtual void OnCollisionExit(const CollisionContact& aContact, GameObject& anOther) = 0;
};
