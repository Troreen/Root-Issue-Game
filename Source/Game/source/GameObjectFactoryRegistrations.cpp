#include "GameObjectFactoryRegistrations.h"

#include "AnimatedMeshComponent.h"
#include "AnimationDemoToggleComponent.h"
#include "AnimationEventDispatcherComponent.h"
#include "AnimationGraphComponent.h"
#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "CollisionShapeType.h"
#include "DamageableComponent.h"
#include "GameObject.h"
#include "GameObjectFactory.h"
#include "KnockbackComponent.h"
#include "LevelTransitionDoorComponent.h"
#include "MeshComponent.h"
#include "ParticleEmitterComponent.h"
#include "ModelTextureOverrides.h"
#include "ObbColliderComponent.h"
#include "ObjectLayer.h"
#include "SceneObjectData.h"
#include "PlayerControllerComponent.h"
#include "ResetComponent.h"
#include "BulletComponent.h"
#include "CameraComponent.h"
#include "MouseDirectionComponent.h"
#include "PickUpComponent.h"
#include <tge/animation/AnimationPlayer.h>
#include "SphereColliderComponent.h"
#include "TeleporterTunnelComponent.h"
#include "WorldTriggerHelpers.h"

#include "SwitchComponent.h"
#include "ToggleComponent.h"

// Enemy Components
#include "EnemyMovementComponent.h"
#include "EnemyAIComponent.h"
#include "EnemyTargetingComponent.h"
#include "EnemyAttackComponent.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <utility>
#include <tge/settings/settings.h>
#include "Essentials.h"

