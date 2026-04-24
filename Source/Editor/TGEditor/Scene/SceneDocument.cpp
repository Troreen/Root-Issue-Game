#include "stdafx.h"

#include <commdlg.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include "SceneDocument.h"

#include <imguizmo/ImGuizmo.h>
#include <tge/input/InputManager.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/drawers/SpriteDrawer.h>

#include <tge/Model/ModelFactory.h>
#include <tge/Model/ModelInstance.h>
#include <tge/editor/CommandManager/CommandManager.h>
#include <tge/texture/TextureManager.h>
#include <tge/drawers/DebugDrawer.h>
#include <tge/graphics/DX11.h>
#include <tge/imgui/ImGuiInterface.h>
#include <tge/scene/SceneSerialize.h>
#include <tge/scene/SceneObjectDefinition.h>
#include <tge/scene/SceneObjectDefinitionManager.h>
#include <tge/scene/ScenePropertyTypes.h>

#include <tge/script/ScriptManager.h>
#include <tge/script/ScriptRuntimeInstance.h>
#include <tge/script/BaseProperties.h>

#include <Scene/SceneSelection.h>
#include <Scene/ActiveScene.h>

#include <Commands/AddSceneObjectsCommand.h>
#include <Commands/RemoveSceneObjectsCommand.h>

#include <Editor.h>

#include <Tools/Viewport/Viewport.h>
#include <Tools/ProjectRunControls/ProjectRunControls.h>
#include <FileDialog/FileDialog.h>
#include "imgui_widgets/imgui_widgets.h"
#include "imgui_internal.h" // for DockBuilder Api

#include <tge/script/Nodes/CommonNodes.h>
#include <tge/script/Nodes/ExampleNodes.h>
#include <tge/script/Nodes/CommonMathNodes.h>
#include <tge/script/Nodes/SceneObjectNodes.h>

#include <IconFontHeaders/IconsLucide.h>

#include <p4/p4.h>

#include <tge/drawers/LineDrawer.h>
#include <tge/primitives/LinePrimitive.h>

using namespace Tga;

namespace
{
	struct ObjectAssetEntry
	{
		std::filesystem::path relativePathFromObjects;
		std::string objectDefinitionName;
		std::string groupPath;
	};

	struct ObjectFootprint
	{
		float width;
		float depth;
	};

	struct FloatPropertyOverride
	{
		std::string propertyName;
		float value = 0.0f;
	};

	struct AnimationSpawnVariant
	{
		std::string nameSuffix;
		std::vector<FloatPropertyOverride> floatOverrides;
	};

	struct AnimationSpawnEvaluation
	{
		bool isAnimatedCandidate = false;
		bool usedFallbackDefault = false;
		std::vector<AnimationSpawnVariant> variants;
	};

	struct AssetZooSpawnEntry
	{
		ObjectAssetEntry asset;
		std::string nameSuffix;
		std::vector<FloatPropertyOverride> floatOverrides;
	};

	constexpr const char* kAnimationPreviewNameTag = "__anim_preview__";

	ObjectFootprint CalculateObjectFootprint(const std::string& objectDefinitionName)
	{
		constexpr float kFallbackSize = 300.0f;
		constexpr float kFootprintPadding = 100.0f;

		SceneObjectDefinitionManager& definitionManager = Editor::GetEditor()->GetSceneObjectDefinitionManager();
		SceneObjectDefinition* definition =
			definitionManager.Get(StringRegistry::RegisterOrGetString(objectDefinitionName.c_str()));

		if (definition == nullptr)
		{
			return { kFallbackSize, kFallbackSize };
		}

		const PropertyTypeBase* modelPropertyType = GetPropertyType<CopyOnWriteWrapper<SceneModel>>();

		float maxHalfWidth = 0.0f;
		float maxHalfDepth = 0.0f;
		bool foundValidModel = false;

		for (const ScenePropertyDefinition& property : definition->GetProperties())
		{
			if (property.type != modelPropertyType)
			{
				continue;
			}

			const auto* modelWrapper = property.value.Get<CopyOnWriteWrapper<SceneModel>>();
			if (modelWrapper == nullptr)
			{
				continue;
			}

			const SceneModel& sceneModel = modelWrapper->Get();
			if (sceneModel.path.IsEmpty())
			{
				continue;
			}

			std::shared_ptr<Model> model = ModelFactory::GetInstance().GetModel(sceneModel.path.GetString());
			if (!model || model->GetMeshCount() == 0)
			{
				continue;
			}

			for (size_t meshIndex = 0; meshIndex < model->GetMeshCount(); ++meshIndex)
			{
				const Tga::BoxSphereBounds& bounds = model->GetMeshData(static_cast<unsigned int>(meshIndex)).Bounds;

				const float halfWidth = std::abs(bounds.Center.x) + bounds.BoxExtents.x;
				const float halfDepth = std::abs(bounds.Center.z) + bounds.BoxExtents.z;

				maxHalfWidth = std::max(maxHalfWidth, halfWidth);
				maxHalfDepth = std::max(maxHalfDepth, halfDepth);
			}

			foundValidModel = true;
		}

		if (!foundValidModel)
		{
			return { kFallbackSize, kFallbackSize };
		}

		const float width = std::max(kFallbackSize, maxHalfWidth * 2.0f + kFootprintPadding);
		const float depth = std::max(kFallbackSize, maxHalfDepth * 2.0f + kFootprintPadding);

		return { width, depth };
	}

	std::string ToLowerAscii(std::string value)
	{
		std::transform(
			value.begin(),
			value.end(),
			value.begin(),
			[](unsigned char c)
			{
				return static_cast<char>(std::tolower(c));
			});

		return value;
	}

	std::string BuildObjectNameBase(const std::filesystem::path& relativePathFromObjects)
	{
		std::filesystem::path withoutExtension = relativePathFromObjects;
		withoutExtension.replace_extension();

		std::string name = withoutExtension.generic_string();
		for (char& c : name)
		{
			const bool isAlphaNum =
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9');

			if (!isAlphaNum)
			{
				c = '_';
			}
		}

		if (name.empty())
		{
			name = "SceneObject";
		}

		return name;
	}

	std::string MakeUniqueName(const std::string& baseName, std::unordered_set<std::string>& usedNames)
	{
		std::string candidate = baseName;
		if (usedNames.find(candidate) == usedNames.end())
		{
			usedNames.insert(candidate);
			return candidate;
		}

		int suffix = 1;
		while (true)
		{
			candidate = baseName + "(" + std::to_string(suffix) + ")";
			if (usedNames.find(candidate) == usedNames.end())
			{
				usedNames.insert(candidate);
				return candidate;
			}

			++suffix;
		}
	}

	bool StartsWith(const std::string& value, const char* prefix)
	{
		const size_t prefixLength = std::strlen(prefix);
		if (value.size() < prefixLength)
		{
			return false;
		}

		return std::equal(prefix, prefix + prefixLength, value.begin());
	}

	std::string SanitizeNameToken(std::string value)
	{
		for (char& c : value)
		{
			const bool isAlphaNum =
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9');

			if (!isAlphaNum)
			{
				c = '_';
			}
		}

		if (value.empty())
		{
			return "variant";
		}

		return value;
	}

	std::string ClipStemFromPropertyName(const std::string& clipPropertyName)
	{
		if (StartsWith(clipPropertyName, "clip_"))
		{
			return clipPropertyName.substr(5);
		}

		return clipPropertyName;
	}

