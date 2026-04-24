#pragma once
#include <vector>
#include <memory>
#include <string>
#include <set>

#include <tge/math/Matrix4x4.h>
#include <tge/scene/SceneObjectDefinition.h>

#include <tge/util/StringCast.h>
#include <tge/uuid/UUIDManager.h>

namespace Tga
{
	class Camera;
	class Scene;
	class SceneObject;
	class ScriptRuntimeInstance;
	class SceneObjectDefinitionManager;
	struct ScriptUpdateContext;

	struct TRS {
		Vector3f translation = { 0.0f };
		Vector3f rotation = { 0.0f };
		Vector3f scale = { 1.0f };
	};

	class SceneObject
	{
	public:
		SceneObject() = default;
		~SceneObject() = default;

		SceneObject(const SceneObject& other) = default;


		Matrix4x4f GetTransform() const 
		{
			Matrix4x4f transform = Matrix4x4f::CreateIdentityMatrix();
			transform.SetRotation(myTRS.rotation);
			transform.SetPosition(myTRS.translation);
			transform.Scale(myTRS.scale);
			return transform; 
		}

		const TRS& GetTRS() const { return myTRS; }
		TRS& GetTRS() { return myTRS; }

		void SetTRS(const TRS& aTRS) { myTRS = aTRS; }
		void SetTransform(const Matrix4x4f& transform) 
		{
			Quaternionf q;
			transform.DecomposeMatrix(myTRS.translation, q, myTRS.scale);
			myTRS.rotation = q.GetYawPitchRoll();
		}

		const Vector3f& GetPosition() const { return myTRS.translation; }
		const Vector3f& GetEuler() const { return myTRS.rotation; }
		const Vector3f& GetScale() const { return myTRS.scale; }

		Vector3f& GetPosition() { return myTRS.translation; }
		Vector3f& GetEuler() { return myTRS.rotation; }
		Vector3f& GetScale() { return myTRS.scale; }

		void SetName(const char* aName) { myName = aName; }
		const char* GetName() const { return myName.c_str(); }
		void SetPath(Scene* aScene, StringId aPath);
		StringId GetPath() const { return myPath; }

		void SetSceneObjectDefintionName(StringId aSceneObjectDefinitionName) { mySceneObjectDefinitionName = aSceneObjectDefinitionName; }
		StringId GetSceneObjectDefinitionName() const { return mySceneObjectDefinitionName; };

		std::span<const SceneProperty> GetPropertyOverrides() const { return myPropertyOverrides; };
		std::vector<SceneProperty>& EditPropertyOverrides() { return myPropertyOverrides; };

		struct PropertySourceAndOveride
		{
			ScenePropertyDefinition source;
			SceneProperty override;
		};

		void CalculateEditablePropertySet(SceneObjectDefinitionManager& objectDefinitionManager, std::vector<PropertySourceAndOveride>& outResult) const;
		void CalculateCombinedPropertySet(SceneObjectDefinitionManager& objectDefinitionManager, std::vector<ScenePropertyDefinition>& outResult) const;

	private:
		StringId mySceneObjectDefinitionName;
		std::vector<SceneProperty> myPropertyOverrides;

		TRS myTRS = { { 0.0f }, { 0.0f }, { 1.0f } };
		std::string myName = "Scene Object";
		StringId myPath = {};

	};
}