namespace
{
    std::string NormalizeText(const std::string& aValue)
    {
            std::string normalized = aValue;
            std::transform(
                normalized.begin(),
                normalized.end(),
                normalized.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return normalized;
    }

    std::string NormalizeAssetPath(std::string aPath)
    {
        std::replace(aPath.begin(), aPath.end(), '\\', '/');

        while (aPath.size() >= 2 && aPath[0] == '.' && aPath[1] == '/')
        {
            aPath.erase(aPath.begin(), aPath.begin() + 2);
        }

        while (!aPath.empty() && aPath.front() == '/')
        {
            aPath.erase(aPath.begin());
        }

        return aPath;
    }

    bool GameAssetExists(std::string aPath, const char* anExpectedExtension)
    {
        aPath = NormalizeAssetPath(std::move(aPath));
        if (aPath.empty())
        {
            return false;
        }

        if (anExpectedExtension && *anExpectedExtension)
        {
            std::filesystem::path path(aPath);
            if (!path.has_extension())
            {
                path.replace_extension(anExpectedExtension);
                aPath = NormalizeAssetPath(path.string());
            }
        }

        return !Tga::Settings::ResolveGameAssetPath(aPath).empty();
    }

    bool ShouldUseIdleOnlyAnimation(const SceneObjectData& aData, const std::string& anAnimationGraphPath)
    {
        const std::string animationGraphTemplate = NormalizeText(
            aData.GetPropertyOr<std::string>("animation_graph_template", ""));
        if (animationGraphTemplate != "idle_only")
        {
            return true;
        }

        const std::string idleClipPath = aData.GetPropertyOr<std::string>("clip_idle", "");
        if (!GameAssetExists(anAnimationGraphPath, ".tgscript"))
        {
            std::cout << "[Factory] Idle animation graph '" << anAnimationGraphPath
                << "' was not found for object '" << aData.name
                << "'. Falling back to static mesh.\n";
            return false;
        }

        if (!GameAssetExists(idleClipPath, nullptr))
        {
            std::cout << "[Factory] Idle animation clip '" << idleClipPath
                << "' was not found for object '" << aData.name
                << "'. Falling back to static mesh.\n";
            return false;
        }

        return true;
    }

    bool TryParseLayer(const std::string& aLayerText, ObjectLayer& outLayer)
    {
        const std::string normalized = NormalizeText(aLayerText);
        if (normalized == "worldstatic" || normalized == "static")
        {
            outLayer = ObjectLayer::WorldStatic;
            return true;
        }
        if (normalized == "player")
        {
            outLayer = ObjectLayer::Player;
            return true;
        }
        if (normalized == "enemy" || normalized == "basicmeleeenemy")
        {
            outLayer = ObjectLayer::Enemy;
            return true;
        }
        if (normalized == "projectile")
        {
            outLayer = ObjectLayer::Projectile;
            return true;
        }
        if (normalized == "trigger")
        {
            outLayer = ObjectLayer::Trigger;
            return true;
        }
        if (normalized == "pickup")
        {
            outLayer = ObjectLayer::Pickup;
            return true;
        }
        if (normalized == "npc")
        {
            outLayer = ObjectLayer::NPC;
            return true;
        }

        return false;
    }

    void ApplyLayer(GameObject& anObject, const SceneObjectData& aData, const ObjectLayer aDefaultLayer)
    {
        anObject.SetLayer(aDefaultLayer);

        const std::string authoredLayer = aData.GetPropertyOr<std::string>("layer", "");
        if (authoredLayer.empty())
        {
            return;
        }

        ObjectLayer parsedLayer = aDefaultLayer;
        if (TryParseLayer(authoredLayer, parsedLayer))
        {
            anObject.SetLayer(parsedLayer);
        }
        else
        {
            std::cout << "[Factory] Unknown layer '" << authoredLayer
                << "' for object '" << aData.name << "'. Using default layer.\n";
        }
    }

    bool TryGetColliderType(const SceneObjectData& aData, CollisionShapeType& outType)
    {
        int colliderTypeValue = 0;
        if (aData.TryGetProperty<int>("colliderType", colliderTypeValue))
        {
            outType = static_cast<CollisionShapeType>(colliderTypeValue);
            return true;
        }

        std::string colliderTypeText;
        if (aData.TryGetProperty<std::string>("colliderType", colliderTypeText))
        {
            const std::string normalized = NormalizeText(colliderTypeText);
            if (normalized.empty())
            {
                return false;
            }

            if (normalized == "box")
            {
                outType = CollisionShapeType::Box;
                return true;
            }
            if (normalized == "sphere")
            {
                outType = CollisionShapeType::Sphere;
                return true;
            }
            if (normalized == "capsule")
            {
                outType = CollisionShapeType::Capsule;
                return true;
            }
            if (normalized == "obb")
            {
                outType = CollisionShapeType::Obb;
                return true;
            }
        }

        return false;
    }

    void ApplyAuthoredCollider(GameObject& anObject, const SceneObjectData& aData)
    {
        CollisionShapeType colliderType = CollisionShapeType::Box;
        if (!TryGetColliderType(aData, colliderType))
        {
            return;
        }

        const Vector3f offset = aData.GetPropertyOr<Vector3f>("colliderOffset", Vector3f(0.0f, 0.0f, 0.0f));
        const bool isTrigger = aData.GetPropertyOr<bool>("colliderIsTrigger", false);
        const Vector3f colliderSize =
            aData.GetPropertyOr<Vector3f>("colliderSize", Vector3f(0.0f, 0.0f, 0.0f));

        if (colliderType == CollisionShapeType::Box)
        {
            const bool constantUpdate = aData.GetPropertyOr<bool>("colliderConstantUpdate", false);
            const bool pivotBottomMiddle = aData.GetPropertyOr<bool>("colliderPivotBottomMiddle", false);
            if (colliderSize.x <= 0.0f || colliderSize.y <= 0.0f || colliderSize.z <= 0.0f)
            {
                std::cout << "[Factory] Invalid colliderSize for object '" << aData.name
                    << "'. Box collider skipped.\n";
                return;
            }

            anObject.AddComponent<BoxColliderComponent>(colliderSize, offset, isTrigger, constantUpdate, pivotBottomMiddle);
            return;
        }

        if (colliderType == CollisionShapeType::Sphere)
        {
            float radius = aData.GetPropertyOr<float>("colliderRadius", 0.0f);
            if (radius <= 0.0f && colliderSize.x > 0.0f && colliderSize.y > 0.0f && colliderSize.z > 0.0f)
            {
                radius = (std::max)(colliderSize.x, (std::max)(colliderSize.y, colliderSize.z)) * 0.5f;
            }

            if (radius <= 0.0f)
            {
                std::cout << "[Factory] Invalid colliderRadius for object '" << aData.name
                    << "'. Sphere collider skipped. Author colliderRadius or colliderSize.\n";
                return;
            }

            const bool pivotBottomMiddle = aData.GetPropertyOr<bool>("colliderPivotBottomMiddle", true);
            const Vector3f anchorToCenter = pivotBottomMiddle
                ? Vector3f(0.0f, radius, 0.0f)
                : Vector3f(radius, radius, radius);
            anObject.AddComponent<SphereColliderComponent>(radius, offset + anchorToCenter, isTrigger);
            return;
        }

        if (colliderType == CollisionShapeType::Capsule)
        {
            float radius = aData.GetPropertyOr<float>("colliderRadius", 0.0f);
            float height = aData.GetPropertyOr<float>("colliderHeight", 0.0f);
            if ((radius <= 0.0f || height <= 0.0f) &&
                colliderSize.x > 0.0f && colliderSize.y > 0.0f && colliderSize.z > 0.0f)
            {
                if (radius <= 0.0f)
                {
                    radius = (std::min)(colliderSize.x, colliderSize.z) * 0.5f;
                }
                if (height <= 0.0f)
                {
                    height = colliderSize.y;
                }
            }

            const bool pivotBottomMiddle = aData.GetPropertyOr<bool>("colliderPivotBottomMiddle", true);
            if (radius <= 0.0f || height <= 0.0f || height < radius * 2.0f)
            {
                std::cout << "[Factory] Invalid capsule colliderRadius/colliderHeight for object '" << aData.name
                    << "'. Capsule collider skipped. Author colliderRadius/colliderHeight or colliderSize where height >= diameter.\n";
                return;
            }

            anObject.AddComponent<CapsuleColliderComponent>(radius, height, offset, isTrigger, pivotBottomMiddle);
            return;
        }

        if (colliderType == CollisionShapeType::Obb)
        {
            const bool pivotBottomMiddle = aData.GetPropertyOr<bool>("colliderPivotBottomMiddle", false);
            if (colliderSize.x <= 0.0f || colliderSize.y <= 0.0f || colliderSize.z <= 0.0f)
            {
                std::cout << "[Factory] Invalid colliderSize for object '" << aData.name
                    << "'. OBB collider skipped.\n";
                return;
            }

            anObject.AddComponent<ObbColliderComponent>(colliderSize, offset, isTrigger, pivotBottomMiddle);
            return;
        }

        std::cout << "[Factory] Unsupported colliderType value " << static_cast<int>(colliderType)
            << " for object '" << aData.name << "'. Supported values: 0=Box, 1=Sphere, 2=Capsule, 3=OBB.\n";
    }

    void ApplyCommonModel(GameObject& anObject, const SceneObjectData& aData)
    {
        const std::string modelPath = aData.GetPropertyOr<std::string>("modelPath", "");
        if (modelPath.empty())
        {
            return;
        }

        std::string animationGraphPath = aData.GetPropertyOr<std::string>("animation_graph", "");
        if (animationGraphPath.empty())
        {
            animationGraphPath = aData.GetPropertyOr<std::string>("animationGraph", "");
        }

        if (!animationGraphPath.empty() && ShouldUseIdleOnlyAnimation(aData, animationGraphPath))
        {
            AnimatedMeshComponent* animatedMeshComponent = anObject.AddComponent<AnimatedMeshComponent>(modelPath);
            MeshTextureOverrides textureOverrides;
            if (aData.TryGetProperty<MeshTextureOverrides>("modelTextures", textureOverrides))
            {
                animatedMeshComponent->SetTextureOverrides(textureOverrides);
            }

            AnimationGraphComponent* graphComponent = anObject.AddComponent<AnimationGraphComponent>(animationGraphPath);
            graphComponent->SetSourceProperties(aData.properties);
            graphComponent->SetModelPropertyName(aData.GetPropertyOr<std::string>("animation_model_property", "model"));
            graphComponent->SetApplyRootMotion(aData.GetPropertyOr<bool>("animation_apply_root_motion", false));
            graphComponent->SetApplyRootMotionRotation(aData.GetPropertyOr<bool>("animation_apply_root_motion_rotation", false));
            graphComponent->SetRootMotionTranslationScale(
                aData.GetPropertyOr<float>("animation_root_motion_translation_scale", 1.0f));

            if (aData.GetPropertyOr<bool>("animation_demo_toggles", false))
            {
                anObject.AddComponent<AnimationDemoToggleComponent>();
            }

            anObject.AddComponent<AnimationEventDispatcherComponent>();
            return;
        }

        MeshComponent* meshComponent = anObject.AddComponent<MeshComponent>(modelPath);

        MeshTextureOverrides textureOverrides;
        if (aData.TryGetProperty<MeshTextureOverrides>("modelTextures", textureOverrides))
        {
            meshComponent->SetTextureOverrides(textureOverrides);
        }
    }

    void ApplyAnimatedModel(GameObject& anObject, const SceneObjectData& aData)
    {
        const std::string modelPath = aData.GetPropertyOr<std::string>("modelPath", "");
        if (!modelPath.empty())
        {
            AnimatedMeshComponent* animatedMeshComponent = anObject.AddComponent<AnimatedMeshComponent>(modelPath);
            // Optionally apply texture overrides if authored
            MeshTextureOverrides textureOverrides;
            if (aData.TryGetProperty<MeshTextureOverrides>("modelTextures", textureOverrides))
            {
                std::cout << "[GameObjectFactoryRegistrations] Applying texture overrides to animated mesh: " << anObject.GetName() << "\n";
                animatedMeshComponent->SetTextureOverrides(textureOverrides);
            }
            else
            {
                std::cout << "[GameObjectFactoryRegistrations] No texture overrides found in TGO for: " << anObject.GetName() << "\n";
            }
        }
    }
    void EnsureTriggerCollider(GameObject& anObject)
    {
        if (!anObject.GetComponent<BoxColliderComponent>() &&
            !anObject.GetComponent<SphereColliderComponent>() &&
            !anObject.GetComponent<CapsuleColliderComponent>() &&
            !anObject.GetComponent<ObbColliderComponent>())
        {
            WorldTriggerHelpers::AddDefaultBoxTrigger(anObject, Vector3f(160.0f, 220.0f, 160.0f));
            return;
        }

        WorldTriggerHelpers::ForceColliderToTrigger(anObject);
    }

    std::unique_ptr<GameObject> BuildStaticWorld(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::WorldStatic);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);