	void SetFloatOverride(std::vector<FloatPropertyOverride>& inOutOverrides, const std::string& propertyName, float value)
	{
		for (FloatPropertyOverride& overrideValue : inOutOverrides)
		{
			if (overrideValue.propertyName == propertyName)
			{
				overrideValue.value = value;
				return;
			}
		}

		inOutOverrides.push_back({ propertyName, value });
	}

	bool TryBuildAnimationSpawnEvaluation(const std::string& objectDefinitionName, AnimationSpawnEvaluation& outEvaluation)
	{
		outEvaluation = AnimationSpawnEvaluation();

		SceneObjectDefinitionManager& definitionManager = Editor::GetEditor()->GetSceneObjectDefinitionManager();
		SceneObjectDefinition* definition =
			definitionManager.Get(StringRegistry::RegisterOrGetString(objectDefinitionName.c_str()));

		if (definition == nullptr)
		{
			return false;
		}

		const PropertyTypeBase* floatPropertyType = GetPropertyType<float>();
		const PropertyTypeBase* animationClipPropertyType = GetPropertyType<CopyOnWriteWrapper<AnimationClipReference>>();

		bool hasAnimationGraphProperty = false;
		bool hasAnimSpeed = false;
		std::vector<std::string> clipPropertyNames;
		std::vector<std::string> clipStems;
		std::unordered_set<std::string> perInstanceFloatProperties;

		for (const ScenePropertyDefinition& property : definition->GetProperties())
		{
			const std::string propertyName = property.name.GetString();

			if (propertyName == "animation_graph" || propertyName == "animationGraph")
			{
				hasAnimationGraphProperty = true;
			}

			if (propertyName == "anim_speed"
				&& (property.flags & ScenePropertyFlags::IsPerInstance) != ScenePropertyFlags::None
				&& property.type == floatPropertyType)
			{
				hasAnimSpeed = true;
			}

			if ((property.flags & ScenePropertyFlags::IsPerInstance) != ScenePropertyFlags::None
				&& property.type == floatPropertyType)
			{
				perInstanceFloatProperties.insert(propertyName);
			}

			if (StartsWith(propertyName, "clip_") && property.type == animationClipPropertyType)
			{
				clipPropertyNames.push_back(propertyName);
				clipStems.push_back(ClipStemFromPropertyName(propertyName));
			}
		}

		if (!hasAnimationGraphProperty || clipPropertyNames.empty())
		{
			return false;
		}

		outEvaluation.isAnimatedCandidate = true;

		std::vector<std::string> weightProperties;
		weightProperties.reserve(clipStems.size());

		size_t requiredWeightProperties = 0;
		size_t foundWeightProperties = 0;

		for (const std::string& clipStem : clipStems)
		{
			const std::string weightPropertyName = "w_" + clipStem;
			if (perInstanceFloatProperties.find(weightPropertyName) != perInstanceFloatProperties.end())
			{
				weightProperties.push_back(weightPropertyName);
			}

			if (ToLowerAscii(clipStem) == "idle")
			{
				continue;
			}

			++requiredWeightProperties;
			if (perInstanceFloatProperties.find(weightPropertyName) != perInstanceFloatProperties.end())
			{
				++foundWeightProperties;
			}
		}

		const bool canUseWeightScheme =
			requiredWeightProperties > 0 && requiredWeightProperties == foundWeightProperties;

		if (canUseWeightScheme)
		{
			outEvaluation.variants.reserve(clipStems.size());

			for (const std::string& clipStem : clipStems)
			{
				AnimationSpawnVariant variant;
				variant.nameSuffix = SanitizeNameToken(clipStem);

				for (const std::string& weightProperty : weightProperties)
				{
					SetFloatOverride(variant.floatOverrides, weightProperty, 0.0f);
				}

				const std::string selectedWeightProperty = "w_" + clipStem;
				if (perInstanceFloatProperties.find(selectedWeightProperty) != perInstanceFloatProperties.end())
				{
					SetFloatOverride(variant.floatOverrides, selectedWeightProperty, 1.0f);
				}

				if (hasAnimSpeed)
				{
					SetFloatOverride(variant.floatOverrides, "anim_speed", 1.0f);
				}

				outEvaluation.variants.push_back(std::move(variant));
			}

			return true;
		}

		const bool hasMoveBlend = perInstanceFloatProperties.find("move_blend") != perInstanceFloatProperties.end();
		const bool hasRunBlend = perInstanceFloatProperties.find("run_blend") != perInstanceFloatProperties.end();
		const bool hasAttackWeight = perInstanceFloatProperties.find("attack_weight") != perInstanceFloatProperties.end();

		const bool canUseLocomotionScheme = hasMoveBlend && (hasRunBlend || hasAttackWeight);
		if (canUseLocomotionScheme)
		{
			std::vector<AnimationSpawnVariant> locomotionVariants;
			locomotionVariants.reserve(clipStems.size());

			bool hasUnmappedClip = false;
			for (const std::string& clipStem : clipStems)
			{
				const std::string clipStemLower = ToLowerAscii(clipStem);
				float moveBlend = 0.0f;
				float runBlend = 0.0f;
				float attackWeight = 0.0f;
				bool mapped = false;

				if (clipStemLower.find("idle") != std::string::npos)
				{
					mapped = true;
				}
				else if (clipStemLower.find("walk") != std::string::npos)
				{
					moveBlend = 1.0f;
					mapped = true;
				}
				else if (clipStemLower.find("run") != std::string::npos
					|| clipStemLower.find("jog") != std::string::npos
					|| clipStemLower.find("sprint") != std::string::npos)
				{
					if (!hasRunBlend)
					{
						hasUnmappedClip = true;
						break;
					}

					moveBlend = 1.0f;
					runBlend = 1.0f;
					mapped = true;
				}
				else if (clipStemLower.find("attack") != std::string::npos
					|| clipStemLower.find("hit") != std::string::npos
					|| clipStemLower.find("strike") != std::string::npos)
				{
					if (!hasAttackWeight)
					{
						hasUnmappedClip = true;
						break;
					}

					attackWeight = 1.0f;
					mapped = true;
				}

				if (!mapped)
				{
					hasUnmappedClip = true;
					break;
				}

				AnimationSpawnVariant variant;
				variant.nameSuffix = SanitizeNameToken(clipStem);

				if (hasMoveBlend)
				{
					SetFloatOverride(variant.floatOverrides, "move_blend", moveBlend);
				}

				if (hasRunBlend)
				{
					SetFloatOverride(variant.floatOverrides, "run_blend", runBlend);
				}

				if (hasAttackWeight)
				{
					SetFloatOverride(variant.floatOverrides, "attack_weight", attackWeight);
				}

				if (hasAnimSpeed)
				{
					SetFloatOverride(variant.floatOverrides, "anim_speed", 1.0f);
				}

				locomotionVariants.push_back(std::move(variant));
			}

			if (!hasUnmappedClip && locomotionVariants.size() == clipStems.size())
			{
				outEvaluation.variants = std::move(locomotionVariants);
				return true;
			}
		}

		outEvaluation.usedFallbackDefault = true;

		AnimationSpawnVariant fallbackVariant;
		fallbackVariant.nameSuffix = "default";
		if (hasAnimSpeed)
		{
			SetFloatOverride(fallbackVariant.floatOverrides, "anim_speed", 1.0f);
		}

		outEvaluation.variants.push_back(std::move(fallbackVariant));
		return true;
	}
}

