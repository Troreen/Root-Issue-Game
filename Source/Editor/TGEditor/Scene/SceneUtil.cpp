#include "SceneUtil.h"

#include <tge/engine.h>
#include <tge/scene/Scene.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/texture/TextureManager.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/sprite/sprite.h>

#include <Editor.h>
#include <Tools/Viewport/Viewport.h>

#include <tge/drawers/LineDrawer.h>
#include <tge/primitives/LinePrimitive.h>
#include <tge/script/BaseProperties.h>

using namespace Tga;

static bool CheckFrustum(const Frustum& frustum, Vector3f center, float radius)
{
	Tga::Vector3f FrustumToPoint{};

	if (frustum.top.normal.Dot(center - frustum.top.pos) <= -radius)
	{
		return false;
	}
	if (frustum.right.normal.Dot(center - frustum.right.pos) <= -radius)
	{
		return false;
	}
	if (frustum.bottom.normal.Dot(center - frustum.bottom.pos) <= -radius)
	{
		return false;
	}
	if (frustum.left.normal.Dot(center - frustum.left.pos) <= -radius)
	{
		return false;
	}
	if (frustum.nearplane.normal.Dot(center - frustum.nearplane.pos) <= -radius)
	{
		return false;
	}
	if (frustum.farplane.normal.Dot(center - frustum.farplane.pos) <= -radius)
	{
		return false;
	}

	return true;
}

void Tga::SceneCache::ClearCache()
{
	myTextureCache.clear();
	myModelCache.clear();
}

std::shared_ptr<Model> Tga::SceneCache::GetModelUsingCache(StringId path)
{
	if (path.IsEmpty())
		return nullptr;

	std::shared_ptr<Model> model;

	auto cacheIt = myModelCache.find(path);
	if (cacheIt != myModelCache.end())
	{
		model = cacheIt->second;
	}
	else
	{
		model = ModelFactory::GetInstance().GetModel(path.GetString());
		myModelCache[path] = model;
	}

	return model;
}

Texture* Tga::SceneCache::GetTextureUsingCache(StringId path, TextureSrgbMode srgbMode)
{
	if (path.IsEmpty())
		return nullptr;

	Texture* texture = nullptr;

	auto cacheIt = myTextureCache.find(path);
	if (cacheIt != myTextureCache.end())
	{
		texture = cacheIt->second;
	}
	else
	{
		auto& engine = *Tga::Engine::GetInstance();
		auto& textureManager = engine.GetTextureManager();

		texture = textureManager.GetTexture(path.GetString(), srgbMode);
		myTextureCache[path] = texture;
	}

	return texture;
}

Scene* Tga::SceneCache::GetSceneUsingCache(StringId path)
{
	if (path.IsEmpty())
		return nullptr;

	Scene* scene = nullptr;

	auto cacheIt = mySceneCache.find(path);
	if (cacheIt != mySceneCache.end())
	{
		scene = cacheIt->second;
	}
	else
	{
		auto& editor = *Tga::Editor::GetEditor();
		auto& sceneManager = editor.GetEditorSceneManager();

		scene = sceneManager.Get(path.GetString());
		mySceneCache[path] = scene;
	}

	return scene;
}

bool Tga::CheckBounds(const Frustum& frustum, Tga::Matrix4x4f matrix, float maxScale, Model& model)
{
	int meshCount = (int)model.GetMeshCount();
	for (int i = 0; i < meshCount; i++)
	{
		const Tga::BoxSphereBounds& bounds = model.GetMeshData(i).Bounds;
		Vector3f transformedCenter = bounds.Center * matrix;

		if (CheckFrustum(frustum, transformedCenter, maxScale * bounds.Radius))
			return true;
	}

	return false;
}

// ugly workaround to avoid allocations per object when drawing
static int locScenePropertyDepth = -1;
static std::vector<std::vector<ScenePropertyDefinition>> locSceneObjectProperties;

