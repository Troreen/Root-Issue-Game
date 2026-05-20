#include "DamageableComponent.h"

#include <algorithm>

void DamageableComponent::Update(float aDeltaTime)
{
    myHealthAtStartOfFrame = myCurrentHealth;

    if (myDamageInvulnerabilityTimer > 0.0f)
    {
        myDamageInvulnerabilityTimer = std::max(0.0f, myDamageInvulnerabilityTimer - aDeltaTime);
    }
}

DamageableComponent::DamageableComponent(int aMaxHealth)
    : myMaxHealth(std::max(0, aMaxHealth))
    , myCurrentHealth(std::max(0, aMaxHealth))
{
}

void DamageableComponent::SetMaxHealth(int aMaxHealth)
{
    myMaxHealth = std::max(0, aMaxHealth);
    myCurrentHealth = std::min(myCurrentHealth, myMaxHealth);
}

int DamageableComponent::GetMaxHealth() const
{
    return myMaxHealth;
}

void DamageableComponent::SetCurrentHealth(int aHealth)
{
    myCurrentHealth = std::clamp(aHealth, 0, myMaxHealth);
    if (myCurrentHealth > 0)
    {
        myHasDied = false;
    }
    if (myCurrentHealth <= 0)
    {
        if (!myHasDied && myOnDeath)
        {
            myHasDied = true;
            myOnDeath(GetOwner());
        }
    }
}

int DamageableComponent::GetCurrentHealth() const
{
    return myCurrentHealth;
}

void DamageableComponent::Heal(int anAmount)
{
    if (anAmount <= 0)
    {
        return;
    }

    SetCurrentHealth(myCurrentHealth + anAmount);
}

void DamageableComponent::TakeDamage(int anAmount, GameObject* anInstigator)
{
    if (IsInvincible() || anAmount <= 0)
    {
        return;
    }

    myCurrentHealth = std::clamp(myCurrentHealth - anAmount, 0, myMaxHealth);

    if (myOnDamage)
    {
        myOnDamage(anAmount, anInstigator);
    }

    if (myCurrentHealth <= 0 && !myHasDied)
    {
        myHasDied = true;
        if (myOnDeath)
        {
            myOnDeath(GetOwner());
        }
    }
    else if (myDamageInvulnerabilityDuration > 0.0f)
    {
        myDamageInvulnerabilityTimer = myDamageInvulnerabilityDuration;
    }
}

void DamageableComponent::SetInvincible(bool anInvincible)
{
    myIsInvincible = anInvincible;
}

bool DamageableComponent::IsInvincible() const
{
    return myIsInvincible || myDamageInvulnerabilityTimer > 0.0f;
}

void DamageableComponent::SetDamagePerHit(int anAmount)
{
    myDamagePerHit = std::max(1, anAmount);
}

int DamageableComponent::GetDamagePerHit() const
{
    return myDamagePerHit;
}

void DamageableComponent::SetDamageInvulnerabilityDuration(float aDurationSeconds)
{
    myDamageInvulnerabilityDuration = std::max(0.0f, aDurationSeconds);
}

float DamageableComponent::GetDamageInvulnerabilityDuration() const
{
    return myDamageInvulnerabilityDuration;
}

bool DamageableComponent::IsDead() const
{
    return myCurrentHealth <= 0;
}

void DamageableComponent::Reset()
{
    myCurrentHealth = myMaxHealth;
    myHasDied = false;
    myDamageInvulnerabilityTimer = 0.0f;
}

void DamageableComponent::SetOnDamageCallback(std::function<void(int, GameObject*)> aCallback)
{
    myOnDamage = std::move(aCallback);
}

void DamageableComponent::SetOnDeathCallback(std::function<void(GameObject*)> aCallback)
{
    myOnDeath = std::move(aCallback);
}

const bool DamageableComponent::TookDamageThisFrame() const
{
    return myCurrentHealth < myHealthAtStartOfFrame;
}