void Tga::SceneP4Handler(SceneFileChangeType aChangeType, const char* aPath)
{
	switch (aChangeType)
	{
	case SceneFileChangeType::Add:
		P4::MarkFileForAdd(aPath);
		break;
	case SceneFileChangeType::Modify:
		P4::CheckoutFile(aPath);
		break;
	case SceneFileChangeType::Delete:
		P4::MarkFileForDelete(aPath);
		break;
	}
}




//SceneDocument::~SceneDocument()
void SceneDocument::Close()
{

}

void SceneDocument::Init(std::string_view path)
{
	Document::Init(path);
	//myNavmeshCreationTool.Init();

	myViewport.Init();
	myViewport.GetGrid().SetGridLineExtreme(2000.0f);

	myScene = Editor::GetEditor()->GetEditorSceneManager().Get(path);


	char buffer[512];
	char asterix[2] = {0, 0};

	sprintf_s(buffer, "%s%s###Document:%s", myScene->GetName(), asterix, myScene->GetPath());
	myImGuiName = StringRegistry::RegisterOrGetString(buffer);

	sprintf_s(buffer, "Viewport##Document:%s", path.data());
	myPanelWindowNames[(size_t)Panels::Viewport] = buffer;
	sprintf_s(buffer, "Properties##Document:%s", path.data());
	myPanelWindowNames[(size_t)Panels::Properties] = buffer;
	sprintf_s(buffer, "Instances##Document:%s", path.data());
	myPanelWindowNames[(size_t)Panels::Instances] = buffer;
	sprintf_s(buffer, "Tool Settings##Document:%s", path.data());
	myPanelWindowNames[(size_t)Panels::ToolSettings] = buffer;
	//sprintf_s(buffer, "Navmesh Creation##Document:%s", path.data());
	//myPanelWindowNames[(size_t)Panels::NavmeshCreationTool] = buffer;

	Camera& camera = myViewport.GetCamera();
	Vector2i resolution = myViewport.GetViewportSize();
	camera.SetPerspectiveProjection(
		60,
		{
			(float)resolution.x,
			(float)resolution.y
		},
		0.1f,
		50000.0f
	);

	Vector3f cameraRotation = { 45, 45, 0 };
	
	camera.GetTransform().SetRotation(cameraRotation);
	myViewport.SetCameraRotation(cameraRotation);
	camera.GetTransform().SetPosition((camera.GetTransform().GetForward() * -myViewport.GetCameraFocusDistance()));
}

void SceneDocument::Save()
{
	SaveScene(*myScene, SceneP4Handler);
	mySaveUndoStackSize = myUndoStackSize;
}

