#include "GameObjectFactoryRegistrations.h"

#include "AnimatedMeshComponent.h"
#include "AnimationDemoToggleComponent.h"
#include "AnimationGraphComponent.h"
#include "BoxColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "CollisionShapeType.h"
#include "DamageableComponent.h"
#include "GameObject.h"
#include "GameObjectFactory.h"
#include "MeshComponent.h"
#include "AnimatedMeshComponent.h"
#include "ModelTextureOverrides.h"
#include "ObbColliderComponent.h"
#include "ObjectLayer.h"
#include "SceneObjectData.h"
#include "PlayerControllerComponent.h"
#include "BulletComponent.h"
#include "PickUpComponent.h"
#include <tge/animation/AnimationPlayer.h>
#include "SphereColliderComponent.h"

// Enemy Components
#include "EnemyMovementComponent.h"
#include "EnemyAIComponent.h"
#include "EnemyTargetingComponent.h"

#include <algorithm>
#include <cctype>
#include <iostream>
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
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return normalized;
    }

    bool TryParseLayer(const std::string& aLayerText, ObjectLayer& outLayer)
    {
        const std::string normalized = NormalizeText(aLayerText);
        if (normalized == "worldstatic")
        {
            outLayer = ObjectLayer::WorldStatic;
            return true;
        }
        if (normalized == "player")
        {
            outLayer = ObjectLayer::Player;
            return true;
        }
        if (normalized == "basicmeleeenemy")
        {
            outLayer = ObjectLayer::BasicMeleeEnemy;
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
                : Vector3f(-radius, radius, radius);
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

        if (!animationGraphPath.empty())
        {
            anObject.AddComponent<AnimatedMeshComponent>(modelPath);

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
            animatedMeshComponent;
            //MeshTextureOverrides textureOverrides;
            //if (aData.TryGetProperty<MeshTextureOverrides>("modelTextures", textureOverrides))
            //{
            //    animatedMeshComponent->SetTextureOverrides(textureOverrides);
            //}
        }
    }

    std::unique_ptr<GameObject> BuildStaticWorld(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::WorldStatic);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);

        return object;
    }

    std::unique_ptr<GameObject> BuildBasicMeleeEnemy(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        ApplyLayer(*object, aData, ObjectLayer::BasicMeleeEnemy);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);

        int health = aData.GetPropertyOr<int>("health", 100);
        if (health < 1)
        {
            health = 1;
        }

        int damage = aData.GetPropertyOr<int>("damage", 10);
        if (damage < 1)
        {
            damage = 1;
        }

        object->AddComponent<EnemyAIComponent>(EnemyType::BasicEnemy);
        object->AddComponent<EnemyMovementComponent>();
        object->AddComponent<EnemyTargetingComponent>();
        DamageableComponent* damageable = object->AddComponent<DamageableComponent>(health);
        damageable->SetCurrentHealth(health);
        damageable->SetDamagePerHit(damage);

        return object;
    }

    std::unique_ptr<GameObject> BuilPlayer(const SceneObjectData& aData)
    {
        auto object = std::make_unique<GameObject>(aData.name);
        Essentials::SetPlayer(*object);
        ApplyLayer(*object, aData, ObjectLayer::Player);
        ApplyCommonModel(*object, aData);
        ApplyAuthoredCollider(*object, aData);
        object->AddComponent<PlayerControllerComponent>();

        auto bullet = std::make_shared<GameObject>();
        bullet->SetLayer(ObjectLayer::Projectile);

        ApplyCommonModel(*bullet, aData);
        bullet->AddComponent<BulletComponent>();
        bullet->SetActive(false);

        //player->SetBullet(bullet);
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
}

void RegisterGameObjectFactories()
{
    GameObjectFactory& factory = GameObjectFactory::GetInstance();
    factory.Register("StaticWorld", BuildStaticWorld);
    factory.Register("BasicMeleeEnemy", BuildBasicMeleeEnemy);
    factory.Register("Player", BuilPlayer);
	factory.Register("Pickup", BuildPickUp);
}