void DrawBounds(const Tga::BoxSphereBounds& bounds, Tga::Vector4f color)
{
	auto& debug = Tga::Engine::GetInstance()->GetGraphicsEngine().GetLineDrawer();
	Tga::LinePrimitive primitive{};
	primitive.color = color;
	auto min = bounds.Center - bounds.BoxExtents;
	auto max = bounds.Center + bounds.BoxExtents;
	primitive.fromPosition = min;
	primitive.toPosition = { min.x, max.y, min.z };
	debug.Draw(primitive);
	primitive.fromPosition = primitive.toPosition;
	primitive.toPosition = { max.x, max.y, min.z };
	debug.Draw(primitive);
	primitive.fromPosition = primitive.toPosition;
	primitive.toPosition = { max.x, min.y, min.z };
	debug.Draw(primitive);
	primitive.fromPosition = primitive.toPosition;
	primitive.toPosition = { min.x, min.y, min.z };
	debug.Draw(primitive);

	primitive.fromPosition = { min.x, min.y, max.z };
	primitive.toPosition = { min.x, max.y, max.z };
	debug.Draw(primitive);
	primitive.fromPosition = primitive.toPosition;
	primitive.toPosition = { max.x, max.y, max.z };
	debug.Draw(primitive);
	primitive.fromPosition = primitive.toPosition;
	primitive.toPosition = { max.x, min.y, max.z };
	debug.Draw(primitive);
	primitive.fromPosition = primitive.toPosition;
	primitive.toPosition = { min.x, min.y, max.z };
	debug.Draw(primitive);

	primitive.fromPosition = { min.x, min.y, min.z };
	primitive.toPosition = { min.x, min.y, max.z };
	debug.Draw(primitive);
	primitive.fromPosition = { min.x, max.y, min.z };
	primitive.toPosition = { min.x, max.y, max.z };
	debug.Draw(primitive);
	primitive.fromPosition = { max.x, max.y, min.z };
	primitive.toPosition = { max.x, max.y, max.z };
	debug.Draw(primitive);
	primitive.fromPosition = { max.x, min.y, min.z };
	primitive.toPosition = { max.x, min.y, max.z };
	debug.Draw(primitive);
};

Frustum Tga::CalculateFrustum(const Camera& camera)
{	
	const Tga::Matrix4x4f& projection = camera.GetProjection();
	const Tga::Matrix4x4f& cameraToWorld = camera.GetTransform();

	Tga::Vector3f tlc = Tga::Vector3f{
		(-1.0f) / projection(1,1),
		(1.0f) / projection(2,2),
		1.f
	}.GetNormalized();

	Tga::Vector3f brc = Tga::Vector3f{
		(1.0f) / projection(1,1),
		(-1.0f) / projection(2,2),
		1.f
	}.GetNormalized();

	Tga::Vector3f trc = Tga::Vector3f{
		(1.0f) / projection(1,1),
		(1.0f) / projection(2,2),
		1.f
	}.GetNormalized();

	Tga::Vector3f blc = Tga::Vector3f{
		(-1.0f) / projection(1,1),
		(-1.0f) / projection(2,2),
		1.f
	}.GetNormalized();

	float nearPlane; float farPlane;
	camera.GetProjectionPlanes(nearPlane, farPlane);

	Tga::Vector3f tln = (nearPlane * tlc) * cameraToWorld;
	Tga::Vector3f brn = (nearPlane * brc) * cameraToWorld;
	Tga::Vector3f trn = (nearPlane * trc) * cameraToWorld;
	Tga::Vector3f bln = (nearPlane * blc) * cameraToWorld;

	Tga::Vector3f tlf = (farPlane * tlc) * cameraToWorld;
	Tga::Vector3f brf = (farPlane * brc) * cameraToWorld;
	Tga::Vector3f trf = (farPlane * trc) * cameraToWorld;
	Tga::Vector3f blf = (farPlane * blc) * cameraToWorld;

	Frustum frustum;

	{
		frustum.left.pos = { tln };
		frustum.left.normal = (tln - tlf).Cross(tln - bln).GetNormalized();
		frustum.right.pos = { trn };
		frustum.right.normal = (brn - trn).Cross(trf - trn).GetNormalized();
		frustum.top.pos = { tln };
		frustum.top.normal = (trn - tln).Cross(tlf - tln).GetNormalized();
		frustum.bottom.pos = { bln };
		frustum.bottom.normal = (brf - brn).Cross(brn - bln).GetNormalized();
		frustum.nearplane.pos = { bln };
		frustum.nearplane.normal = (brn - bln).Cross(trn - brn).GetNormalized();
		frustum.farplane.pos = { blf };
		frustum.farplane.normal = (trf - brf).Cross(brf - blf).GetNormalized();
	}
	

	return frustum;
}