void SceneDocument::PopulateSceneFromObjectAssets(bool aAnimationClipZoo)
{
	const std::filesystem::path objectsRoot = Tga::Settings::GameAssetRoot() / "Objects";
	if (!std::filesystem::exists(objectsRoot) || !std::filesystem::is_directory(objectsRoot))
	{
		std::cout << "[AssetZoo] Could not populate scene. Objects folder was not found at: "
			<< objectsRoot.string() << "\n";
		return;
	}

	std::vector<ObjectAssetEntry> discoveredAssets;
	std::unordered_map<std::string, int> definitionNameCounts;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(objectsRoot))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const std::string extension = ToLowerAscii(entry.path().extension().string());
		if (extension != ".tgo")
		{
			continue;
		}

		std::filesystem::path relativePath = std::filesystem::relative(entry.path(), objectsRoot);
		std::string groupPath = relativePath.parent_path().generic_string();
		if (groupPath == ".")
		{
			groupPath.clear();
		}

		ObjectAssetEntry asset = {
			.relativePathFromObjects = relativePath,
			.objectDefinitionName = entry.path().stem().string(),
			.groupPath = groupPath
		};

		discoveredAssets.push_back(std::move(asset));
		definitionNameCounts[discoveredAssets.back().objectDefinitionName]++;
	}

	std::sort(
		discoveredAssets.begin(),
		discoveredAssets.end(),
		[](const ObjectAssetEntry& left, const ObjectAssetEntry& right)
		{
			return left.relativePathFromObjects.generic_string() < right.relativePathFromObjects.generic_string();
		});

	std::unordered_set<std::string> duplicateDefinitionNames;
	for (const auto& [definitionName, count] : definitionNameCounts)
	{
		if (count > 1)
		{
			duplicateDefinitionNames.insert(definitionName);
		}
	}

	std::map<std::string, std::vector<AssetZooSpawnEntry>> groupedAssets;
	size_t skippedDuplicateAssets = 0;
	size_t skippedNonAnimatedAssets = 0;
	size_t animatedCandidateAssets = 0;
	size_t ambiguousAnimationDefaults = 0;

	for (const ObjectAssetEntry& asset : discoveredAssets)
	{
		if (duplicateDefinitionNames.find(asset.objectDefinitionName) != duplicateDefinitionNames.end())
		{
			++skippedDuplicateAssets;
			continue;
		}

		if (!aAnimationClipZoo)
		{
			AssetZooSpawnEntry spawnEntry;
			spawnEntry.asset = asset;
			groupedAssets[asset.groupPath].push_back(std::move(spawnEntry));
			continue;
		}

		AnimationSpawnEvaluation animationEvaluation;
		if (!TryBuildAnimationSpawnEvaluation(asset.objectDefinitionName, animationEvaluation)
			|| !animationEvaluation.isAnimatedCandidate)
		{
			++skippedNonAnimatedAssets;
			continue;
		}

		++animatedCandidateAssets;
		if (animationEvaluation.usedFallbackDefault)
		{
			++ambiguousAnimationDefaults;
		}

		std::vector<AssetZooSpawnEntry>& groupEntries = groupedAssets[asset.groupPath];
		for (const AnimationSpawnVariant& variant : animationEvaluation.variants)
		{
			AssetZooSpawnEntry spawnEntry;
			spawnEntry.asset = asset;
			spawnEntry.nameSuffix = variant.nameSuffix;
			spawnEntry.floatOverrides = variant.floatOverrides;
			groupEntries.push_back(std::move(spawnEntry));
		}
	}

	size_t totalSpawnEntries = 0;
	for (const auto& groupedEntry : groupedAssets)
	{
		totalSpawnEntries += groupedEntry.second.size();
	}

	std::vector<std::shared_ptr<SceneObject>> generatedObjects;
	generatedObjects.reserve(totalSpawnEntries);

	std::unordered_set<std::string> usedNames;
	const PropertyTypeBase* floatPropertyType = GetPropertyType<float>();

	struct GroupLayout
	{
		std::string groupPath;
		StringId folderPath;
		std::vector<AssetZooSpawnEntry> assets;
		std::vector<float> columnWidths;
		std::vector<float> rowDepths;
		std::vector<float> columnStarts;
		std::vector<float> rowStarts;
		int columns = 0;
		int rowCount = 0;
		float width = 0.0f;
		float depth = 0.0f;
	};

	constexpr int kMinColumns = 1;
	constexpr int kMaxColumns = 12;
	constexpr float kColumnGap = 100.0f;
	constexpr float kRowGap = 100.0f;
	constexpr float kGroupGap = 75.0f;

	std::unordered_map<std::string, ObjectFootprint> footprintCache;
	std::vector<GroupLayout> groupLayouts;
	groupLayouts.reserve(groupedAssets.size());

	float maxGroupWidth = 0.0f;
	float maxGroupDepth = 0.0f;

	for (const auto& [groupPath, assetsInGroup] : groupedAssets)
	{
		if (assetsInGroup.empty())
		{
			continue;
		}

		GroupLayout groupLayout;
		groupLayout.groupPath = groupPath;
		groupLayout.assets = assetsInGroup;

		std::string sceneFolderPath = "Objects";
		if (!groupPath.empty())
		{
			sceneFolderPath += "/" + groupPath;
		}

		groupLayout.folderPath = StringRegistry::RegisterOrGetString(sceneFolderPath.c_str());

		const int preferredColumns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(groupLayout.assets.size()))));
		groupLayout.columns = std::max(kMinColumns, std::min(kMaxColumns, preferredColumns));

		groupLayout.rowCount = static_cast<int>((groupLayout.assets.size() + static_cast<size_t>(groupLayout.columns) - 1) /
			static_cast<size_t>(groupLayout.columns));

		std::vector<ObjectFootprint> footprints;
		footprints.reserve(groupLayout.assets.size());

		groupLayout.columnWidths.assign(static_cast<size_t>(groupLayout.columns), 0.0f);
		groupLayout.rowDepths.assign(static_cast<size_t>(groupLayout.rowCount), 0.0f);

		for (size_t i = 0; i < groupLayout.assets.size(); ++i)
		{
			const AssetZooSpawnEntry& spawnEntry = groupLayout.assets[i];
			const ObjectAssetEntry& asset = spawnEntry.asset;

			const auto cacheIt = footprintCache.find(asset.objectDefinitionName);
			ObjectFootprint footprint = {};
			if (cacheIt != footprintCache.end())
			{
				footprint = cacheIt->second;
			}
			else
			{
				footprint = CalculateObjectFootprint(asset.objectDefinitionName);
				footprintCache[asset.objectDefinitionName] = footprint;
			}

			footprints.push_back(footprint);

			const int row = static_cast<int>(i / static_cast<size_t>(groupLayout.columns));
			const int column = static_cast<int>(i % static_cast<size_t>(groupLayout.columns));

			groupLayout.columnWidths[static_cast<size_t>(column)] =
				std::max(groupLayout.columnWidths[static_cast<size_t>(column)], footprint.width);
			groupLayout.rowDepths[static_cast<size_t>(row)] =
				std::max(groupLayout.rowDepths[static_cast<size_t>(row)], footprint.depth);
		}

		groupLayout.columnStarts.assign(static_cast<size_t>(groupLayout.columns), 0.0f);
		for (size_t column = 1; column < groupLayout.columnStarts.size(); ++column)
		{
			groupLayout.columnStarts[column] =
				groupLayout.columnStarts[column - 1] + groupLayout.columnWidths[column - 1] + kColumnGap;
		}

		groupLayout.rowStarts.assign(static_cast<size_t>(groupLayout.rowCount), 0.0f);
		for (size_t row = 1; row < groupLayout.rowStarts.size(); ++row)
		{
			groupLayout.rowStarts[row] =
				groupLayout.rowStarts[row - 1] + groupLayout.rowDepths[row - 1] + kRowGap;
		}

		groupLayout.width = groupLayout.columnStarts.back() + groupLayout.columnWidths.back();
		groupLayout.depth = groupLayout.rowStarts.back() + groupLayout.rowDepths.back();

		maxGroupWidth = std::max(maxGroupWidth, groupLayout.width);
		maxGroupDepth = std::max(maxGroupDepth, groupLayout.depth);

		groupLayouts.push_back(std::move(groupLayout));
	}

	if (!groupLayouts.empty())
	{
		const int groupCount = static_cast<int>(groupLayouts.size());
		const int groupGridColumns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(groupCount)))));
		const int groupGridRows = (groupCount + groupGridColumns - 1) / groupGridColumns;

		const float groupCellWidth = maxGroupWidth + kGroupGap;
		const float groupCellDepth = maxGroupDepth + kGroupGap;

		for (int groupIndex = 0; groupIndex < groupCount; ++groupIndex)
		{
			GroupLayout& groupLayout = groupLayouts[static_cast<size_t>(groupIndex)];

			const int groupRow = groupIndex / groupGridColumns;
			const int groupColumn = groupIndex % groupGridColumns;

			const float groupCenterX =
				(static_cast<float>(groupColumn) - (static_cast<float>(groupGridColumns) - 1.0f) * 0.5f) * groupCellWidth;
			const float groupCenterZ =
				(static_cast<float>(groupRow) - (static_cast<float>(groupGridRows) - 1.0f) * 0.5f) * groupCellDepth;

			for (size_t i = 0; i < groupLayout.assets.size(); ++i)
			{
				const AssetZooSpawnEntry& spawnEntry = groupLayout.assets[i];
				const ObjectAssetEntry& asset = spawnEntry.asset;

				std::string nameBase = BuildObjectNameBase(asset.relativePathFromObjects);
				if (aAnimationClipZoo)
				{
					nameBase += "_";
					nameBase += SanitizeNameToken(spawnEntry.nameSuffix.empty() ? "default" : spawnEntry.nameSuffix);
					nameBase += kAnimationPreviewNameTag;
				}

				const std::string uniqueName = MakeUniqueName(nameBase, usedNames);

				auto sceneObject = std::make_shared<SceneObject>();
				sceneObject->SetName(uniqueName.c_str());
				sceneObject->SetSceneObjectDefintionName(StringRegistry::RegisterOrGetString(asset.objectDefinitionName.c_str()));
				sceneObject->SetPath(nullptr, groupLayout.folderPath);

				const int row = static_cast<int>(i / static_cast<size_t>(groupLayout.columns));
				const int column = static_cast<int>(i % static_cast<size_t>(groupLayout.columns));

				const float localX =
					groupLayout.columnStarts[static_cast<size_t>(column)] +
					groupLayout.columnWidths[static_cast<size_t>(column)] * 0.5f -
					groupLayout.width * 0.5f;

				const float localZ =
					groupLayout.rowStarts[static_cast<size_t>(row)] +
					groupLayout.rowDepths[static_cast<size_t>(row)] * 0.5f -
					groupLayout.depth * 0.5f;

				sceneObject->GetTRS().translation =
				{
					groupCenterX + localX,
					0.0f,
					groupCenterZ + localZ
				};

				sceneObject->GetTRS().rotation = { 0.0f, 0.0f, 0.0f };
				sceneObject->GetTRS().scale = { 1.0f, 1.0f, 1.0f };

				if (!spawnEntry.floatOverrides.empty())
				{
					std::vector<SceneProperty>& propertyOverrides = sceneObject->EditPropertyOverrides();
					propertyOverrides.reserve(propertyOverrides.size() + spawnEntry.floatOverrides.size());

					for (const FloatPropertyOverride& floatOverride : spawnEntry.floatOverrides)
					{
						SceneProperty overrideProperty;
						overrideProperty.name = StringRegistry::RegisterOrGetString(floatOverride.propertyName.c_str());
						overrideProperty.type = floatPropertyType;
						overrideProperty.value = Property::Create<float>(floatOverride.value);
						propertyOverrides.push_back(std::move(overrideProperty));
					}
				}

				generatedObjects.push_back(std::move(sceneObject));
			}
		}
	}

	std::vector<uint32_t> existingObjectIds;
	existingObjectIds.reserve(myScene->GetSceneObjects().size());
	for (const auto& pair : myScene->GetSceneObjects())
	{
		existingObjectIds.push_back(pair.first);
	}

	if (!existingObjectIds.empty())
	{
		std::shared_ptr<RemoveSceneObjectsCommand> removeCommand = std::make_shared<RemoveSceneObjectsCommand>();
		removeCommand->AddObjects(std::span<const uint32_t>(existingObjectIds));
		CommandManager::DoCommand(removeCommand);
	}

	SceneSelection::GetActiveSceneSelection()->ClearSelection();

	if (!generatedObjects.empty())
	{
		std::shared_ptr<AddSceneObjectsCommand> addCommand = std::make_shared<AddSceneObjectsCommand>();
		addCommand->AddObjects(std::span<std::shared_ptr<SceneObject>>(generatedObjects));
		CommandManager::DoCommand(addCommand);

		for (const auto& pair : addCommand->GetObjects())
		{
			SceneSelection::GetActiveSceneSelection()->AddToSelection(pair.first);
		}
	}

	mySceneObjectList.SetSceneDirty();

	if (aAnimationClipZoo)
	{
		std::cout
			<< "[AssetZoo] Animation clip zoo found " << discoveredAssets.size() << " .tgo assets. "
			<< "Animated candidates: " << animatedCandidateAssets << ". "
			<< "Spawned " << generatedObjects.size() << " preview scene objects in " << groupedAssets.size() << " groups.";

		if (ambiguousAnimationDefaults > 0)
		{
			std::cout << " Used default variant fallback for " << ambiguousAnimationDefaults << " animated definitions.";
		}

		if (skippedNonAnimatedAssets > 0)
		{
			std::cout << " Skipped " << skippedNonAnimatedAssets << " non-animated assets.";
		}
	}
	else
	{
		std::cout
			<< "[AssetZoo] Found " << discoveredAssets.size() << " .tgo assets. "
			<< "Spawned " << generatedObjects.size() << " scene objects in " << groupedAssets.size() << " groups.";
	}

	if (skippedDuplicateAssets > 0)
	{
		std::vector<std::string> duplicateNames(duplicateDefinitionNames.begin(), duplicateDefinitionNames.end());
		std::sort(duplicateNames.begin(), duplicateNames.end());

		std::cout << " Skipped " << skippedDuplicateAssets
			<< " assets due to duplicate object-definition names: ";
		for (size_t i = 0; i < duplicateNames.size(); ++i)
		{
			if (i > 0)
			{
				std::cout << ", ";
			}
			std::cout << duplicateNames[i];
		}
		std::cout << ".";
	}

	std::cout << "\n";
}