        return object;
    }

    EnemyData CreateBasicEnemyData(const SceneObjectData& /*aData*/)
    {
        EnemyData MeleeEnemyData;

        MeleeEnemyData.EnemyType = EnemyType::BasicEnemy;

        MeleeEnemyData.WalkSpeed = 300.0f;
        MeleeEnemyData.ChaseSpeed = 450.0f;
        MeleeEnemyData.RotationSpeed = 5.0f;

        MeleeEnemyData.DetectionRange = 600.0f;

        //MeleeEnemyData.IdleTimeMin = 1.0f;
        //MeleeEnemyData.IdleTimeMax = 2.0f;

        //MeleeEnemyData.WanderTimeMin = 1.5f;
        //MeleeEnemyData.WanderTimeMax = 3.0f;

        //MeleeEnemyData.WanderTurnAngleMin = -30.0f;
        //MeleeEnemyData.WanderTurnAngleMax = 30.0f;

        MeleeEnemyData.AttackRange = 180.0f;
        MeleeEnemyData.AttackCooldown = 2.0f;
        MeleeEnemyData.AttackWindup = 0.3f;
        MeleeEnemyData.AttackRecovery = 1.0f;

        return MeleeEnemyData;
    }

    EnemyData CreateRollingEnemyData(const SceneObjectData& /*aData*/)
    {
        EnemyData RollingEnemyData;
        return RollingEnemyData;
    }

    std::unique_ptr<GameObject> BuildBasicMeleeEnemy(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::Enemy);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);
        if (!object->GetComponent<BoxColliderComponent>() &&
            !object->GetComponent<SphereColliderComponent>() &&
            !object->GetComponent<CapsuleColliderComponent>() &&
            !object->GetComponent<ObbColliderComponent>())
        {
            object->AddComponent<CapsuleColliderComponent>(50.0f, 180.0f, Vector3f::Zero, false, true);
        }

        EnemyData data = CreateBasicEnemyData(aData);

        int health = aData.GetPropertyOr<int>("health", 3);
        if (health < 1)
        {
            health = 1;
        }

        int damage = aData.GetPropertyOr<int>("damage", 1);
        if (damage < 1)
        {
            damage = 1;
        }

        object->AddComponent<EnemyAIComponent>(data);
        object->AddComponent<EnemyMovementComponent>();
        object->AddComponent<EnemyTargetingComponent>();
        object->AddComponent<KnockbackComponent>();
        object->AddComponent<EnemyAttackComponent>(data);
        object->AddComponent<ResetComponent>(aData);
        DamageableComponent* damageable = object->AddComponent<DamageableComponent>(health);
        damageable->SetCurrentHealth(health);
        damageable->SetDamagePerHit(damage);
        ParticleEmitterComponent* emitter = object->AddComponent<ParticleEmitterComponent>();
        emitter->AttachSettings();
        return object;
    }

    std::unique_ptr<GameObject> BuildPlayer(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        Essentials::SetPlayer(*object);
        ApplyLayer(*object, aData, ObjectLayer::Player);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);
        object->AddComponent<ResetComponent>(aData);
        object->AddComponent<PlayerControllerComponent>();
        object->AddComponent<CameraComponent>();
        object->AddComponent<MouseDirectionComponent>();

        int health = aData.GetPropertyOr<int>("health", 4);
        if (health < 1)
        {
            health = 1;
        }

        DamageableComponent* damageable = object->AddComponent<DamageableComponent>(health);
        damageable->SetCurrentHealth(health);
        return object;
    }

    std::unique_ptr<GameObject> BuildPickUp(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::Pickup);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);
        object->AddComponent<PickUpComponent>();
        return object;
    }

    std::unique_ptr<GameObject> SwitchBuild(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::Switch);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);

        if (!object->GetComponent<BoxColliderComponent>() &&
            !object->GetComponent<SphereColliderComponent>() &&
            !object->GetComponent<CapsuleColliderComponent>() &&
            !object->GetComponent<ObbColliderComponent>())
        {
            object->AddComponent<CapsuleColliderComponent>(50.0f, 180.0f, Vector3f::Zero, false, true);
        }

        object->AddComponent<SwitchComponent>(aData.GetPropertyOr("UniqueID", 0));
        return object;
    }

    std::unique_ptr<GameObject> BuildToggle(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::WorldStatic);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);

        object->AddComponent<ToggleComponent>(aData.GetPropertyOr("UniqueID", 0));
        return object;
    }

    std::unique_ptr<GameObject> BuildLevelTransitionDoor(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::Trigger);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);
        EnsureTriggerCollider(*object);

        object->AddComponent<LevelTransitionDoorComponent>(
            aData.GetPropertyOr<std::string>("targetScene", ""),
            aData.GetPropertyOr<std::string>("targetSpawnId", ""),
            aData.GetPropertyOr<float>("autoWalkSpeed", 600.0f),
            aData.GetPropertyOr<float>("fadeOutSeconds", 0.5f));

        return object;
    }

    std::unique_ptr<GameObject> BuildTeleporterTunnel(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::Trigger);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);
        EnsureTriggerCollider(*object);

        object->AddComponent<TeleporterTunnelComponent>(
            aData.GetPropertyOr<int>("pairId", 0),
            aData.GetPropertyOr<int>("exitDirection", 0),
            aData.GetPropertyOr<float>("autoWalkSpeed", 600.0f),
            aData.GetPropertyOr<float>("exitPadding", 90.0f));

        return object;
    }
}

void RegisterGameObjectFactories()
{
    GameObjectFactory& factory = GameObjectFactory::GetInstance();
    factory.Register("StaticWorld", BuildStaticWorld);
    factory.Register("BasicMeleeEnemy", BuildBasicMeleeEnemy);
    factory.Register("Player", BuildPlayer);
    factory.Register("Pickup", BuildPickUp);
    factory.Register("Switch", SwitchBuild);
    factory.Register("Toggle", BuildToggle);
    factory.Register("LevelTransitionDoor", BuildLevelTransitionDoor);
    factory.Register("TeleporterTunnel", BuildTeleporterTunnel);
}
