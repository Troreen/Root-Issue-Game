#pragma once

#include <unordered_map>
#include <tge/stringRegistry/StringRegistry.h>
#include <tge/texture/texture.h>
#include <tge/model/model.h>
#include <tge/math/Matrix4x4.h>
#include <tge/script/Property.h>

namespace Tga
{
	class EditorViewport;

	class Scene;
	class SceneObject;
	struct ScenePropertyDefinition;
	class ModelShader;
	class Camera;

	struct Plane
	{
		Tga::Vector3f pos;
		Tga::Vector3f normal;
	};
	struct PlaneRect
	{
		Tga::Vector3f tl, tr, br, bl;
	};
	struct Frustum
	{
		Tga::Plane top, right, bottom, left, nearplane, farplane;
		Tga::PlaneRect nearrect, farrect;
	};

	class SceneCache
	{
		std::unordered_map<StringId, Texture*> myTextureCache;
		std::unordered_map<StringId, std::shared_ptr<Model>> myModelCache;
		std::unordered_map<StringId, Scene*> mySceneCache;

	public:
		Texture* GetTextureUsingCache(StringId path, TextureSrgbMode srgbMode);
		std::shared_ptr<Model> GetModelUsingCache(StringId path);
		Scene* GetSceneUsingCache(StringId path);

		void ClearCache();
	};

	struct DrawParameters
	{
		bool useIdShader;
		bool drawBounds;
		Vector3f boundsColor;
		SceneCache& cache;
		Frustum& frustum;
		EditorViewport& viewport;
		ModelShader* overrideModelShader;

		std::unordered_map<StringId, ModelSpacePose>* previewPoses;
	};

	Frustum CalculateFrustum(const Camera& camera);

	bool DrawSceneProperty(const ScenePropertyDefinition& property, float maxScale, DrawParameters& drawParameters);
	void DrawSceneObject(const SceneObject& sceneObject, DrawParameters& drawParameters);
	void DrawScene(const Scene& scene, DrawParameters& drawParameters);

	bool CheckBounds(const Frustum& frustum, Tga::Matrix4x4f matrix, float maxScale, Model& model);

}