void SceneDocument::Update(float aTimeDelta, InputManager& inputManager)
{
	aTimeDelta; inputManager;

	assert(GetActiveScene() == nullptr);
	SetActiveScene(myScene);
	assert(SceneSelection::GetActiveSceneSelection() == nullptr);
	SceneSelection::SetActiveSceneSelection(&mySceneSelection);

	// clearing out caches every frame to support updates to assets while the editor is running
	myCache.ClearCache();

	const Camera& renderCamera = myViewport.GetCamera();
	Frustum frustum = CalculateFrustum(renderCamera);
	
	{
		myViewport.BeginDraw();

		std::vector<ScenePropertyDefinition> sceneObjectProperties;

		{ // One pass to render ID
			myViewport.SetupIdPass();

			DrawParameters drawParameters = {
				.useIdShader = true,
				.drawBounds = false,
				.boundsColor = {},
				.cache = myCache,
				.frustum = frustum,
				.viewport = myViewport,
				.overrideModelShader = nullptr
			};

			for (auto& p : GetActiveScene()->GetSceneObjects())
			{
				auto& info = P4::GetFileInfo(myScene->GetObjectFilePath(p.first).GetString());

				myViewport.SetObjectAndSelectionId(
					p.first,
					SceneSelection::GetActiveSceneSelection()->Contains(p.first) ? p.first : 0,
					info
				);

				DrawSceneObject(*p.second, drawParameters);
			}
		}

		{ 
			// And one pass to render to editor render-target
			myViewport.SetupColorPass();
	
			DrawParameters drawParameters = {
				.useIdShader=false,
				.drawBounds=false,
				.boundsColor={},
				.cache=myCache,
				.frustum=frustum,
				.viewport=myViewport,
				.overrideModelShader=nullptr
			};


			for (auto& p : GetActiveScene()->GetSceneObjects())
			{
				drawParameters.boundsColor = Tga::Vector4f(0.f, 1.f, 0.f, 1.f);
				if (ImGui::GetIO().KeyShift)
				{
					drawParameters.boundsColor = SceneSelection::GetActiveSceneSelection()->Contains(p.first) ? Tga::Vector4f(0.f, 0.f, 0.f, 0.0f) : Tga::Vector4f(0.f, 1.f, 0.f, 1.f);
				}

				DrawSceneObject(*p.second, drawParameters);
			}
		}
	}

	myViewport.EndDraw();


	char buffer[512];
	char asterix[2] = {0, 0};

	// Todo: all of this base imgui stuff should move to the Document base class
	// Todo: it seems like ImGui figures out the name when calling ImGui::SetWindowFocus (in Editor when trying to create an already open document). Perhaps myImGuiName should actually be updated?
	if (mySaveUndoStackSize != myUndoStackSize)
		asterix[0] = '*';

	sprintf_s(buffer, "%s%s###Document:%s", myScene->GetName(), asterix, myScene->GetPath());

	if (!myIsDockingInitialized)
	{
		ImGui::DockBuilderSetNodeSize(Editor::GetEditor()->GetDocumentDockSpaceId(), Editor::GetEditor()->GetDocumentDockSpaceSize());
		ImGui::DockBuilderDockWindow(buffer, Editor::GetEditor()->GetDocumentDockSpaceId());

		ImGui::DockBuilderFinish(Editor::GetEditor()->GetDocumentDockSpaceId());
	}

	ImGui::SetNextWindowClass(Editor::GetEditor()->GetDocumentWindowClass());
	ImGui::SetNextWindowDockID(Editor::GetEditor()->GetDocumentDockSpaceId(), ImGuiCond_Once);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);


	bool open = true;
	ImGui::Begin(buffer, &open);
	if (myState == Document::State::Open && !open)
	{
		myState = Document::State::CloseRequested;
	}
	{
		// Todo: move this out so it can be reused between documents

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 0));

		ImGui::PushFont(ImGuiInterface::GetIconFontLarge());

		// not quite sure why exactly these numbers are needed, but fixes padding
		ImGui::SetCursorPosX(6);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);

		// Add half of CellPadding to make positions of first icon more consistent
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

		if (ImGui::BeginTable("Toolbar", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImVec2 toolbarItemSize = ImVec2(26, 28);

			if (ImGui::Selectable(ICON_LC_SAVE_ALL, false, 0, toolbarItemSize))
			{
				Editor::GetEditor()->Save();
			}

			ImGui::TableSetColumnIndex(1);

			if (ImGui::Selectable(ICON_LC_PLAY, false, 0, toolbarItemSize) || ImGui::IsKeyPressed(ImGuiKey_F5))
			{
				ProjectRunControls::ExecuteRun(nullptr);
			}

			ImGui::SameLine();

			if (ImGui::Selectable(ICON_LC_FAST_FORWARD, false, 0, toolbarItemSize) || ImGui::IsKeyPressed(ImGuiKey_F6))
			{
				ProjectRunControls::ExecuteRun(this);
			}

			ImGui::TableSetColumnIndex(2);

			{
				Gizmos& gizmos = myViewport.GetGizmos();
				bool isMoveToolActive = gizmos.GetCurrentOperation() == ImGuizmo::TRANSLATE;
				if (ImGui::Selectable(ICON_LC_MOVE_3D, isMoveToolActive, 0, toolbarItemSize))
				{
					gizmos.SetCurrentOperation(isMoveToolActive ? 0 : ImGuizmo::TRANSLATE);
				}
				ImGui::SameLine();

				bool isRotateToolActive = gizmos.GetCurrentOperation() == ImGuizmo::ROTATE;
				if (ImGui::Selectable(ICON_LC_ROTATE_3D, isRotateToolActive, 0, toolbarItemSize))
					gizmos.SetCurrentOperation(isRotateToolActive ? 0 : ImGuizmo::ROTATE);

				ImGui::SameLine();

				bool isScaleToolActive = gizmos.GetCurrentOperation() == ImGuizmo::SCALE;
				if (ImGui::Selectable(ICON_LC_SCALE_3D, isScaleToolActive, 0, toolbarItemSize))
					gizmos.SetCurrentOperation(isScaleToolActive ? 0 : ImGuizmo::SCALE);
			}

			ImGui::EndTable();
		}

		ImGui::PopFont();
		ImGui::PopStyleVar(2);
	}

	ImGui::PopStyleVar(2);

	ImVec2 docSpaceSize = ImGui::GetContentRegionAvail();

	ImGuiID dockSpaceId = ImGui::GetID("Document Dockspace");
	// todo: ImGui::GetContentRegionAvail() returns wrong result first time it seems. What to do instead?
	ImGui::DockSpace(dockSpaceId, docSpaceSize, ImGuiDockNodeFlags_None, &myDocumentWindowClass);

	if (!myIsDockingInitialized)
	{
		ImGuiID center = 0, left = 0, rightBottom = 0, rightTop = 0;

		ImGui::DockBuilderRemoveNode(dockSpaceId); // clear any previous layout
		ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockSpaceId, docSpaceSize);

		center = dockSpaceId;

		ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.2f, &left, &center);
		ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, &rightBottom, &center);
		ImGui::DockBuilderSplitNode(rightBottom, ImGuiDir_Up, 0.2f, &rightTop, &rightBottom);

		ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::Viewport].c_str(), center);
		ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::ToolSettings].c_str(), rightTop);
		ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::Properties].c_str(), rightBottom);
		ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::Instances].c_str(), left);
		//ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::NavmeshCreationTool].c_str(), left);

		ImGui::DockBuilderFinish(dockSpaceId);

		myIsDockingInitialized = true;
	}

	ImGui::End();

	const Tga::Color color = Tga::Engine::GetInstance()->GetClearColor();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(color.myR, color.myG, color.myB, color.myA));
	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	bool isViewportOrInstancesFocused = false;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin(myPanelWindowNames[(size_t)Panels::Viewport].c_str());
	ImGui::PopStyleVar(1);

	isViewportOrInstancesFocused = isViewportOrInstancesFocused || ImGui::IsWindowFocused();
	myViewport.DrawAndUpdateViewportWindow(aTimeDelta, *this);

	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	ImGui::Begin(myPanelWindowNames[(size_t)Panels::ToolSettings].c_str());
	Gizmos& gizmos = myViewport.GetGizmos();
	gizmos.Draw();

	ImGui::Separator();
	ImGui::TextUnformatted("Asset Zoo");

	std::string sceneFilename = ToLowerAscii(std::filesystem::path(myScene->GetPath()).filename().string());
	const bool isAssetZooScene = (sceneFilename == "asset_zoo.tgs");

	if (!isAssetZooScene)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Populate From Objects (.tgo)"))
	{
		PopulateSceneFromObjectAssets(false);
	}

	if (ImGui::Button("Populate Animation Clip Zoo (.tgo)"))
	{
		PopulateSceneFromObjectAssets(true);
	}

	if (!isAssetZooScene)
	{
		ImGui::EndDisabled();
		ImGui::TextUnformatted("Open asset_zoo.tgs to enable population.");
	}
	ImGui::End();

	ImGui::SetNextWindowClass(&myDocumentWindowClass);
	ImGui::Begin(myPanelWindowNames[(size_t)Panels::Properties].c_str());
	myProperties.Draw();
	ImGui::End();

	ImGui::SetNextWindowClass(&myDocumentWindowClass);
	ImGui::Begin(myPanelWindowNames[(size_t)Panels::Instances].c_str());
	isViewportOrInstancesFocused = isViewportOrInstancesFocused || ImGui::IsWindowFocused();
	mySceneObjectList.Draw();
	ImGui::End();

