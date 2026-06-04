
#include "EnemyAIComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyTargetingComponent.h"
#include "EnemyAnimationComponent.h"
#include "ParticleEmitterComponent.h"
#include "EnemyAttackComponent.h"
#include "DamageableComponent.h"
#include "SphereColliderComponent.h"
#include "GameObject.h"
#include "Essentials.h"

EnemyAIComponent::EnemyAIComponent(const EnemyData& someEnemyData)
{
    myEnemyData = someEnemyData;
}

void EnemyAIComponent::Init(Tga::Engine&)
{
    if (myHasBeenInitialized)
    {
        return;
    }

    myMovement = GetOwner()->GetComponent<EnemyMovementComponent>();
    myTargeting = GetOwner()->GetComponent<EnemyTargetingComponent>();
    myAnimation = GetOwner()->GetComponent<EnemyAnimationComponent>();
    myEmitterComponent = GetOwner()->GetComponent<ParticleEmitterComponent>();
    myAttack = GetOwner()->GetComponent<EnemyAttackComponent>();
    myDamageableComponent = GetOwner()->GetComponent<DamageableComponent>();

    /*if (myEnemyData.EnemyType == EnemyType::RollingEnemy)
    {
        myCollider = GetOwner()->GetComponent<SphereColliderComponent>();

        if (!myCollider)
        {
            std::cout << "Error: myCollider is nullptr in AI" << std::endl;
        }
    }*/

    //Debug
    if (!myMovement)
    {
        std::cout << "Error: Movement is nullptr in AI" << std::endl;
    }
    if (!myTargeting)
    {
        std::cout << "Error: Targeting is nullptr in AI" << std::endl;
    }
    if (!myAnimation)
    {
        std::cout << "Error: Animation is nullptr in AI" << std::endl;
    }
    if (!myEmitterComponent)
    {
        std::cout << "Error: EmitterComponent is nullptr in AI" << std::endl;
    }
    if (!myAttack)
    {
        std::cout << "Error: myAttack is nullptr in AI" << std::endl;
    }
    if (!myDamageableComponent)
    {
        std::cout << "Error: myDamageableComponent is nullptr in AI" << std::endl;
    }

    //// TODO: testing of particles
    /*myEmitterComponent->SetContinuousEmission(ParticleType::Blood, true);
    myEmitterComponent->SetContinuousEmission(ParticleType::Test, true);*/
    ////end


    if (myEnemyData.ShouldSpawn)
    {
        myCurrentState = EnemyState::Spawn;

        GetOwner()->SetActive(false);

        Essentials::AddEnemy(this);
    }
    else
    {
        myCurrentState = EnemyState::Idle;
    }

    std::random_device seed;
    myRandomEngine = std::mt19937(seed());

    myIdleTimer = GetRandomFloat(1.f, 2.f);
    myWanderTimer = GetRandomFloat(1.5f, 3.f);

    mySpawnTimer = myEnemyData.SpawnTime;

    myStartPosition = GetOwner()->GetTransform().GetPosition();

    PickNewDirection();

    myHasBeenInitialized = true;
}

void EnemyAIComponent::Reset()
{
    if (!myActiveAfterSave)
    {
        return;
    }

    GetOwner()->SetActive(!myEnemyData.ShouldSpawn);

    myCurrentState = myEnemyData.ShouldSpawn ? EnemyState::Spawn : EnemyState::Idle;
    myIsAggro = false;
    mySpawnTimer = myEnemyData.SpawnTime;
    myIdleTimer = GetRandomFloat(1.f, 2.f);
    myWanderTimer = GetRandomFloat(1.5f, 3.f);
}

void EnemyAIComponent::Save()
{
    myActiveAfterSave = myCurrentState != EnemyState::Death;
}

void EnemyAIComponent::Spawn()
{
    GetOwner()->SetActive(true);

    myAnimation->BlendTo(EnemyAnimationState::Spawn);
}

void EnemyAIComponent::ChangeState(EnemyState aState)
{
    myPreviousState = myCurrentState;
    myCurrentState = aState;
}

void EnemyAIComponent::PickNewDirection()
{
    float angle = GetRandomAngleDegreeToRad(-180.f, 180.f);

    myWanderDirection.x = sin(angle);
    myWanderDirection.z = cos(angle);
    myWanderDirection.y = 0.f;

    myWanderDirection.Normalize();
}

void EnemyAIComponent::MoveTowardsHome(float aDeltaTime)
{
    Vector3f direction = myStartPosition - GetOwner()->GetTransform().GetPosition();

    float distance = direction.Length();

    if (distance <= myReturnTolerance)
    {
        myMovement->StopMoving();

        myMovement->SetMovementSpeed(myEnemyData.WalkSpeed);

        if (myEnemyData.EnemyType == EnemyType::BasicEnemy)
        {
            if (!myIsAggro)
            {
                myAnimation->BlendTo(EnemyAnimationState::Idle);
            }
            else
            {
                myAnimation->BlendTo(EnemyAnimationState::IdleAggro);
            }
        }
        else
        {
            myAnimation->BlendTo(EnemyAnimationState::Idle);
        }

        ChangeState(EnemyState::Idle);

        return;
    }

    direction.Normalize();

    myMovement->RotateTowards(direction, aDeltaTime);
    myMovement->MoveForward(aDeltaTime);
}

float EnemyAIComponent::GetRandomFloat(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);

    return dist(myRandomEngine);
}

float EnemyAIComponent::GetRandomAngleDegreeToRad(float min, float max)
{
    float pi = 3.14159f;

    float minRadians = min * pi / 180.f;
    float maxRadians = max * pi / 180.f;

    std::uniform_real_distribution<float> dist(minRadians, maxRadians);

    return dist(myRandomEngine);
}

std::string EnemyAIComponent::StringifyState(const EnemyState& aState) const
{
    switch (aState)
    {
    case EnemyState::Idle: return "Idle";
    case EnemyState::Wander: return "Wander";
    case EnemyState::Chasing: return "Chasing";
    case EnemyState::Attacking: return "Attacking";
    case EnemyState::Hurt: return "Hurt";
    case EnemyState::Stunned: return "Stunned";
    case EnemyState::Death: return "Death";
    case EnemyState::ReturnHome: return "Returning";
    default: return "Unknown";
    }
}
