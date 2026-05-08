#pragma once

#include "Component.h"

#include <functional>

class DamageableComponent final : public Component
{
public:
    explicit DamageableComponent(int aMaxHealth);

    void Update(float aDeltaTime) override;

    void SetMaxHealth(int aMaxHealth);
    int GetMaxHealth() const;

    void SetCurrentHealth(int aHealth);
    int GetCurrentHealth() const;

    void Heal(int anAmount);
    void TakeDamage(int anAmount, GameObject* anInstigator = nullptr);

    void SetInvincible(bool anInvincible);
    bool IsInvincible() const;

    void SetDamagePerHit(int anAmount);
    int GetDamagePerHit() const;

    void SetDamageInvulnerabilityDuration(float aDurationSeconds);
    float GetDamageInvulnerabilityDuration() const;

    bool IsDead() const;

    void Reset() override;

    void SetOnDamageCallback(std::function<void(int, GameObject*)> aCallback);
    void SetOnDeathCallback(std::function<void(GameObject*)> aCallback);

    const bool TookDamageThisFrame() const;

private:
    int myMaxHealth;
    int myCurrentHealth;
    int myHealthAtStartOfFrame;
    bool myIsInvincible = false;
    bool myHasDied = false;
    int myDamagePerHit = 1;
    float myDamageInvulnerabilityDuration = 0.0f;
    float myDamageInvulnerabilityTimer = 0.0f;

    std::function<void(int, GameObject*)> myOnDamage;
    std::function<void(GameObject*)> myOnDeath;
};