#ifdef NOT_USED_FOR_HANDELING_P4_LOGIN
	if (authRequest) {
		char password[128]{};
		bool shouldSubmit = false;
		static bool focus_set = false;

		ImGui::OpenPopup("##p4password");

		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.0f, 0.5f, 1.0f)); // Purple color
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f); // Thicker border
		if (ImGui::BeginPopupModal("##p4password", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::Text("Enter perforce password for% s", P4::MyUser(), NULL, ImGuiWindowFlags_AlwaysAutoResize);
			if (!focus_set)
			{
				ImGui::SetKeyboardFocusHere();
				focus_set = false;
			}
			ImGui::InputText("##password_input", password, sizeof(password), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_AutoSelectAll);

			// Add spacing for better layout
			ImGui::Spacing();

			if (ImGui::IsKeyPressed(ImGuiKey_Enter))
			{
				shouldSubmit = true;
			}
			if (ImGui::Button("Submit"))
			{
				shouldSubmit = true;
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				focus_set = false;
				ImGui::CloseCurrentPopup();
			}

			if (shouldSubmit)
			{
				focus_set = false;
				ImGui::CloseCurrentPopup();

				/*
				todo: this keeps blocking when login fails
				authRequest = false;
				if (P4::Login(password))
				{
					std::filesystem::path p = myScene->GetName();
					P4::StartPollingLevelInfo(p.replace_extension(".leveldata").string().c_str(), 2, []() {
						authRequest = true;
					});
				}
				else
				{
					// show error!
				}*/
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}
#endif
	//ImGui::SetNextWindowClass(&myDocumentWindowClass);
	//ImGui::Begin(myPanelWindowNames[(size_t)Panels::NavmeshCreationTool].c_str());
	//myNavmeshCreationTool.DrawUI();
	//ImGui::End();

	ImGuiIO& io = ImGui::GetIO();
	if (isViewportOrInstancesFocused)
	{
		if (ImGui::IsAnyItemActive() == false && ImGui::IsKeyDown(ImGuiKey_Delete))
		{
			if (SceneSelection::GetActiveSceneSelection()->GetSelection().size() > 0)
			{
				std::shared_ptr<RemoveSceneObjectsCommand> command = std::make_shared<RemoveSceneObjectsCommand>();
				command->AddObjects(SceneSelection::GetActiveSceneSelection()->GetSelection());

				CommandManager::DoCommand(command);

				SceneSelection::GetActiveSceneSelection()->ClearSelection();
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_F) && !SceneSelection::GetActiveSceneSelection()->GetSelection().empty())
		{
			Vector3f pos{};
			size_t selectionSize = SceneSelection::GetActiveSceneSelection()->GetSelection().size();
			for (uint32_t id : SceneSelection::GetActiveSceneSelection()->GetSelection())
			{
				SceneObject* obj = myScene->GetSceneObject(id);
				pos += obj->GetPosition();
			}

			Tga::Camera& activeCamera = myViewport.GetCamera();
			pos.x /= selectionSize;
			pos.y /= selectionSize;
			pos.z /= selectionSize;
			activeCamera.GetTransform().SetPosition(pos);
			activeCamera.GetTransform().Translate(-myViewport.GetCameraFocusDistance() * activeCamera.GetTransform().GetForward());

		}

		if (io.KeyCtrl)
		{
			if (ImGui::IsKeyReleased(ImGuiKey_D))
			{
				std::span<const uint32_t> selection = SceneSelection::GetActiveSceneSelection()->GetSelection();

				if (selection.size() > 0)
				{
					std::shared_ptr<AddSceneObjectsCommand> command = std::make_shared<AddSceneObjectsCommand>();

					constexpr int nameBufferSize = 512;
					char nameBuffer[nameBufferSize];

					for (uint32_t id : selection)
					{
						std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>(*GetActiveScene()->GetSceneObject(id));

						const char* initialName = object->GetName();
						size_t initialLength = strlen(initialName);

						std::regex re("(.*)\\((\\d+)\\)$"); // Regex to match name and number in parentheses
						std::cmatch match;
						int number = 1;
						size_t baseLength = 0;

						// Use regex to parse the base name and number if parentheses with numbers are present
						if (std::regex_match(initialName, initialName + initialLength, match, re))
						{
							std::string base = match[1].str();
							baseLength = base.length();
							sprintf_s(nameBuffer, nameBufferSize, "%s", base.c_str());
							if (match[2].matched) 
							{
								number = std::stoi(match[2].str()) + 1;
							}
						}
						else 
						{
							sprintf_s(nameBuffer, nameBufferSize, "%s", initialName);
							baseLength = initialLength;
						}

						// Check if the base name already exists
						while (true)
						{
							bool exists = myScene->GetFirstSceneObject(nameBuffer) != nullptr;
							if (!exists)
							{
								for (auto pair : command->GetObjects())
								{
									if (pair.second->GetName() == std::string_view(nameBuffer))
									{
										exists = true;
										break;
									}
								}
							}

							if (!exists)
								break;

							sprintf_s(nameBuffer + baseLength, nameBufferSize - baseLength, "(%d)", number++);
						}

						object->SetName(nameBuffer);

						/*
						int i = 1;

						// todo: this is a O(n^2) algorithm
						while (true)
						{
							sprintf_s(buffer, "%s(%i)", object->GetName(), i);
							if (myScene->GetFirstSceneObject(buffer) == nullptr)
							{
								object->SetName(buffer);
								break;
							}
							i++;
						}
						*/

						command->AddObjects(std::span<std::shared_ptr<SceneObject>>(&object, 1));
					}

					CommandManager::DoCommand(command);

					SceneSelection::GetActiveSceneSelection()->ClearSelection();

					std::span<const std::pair<uint32_t, std::shared_ptr<SceneObject>>>  createdObjects = command->GetObjects();

					for (const std::pair<uint32_t, std::shared_ptr<SceneObject>>& p : createdObjects)
					{
						SceneSelection::GetActiveSceneSelection()->AddToSelection(p.first);
					}
				}
			}
		}
	}
	assert(GetActiveScene() == myScene);
	SetActiveScene(nullptr);
	assert(SceneSelection::GetActiveSceneSelection() == &mySceneSelection);
	SceneSelection::SetActiveSceneSelection(nullptr);
}

Scene* locPrevScene;

void SceneDocument::OnAction(CommandManager::Action action)
{
	static std::vector<uint32_t> objects;

	// keep track of which objects have been modified and if the scene has been modified
	// first time an object is modified, check it out in p4
	auto updateCountsAndP4 = [&](const AbstractCommand* command, int change)
		{
			const SceneCommandBase* commandBase = dynamic_cast<const SceneCommandBase*>(command);

			if (commandBase == nullptr)
			{
				if (mySceneModificationsCount == 0)
				{
					P4::CheckoutFile(myPath);
				}

				mySceneModificationsCount += change;
			}
			else
			{
				objects.clear();
				bool hasSceneChanged;
				commandBase->GetModifiedObjects(objects, hasSceneChanged);

				if (hasSceneChanged)
				{
					if (mySceneModificationsCount == 0)
					{
						P4::CheckoutFile(myPath);
					}

					mySceneModificationsCount += change;
				}

				for (uint32_t object : objects)
				{
					int& count = myObjectModificationsCounts[object];

					if (count == 0)
					{
						P4::CheckoutFile(myScene->GetObjectFilePath(object).GetString());
					}

					count += change;
				}
			}
		};

	if (action == CommandManager::Action::Do)
	{
		// If doing something when the undo stack is lower than when we saved, it means we can't get back to the saved state
		if (myUndoStackSize < mySaveUndoStackSize)
			mySaveUndoStackSize = -1;

		myUndoStackSize++;

		updateCountsAndP4(CommandManager::GetTopOfUndoStack(), 1);
	}
	if (action == CommandManager::Action::PostRedo)
	{
		myUndoStackSize++;

		updateCountsAndP4(CommandManager::GetTopOfUndoStack(), 1);
	}
	if (action == CommandManager::Action::PreUndo)
	{
		updateCountsAndP4(CommandManager::GetTopOfUndoStack(), -1);
	}

	if (action == CommandManager::Action::PostUndo)
	{
		myUndoStackSize--;
	}
	if (action == CommandManager::Action::Clear)
	{
		myUndoStackSize = 0;
	}

	if (action == CommandManager::Action::PreRedo || action == CommandManager::Action::PreUndo)
	{
		assert(locPrevScene == nullptr);

		locPrevScene = GetActiveScene();
		SetActiveScene(myScene);
	}

	mySceneSelection.OnAction(action);

	if (action == CommandManager::Action::PostRedo || action == CommandManager::Action::PostUndo)
	{
		assert(GetActiveScene() == myScene);

		SetActiveScene(locPrevScene);
		locPrevScene = nullptr;
	}

	mySceneObjectList.SetSceneDirty();
}

void SceneDocument::HandleDrop() 
{
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(".tgo"))
		{
			std::string data = (const char*)payload->Data;

			auto object = std::make_shared<SceneObject>();
			StringId objectDefinitionName = StringRegistry::RegisterOrGetString(fs::path(data).stem().string());
			object->SetSceneObjectDefintionName(objectDefinitionName);

			if (myScene->GetFirstSceneObject(objectDefinitionName.GetString()) == nullptr)
			{
				object->SetName(objectDefinitionName.GetString());
			}
			else
			{
				char buffer[512];

				int i = 1;

				// todo: this is a O(n^2) algorithm
				while (true)
				{
					sprintf_s(buffer, "%s(%i)", objectDefinitionName.GetString(), i);
					if (myScene->GetFirstSceneObject(buffer) == nullptr)
					{
						object->SetName(buffer);
						break;
					}
					i++;
				}
			}

			{
				Camera& cam = myViewport.GetCamera();
				Vector3f pos = cam.GetTransform().GetPosition() + cam.GetTransform().GetForward() * myViewport.GetCameraFocusDistance();

				if (myViewport.GetGizmos().GetSnappingInfo().snapPos)
				{
					pos = pos / myViewport.GetGizmos().GetSnappingInfo().pos;
					pos.x = round(pos.x);
					pos.y = round(pos.y);
					pos.z = round(pos.z);

					pos = myViewport.GetGizmos().GetSnappingInfo().pos * pos;
				}

				object->GetTRS().translation = pos;
			}

			std::shared_ptr<AddSceneObjectsCommand> command = std::make_shared<AddSceneObjectsCommand>();
			command->AddObjects(std::span<std::shared_ptr<SceneObject>>(&object, 1));
			CommandManager::DoCommand(command);

			SceneSelection::GetActiveSceneSelection()->ClearSelection();

			std::span<const std::pair<uint32_t, std::shared_ptr<SceneObject>>>  createdObjects = command->GetObjects();
			for (const std::pair<uint32_t, std::shared_ptr<SceneObject>>& p : createdObjects)
			{
				SceneSelection::GetActiveSceneSelection()->AddToSelection(p.first);
			}

			mySceneObjectList.SetSceneDirty();
		}

		ImGui::EndDragDropTarget();
	}
}
void SceneDocument::BeginDragSelection(Vector2f mousePos)
{
	Vector2i vpos = myViewport.GetViewportPos();
	Vector2i vsize = myViewport.GetViewportSize();

	RectSelection::GetCurrentRectSelection()->Update(
		{ mousePos.x - vpos.x, mousePos.y - vpos.y },
		{ (float)vpos.x, (float)vpos.y },
		{ (float)vsize.x, (float)vsize.y },
		myViewport.GetCamera()
	);

	SceneObjectDefinitionManager& manager = Editor::GetEditor()->GetSceneObjectDefinitionManager();
	std::vector<ScenePropertyDefinition> sceneObjectProperties;

	if (ImGui::GetIO().KeyShift == false)
	{
		RectSelection::GetCurrentRectSelection()->ClearSelection();
	}

	Matrix4x4f worldToCamera = Matrix4x4f::GetFastInverse(myViewport.GetCamera().GetTransform());
	for (auto& p : myScene->GetSceneObjects())
	{
		sceneObjectProperties.clear();
		p.second->CalculateCombinedPropertySet(manager, sceneObjectProperties);

		bool hasModel = false;

		for (ScenePropertyDefinition& property : sceneObjectProperties)
		{
			if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneModel>>())
			{
				const SceneModel& value = property.value.Get<CopyOnWriteWrapper<SceneModel>>()->Get();

				StringId path = value.path;
				if (path.IsEmpty() || Settings::ResolveAssetPath(path.GetString()).empty())
					continue;

				if (ModelFactory::GetInstance().GetModel(path.GetString()))
				{
					hasModel = true;

					std::shared_ptr<Model> model = ModelFactory::GetInstance().GetModel(path.GetString());
					const auto& bounds = model->GetMeshData(0).Bounds;
					Tga::Vector3f viewCenter = bounds.Center * p.second->GetTransform() * worldToCamera;

					// todo: Bounds here aren't correct since they aren't rotated.
					// Have to send in the transform as well into CheckFrustum for this to work
					// Easiest way to make this work then is to transform the frustrum planes with the inverse of the transform, 
					// so that the bounds still form a box/sphere
					const Tga::BoxSphereBounds viewBounds = {
						.Radius = bounds.Radius,
						.BoxExtents = bounds.BoxExtents,
						.Center = viewCenter
					};

					if (RectSelection::GetCurrentRectSelection()->CheckFrustum(viewBounds))
					{
						RectSelection::GetCurrentRectSelection()->AddToSelection(p.first);
					}
				}
			}
		}

		// If the object lacks a model, select based on its position only
		if (!hasModel)
		{
			Tga::Vector3f viewCenter = Vector3f(0.f) * p.second->GetTransform() * worldToCamera;
			const Tga::BoxSphereBounds viewBounds = {
				.Radius = 0,
				.BoxExtents = 0,
				.Center = viewCenter
			};

			if (RectSelection::GetCurrentRectSelection()->CheckFrustum(viewBounds))
			{
				RectSelection::GetCurrentRectSelection()->AddToSelection(p.first);
			}
		}
	}
}
void SceneDocument::EndDragSelection(Vector2f mousePos, bool isShiftDown)
{
	Vector2i vpos = myViewport.GetViewportPos();
	Vector2i vsize = myViewport.GetViewportSize();

	RectSelection::GetCurrentRectSelection()->Update(
		{ mousePos.x, mousePos.y },
		{ (float)vpos.x, (float)vpos.y },
		{ (float)vsize.x, (float)vsize.y },
		myViewport.GetCamera()
	);

	SceneObjectDefinitionManager& manager = Editor::GetEditor()->GetSceneObjectDefinitionManager();
	std::vector<ScenePropertyDefinition> sceneObjectProperties;

	Matrix4x4f worldToCamera = Matrix4x4f::GetFastInverse(myViewport.GetCamera().GetTransform());
	if (RectSelection::GetCurrentRectSelection()->IsActive() && isShiftDown == false)
	{
		SceneSelection::GetActiveSceneSelection()->ClearSelection();
	}

	for (auto& p : myScene->GetSceneObjects())
	{
		sceneObjectProperties.clear();
		p.second->CalculateCombinedPropertySet(manager, sceneObjectProperties);

		if (RectSelection::GetCurrentRectSelection()->Contains(p.first))
		{
			SceneSelection::GetActiveSceneSelection()->AddToSelection(p.first);
		}

	}
	RectSelection::GetCurrentRectSelection()->ClearSelection();
}