bool Tga::DrawSceneProperty(const ScenePropertyDefinition& property, float maxScale, DrawParameters& drawParameters)
{
	auto& engine = *Tga::Engine::GetInstance();
	auto& graphicsStateStack = engine.GetGraphicsEngine().GetGraphicsStateStack();

	bool hasBeenRendered = false;
	if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneReference>>())
	{
		const SceneReference& value = property.value.Get<CopyOnWriteWrapper<SceneReference>>()->Get();

		Scene* scene = drawParameters.cache.GetSceneUsingCache(value.path);
		if (scene)
		{
			DrawScene(*scene, drawParameters);
		}

	}
	else if (drawParameters.useIdShader)
	{
		if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneModel>>())
		{
			StringId path = property.value.Get<CopyOnWriteWrapper<SceneModel>>()->Get().path;
			std::shared_ptr<Model> model = drawParameters.cache.GetModelUsingCache(path);

			if (model && CheckBounds(drawParameters.frustum, graphicsStateStack.GetTransform(), maxScale, *model))
			{
				ModelSpacePose* pose = nullptr;
				if (drawParameters.previewPoses)
				{
					auto it = drawParameters.previewPoses->find(property.name);

					if (it != drawParameters.previewPoses->end())
					{
						pose = &it->second;
					}
				}

				if (pose)
				{
					AnimatedModelInstance instance;
					instance.Init(model);
					instance.SetPose(*pose);
					Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().Draw(instance, drawParameters.viewport.GetIdAnimatedModelShader());
				}
				else
				{
					ModelInstance instance;
					instance.Init(model);
					Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().Draw(instance, drawParameters.viewport.GetIdModelShader());
				}
				hasBeenRendered = true;
			}
		}
		else if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneSprite>>())
		{
			const SceneSprite& value = property.value.Get<CopyOnWriteWrapper<SceneSprite>>()->Get();

			SpriteSharedData sharedData = {};
			sharedData.myTexture = drawParameters.cache.GetTextureUsingCache(value.textures[0], TextureSrgbMode::ForceSrgbFormat);
			sharedData.myCustomShader = &drawParameters.viewport.GetIdSpriteShader();

			Sprite2DInstanceData instance = {};
			instance.myPivot = value.pivot;
			instance.mySize = value.size;

			Tga::Engine::GetInstance()->GetGraphicsEngine().GetSpriteDrawer().Draw(sharedData, instance);

			hasBeenRendered = true;

		}
	}
	else
	{
	
		if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneModel>>())
		{
			const SceneModel& value = property.value.Get<CopyOnWriteWrapper<SceneModel>>()->Get();

			StringId path = value.path;
			std::shared_ptr<Model> model = drawParameters.cache.GetModelUsingCache(path);

			if (model && CheckBounds(drawParameters.frustum, graphicsStateStack.GetTransform(), maxScale, *model))
			{
				ModelSpacePose* pose = nullptr;
				if (drawParameters.previewPoses)
				{
					auto it = drawParameters.previewPoses->find(property.name);

					if (it != drawParameters.previewPoses->end())
					{
						pose = &it->second;
					}
				}

				if (pose)
				{
					AnimatedModelInstance instance;
					instance.Init(model);
					instance.SetPose(*pose);

					int meshCount = (int)instance.GetModel()->GetMeshCount();
					if (meshCount > MAX_MESHES_PER_MODEL)
						meshCount = MAX_MESHES_PER_MODEL;

					for (int i = 0; i < meshCount; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							if (!value.textures[i][j].IsEmpty())
							{
								// diffuse texture should be srgb, the rest not
								TextureSrgbMode srgbMode = (j == 0) ? TextureSrgbMode::ForceSrgbFormat : TextureSrgbMode::ForceNoSrgbFormat;
								Texture* texture = drawParameters.cache.GetTextureUsingCache(value.textures[i][j], srgbMode);

								if (texture != nullptr)
									instance.SetTexture(i, j, texture);
							}
						}
					}

					// todo override shader
					Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().Draw(instance);
				}
				else
				{
					ModelInstance instance;
					instance.Init(model);

					int meshCount = (int)instance.GetModel()->GetMeshCount();
					if (meshCount > MAX_MESHES_PER_MODEL)
						meshCount = MAX_MESHES_PER_MODEL;

					for (int i = 0; i < meshCount; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							if (!value.textures[i][j].IsEmpty())
							{
								// diffuse texture should be srgb, the rest not
								TextureSrgbMode srgbMode = (j == 0) ? TextureSrgbMode::ForceSrgbFormat : TextureSrgbMode::ForceNoSrgbFormat;
								Texture* texture = drawParameters.cache.GetTextureUsingCache(value.textures[i][j], srgbMode);

								if (texture != nullptr)
									instance.SetTexture(i, j, texture);
							}
						}
					}

					if (drawParameters.overrideModelShader)
					{
						Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().Draw(instance, *drawParameters.overrideModelShader);

					}
					else
					{
						Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().Draw(instance);
					}
				}

				if (drawParameters.drawBounds)
				{
					const BoxSphereBounds& bounds = model->GetMeshData(0).Bounds;
					DrawBounds(bounds, drawParameters.boundsColor);
				}

				hasBeenRendered = true;
			}
		}
		else if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneSprite>>())
		{
			const SceneSprite& value = property.value.Get<CopyOnWriteWrapper<SceneSprite>>()->Get();

			SpriteSharedData sharedData = {};
			sharedData.myTexture = drawParameters.cache.GetTextureUsingCache(value.textures[0], TextureSrgbMode::ForceSrgbFormat);
			sharedData.myMaps[0] = drawParameters.cache.GetTextureUsingCache(value.textures[1], TextureSrgbMode::ForceNoSrgbFormat);
			sharedData.myMaps[1] = drawParameters.cache.GetTextureUsingCache(value.textures[2], TextureSrgbMode::ForceNoSrgbFormat);
			sharedData.myMaps[2] = drawParameters.cache.GetTextureUsingCache(value.textures[3], TextureSrgbMode::ForceNoSrgbFormat);

			Sprite2DInstanceData instance = {};
			instance.myPivot = value.pivot;
			instance.mySize = value.size;
			Tga::Engine::GetInstance()->GetGraphicsEngine().GetSpriteDrawer().Draw(sharedData, instance);

			hasBeenRendered = true;
		}
	}

	return hasBeenRendered;
}

