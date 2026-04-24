#pragma once
#include <tge/Graphics/RenderTarget.h>
#include <tge/Graphics/DepthBuffer.h>
#include <tge/shaders/ModelShader.h>
#include <tge/shaders/SpriteShader.h>
#include <tge/scene/Scene.h>

#include <Tools/Navmesh/NavmeshCreationTool.h>
#include <Tools/SceneObjectProperties/SceneObjectProperties.h>
#include <Tools/AssetBrowser/AssetBrowser.h>
#include <Tools/SceneObjectList/SceneObjectList.h>
#include <Tools/Viewport/Viewport.h>
#include <Tools/ViewportGrid/ViewportGrid.h>

#include <imgui.h>

#include <Document/Document.h>
#include <Scene/SceneSelection.h>
#include <tge/scene/SceneSerialize.h>
#include <Scene/SceneUtil.h>

namespace Tga
{

	void SceneP4Handler(SceneFileChangeType aChangeType, const char* aPath);

	class InputManager;

class SceneDocument : public Document, public ViewportInterface
{
public:
	enum class Panels
	{
		Viewport,
		Instances,
		Properties,
		ToolSettings,
		NavmeshCreationTool,
		Count
	};

	void Close() override;
	void Init(std::string_view path) override;
	void Update(float aTimeDelta, InputManager& inputManager) override;
	void Save() override;
	void OnAction(CommandManager::Action action) override;

	void HandleDrop() override;
	void BeginDragSelection(Vector2f mousePos) override;
	void EndDragSelection(Vector2f mousePos, bool isShiftDown) override;
	void ClickSelection(Vector2f mousePos, uint32_t selectedId, bool isShiftDown) override;

	void BeginTransformation() override;
	void UpdateTransformation(const Vector3f& referencePosition, const Matrix4x4f& transformRelativeStart) override;
	void EndTransformation() override;

	Vector3f CalculateSelectionPosition() override;
	Matrix4x4f CalculateSelectionOrientation() override;

	virtual bool HasTransformableSelection() override;

private:
	void PopulateSceneFromObjectAssets(bool aAnimationClipZoo = false);

	EditorViewport myViewport;
	SceneObjectProperties myProperties;
	SceneObjectList mySceneObjectList;
	NavmeshCreationTool myNavmeshCreationTool;

	Scene* myScene;
	SceneSelection mySceneSelection;

	bool myIsDockingInitialized = false;

	std::string myPanelWindowNames[(size_t)Panels::Count];

	std::unordered_map<uint32_t, int> myObjectModificationsCounts;
	int mySceneModificationsCount = 0;

	TransformCommand myPendingTransformCommand;
	std::vector<Matrix4x4f> myTransformationInitialTransforms;

	SceneCache myCache;
};

}