void SceneDocument::ClickSelection(Vector2f mousePos, uint32_t selectedId, bool isShiftDown)
{
	mousePos;

	if (isShiftDown == false)
	{
		SceneSelection::GetActiveSceneSelection()->ClearSelection();
	}

	if (selectedId > 0)
	{
		for (auto& p : myScene->GetSceneObjects())
		{
			if (selectedId == p.first) 
			{
				if (SceneSelection::GetActiveSceneSelection()->Contains(p.first))
				{
					SceneSelection::GetActiveSceneSelection()->RemoveFromSelection(p.first);
				}
				else
				{
					SceneSelection::GetActiveSceneSelection()->AddToSelection(p.first);
				}
			}
		}
	}
}

void SceneDocument::BeginTransformation() 
{
	myTransformationInitialTransforms.clear();

	const std::span<const uint32_t>& selection = SceneSelection::GetActiveSceneSelection()->GetSelection();

	for (const uint32_t& objectid : selection)
	{
		P4::CheckoutFile(myScene->GetObjectFilePath(objectid).GetString());

		SceneObject& object = *myScene->GetSceneObject(objectid);

		myTransformationInitialTransforms.push_back(object.GetTransform());
	}
	myPendingTransformCommand.Begin(selection);
}

void SceneDocument::UpdateTransformation(const Vector3f& referencePosition, const Matrix4x4f& transform)
{
	const std::span<const uint32_t>& selection = SceneSelection::GetActiveSceneSelection()->GetSelection();

	for (int i = 0; i < selection.size(); i++)
	{
		uint32_t objectid = selection[i];

		SceneObject& object = *myScene->GetSceneObject(objectid);

		Matrix4x4f oldTransform = myTransformationInitialTransforms[i];
		oldTransform.SetPosition(oldTransform.GetPosition() - referencePosition);
		Matrix4x4f t = oldTransform * transform;
		t.SetPosition(t.GetPosition() + referencePosition);

		object.SetTransform(t);
	}
}

void SceneDocument::EndTransformation() 
{
	myTransformationInitialTransforms.clear();

	myPendingTransformCommand.End();
	myPendingTransformCommand = {};
}

Vector3f SceneDocument::CalculateSelectionPosition()
{
	const std::span<const uint32_t>& selection = SceneSelection::GetActiveSceneSelection()->GetSelection();

	return GetActiveScene()->GetSceneObject(selection.back())->GetPosition();
}

Matrix4x4f SceneDocument::CalculateSelectionOrientation()
{
	const std::span<const uint32_t>& selection = SceneSelection::GetActiveSceneSelection()->GetSelection();

	return GetActiveScene()->GetSceneObject(selection.back())->GetTransform();
}

bool SceneDocument::HasTransformableSelection()
{
	return !SceneSelection::GetActiveSceneSelection()->GetSelection().empty();
}