void Tga::DrawSceneObject(const SceneObject& sceneObject, DrawParameters& drawParameters)
{
	auto& engine = *Tga::Engine::GetInstance();
	auto& graphicsStateStack = engine.GetGraphicsEngine().GetGraphicsStateStack();
	SceneObjectDefinitionManager& sceneObjectDefinitionManager = Editor::GetEditor()->GetSceneObjectDefinitionManager();


	graphicsStateStack.Push();

	Matrix4x4f transform = sceneObject.GetTransform();
	graphicsStateStack.ApplyTransform(transform);

	Vector3f scale = sceneObject.GetScale();
	float maxScale = std::max(scale.x, std::max(scale.y, scale.z));

	locScenePropertyDepth++;

	if (locSceneObjectProperties.size() <= locScenePropertyDepth)
		locSceneObjectProperties.resize(locScenePropertyDepth + 1);

	sceneObject.CalculateCombinedPropertySet(sceneObjectDefinitionManager, locSceneObjectProperties[locScenePropertyDepth]);

	bool hasBeenRendered = false;
	for (ScenePropertyDefinition& property : locSceneObjectProperties[locScenePropertyDepth])
	{
		if (DrawSceneProperty(property, maxScale, drawParameters))
			hasBeenRendered = true;
	}

	if (!hasBeenRendered)
	{
		std::shared_ptr<Model> model = drawParameters.cache.GetModelUsingCache("models/locator.fbx"_tgaid);

		if (model && CheckBounds(drawParameters.frustum, graphicsStateStack.GetTransform(), maxScale, *model))
		{
			ModelInstance instance;
			instance.Init(model);

			if (drawParameters.useIdShader)
			{
				Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().Draw(instance, drawParameters.viewport.GetIdModelShader());
			}
			else
			{
				Tga::Engine::GetInstance()->GetGraphicsEngine().GetModelDrawer().Draw(instance);

				if (drawParameters.drawBounds)
				{
					const BoxSphereBounds& bounds = instance.GetModel()->GetMeshData(0).Bounds;
					DrawBounds(bounds, drawParameters.boundsColor);
				}
			}

		}
	}

	graphicsStateStack.Pop();

	locSceneObjectProperties[locScenePropertyDepth].clear();
	locScenePropertyDepth--;
}

void Tga::DrawScene(const Scene& scene, DrawParameters& drawParameters)
{
	for (auto& p : scene.GetSceneObjects())
	{
		DrawSceneObject(*p.second, drawParameters);
	}
}
