#pragma once

#include <imgui.h>
#include <Document/Document.h>
#include <Scene/SceneSelection.h>
#include <Tools/Viewport/Viewport.h>
#include <tge/scene/SceneObjectDefinition.h>
#include "LivePreviewData.h"

namespace Tga
{

class ObjectDefinitionDocument : public Document, public ViewportInterface
{
public:
	enum class Panels
	{
		ObjectDefinition,
		Properties,
		Viewport,
		Script,
		VisualPreviewSettings,
		LivePreview,
		Count
	};

	void Init(std::string_view path) override;
	void Update(float aTimeDelta, InputManager& inputManager) override;
	void Save() override;
	void OnAction(CommandManager::Action action) override;

	void HandleDrop() override;
	void BeginDragSelection(Vector2f mousePos) override;
	void EndDragSelection(Vector2f mousePos, bool isShiftDown) override;
	void ClickSelection(Vector2f mousePos, uint32_t selectedId, bool isShiftDown) override;

	void BeginTransformation() override;
	void UpdateTransformation(const Vector3f& referencePosition, const Matrix4x4f& transform) override;
	void EndTransformation() override;

	Vector3f CalculateSelectionPosition() override;
	Matrix4x4f CalculateSelectionOrientation() override;

	void SetActiveScript(const std::string_view& aScriptPath);

	virtual bool HasTransformableSelection() override;

private:

	void DrawObjectDefinitionPanel();
	void DrawPropertyPanel();
	void DrawPreviewSettings();
	void DrawAndUpdateLivePreview(float aTimeDelta);

	SceneObjectDefinition* myObjectDefinition;

	StringId mySelectedProperty;
	std::string mySelectedScript;
	std::string myActiveScript;
	std::string myPrevActiveScript;

	EditorViewport myViewport;
	SceneCache myCache;

	LivePreviewData myLivePreviewData;

	bool myIsDockingInitialized = false;

	std::string myPanelWindowNames[(size_t)Panels::Count];
};

}