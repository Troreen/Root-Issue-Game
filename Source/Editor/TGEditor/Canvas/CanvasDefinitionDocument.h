#pragma once

#include <imgui.h>

#include <Document/Document.h>
#include <Scene/SceneSelection.h>
#include <Tools/Viewport/Viewport.h>
#include "tge/ui/CanvasObjectDefinition.h"
#include "LivePreviewData.h"

namespace Tga
{
	class CanvasDefinitionDocument : public Document, public ViewportInterface
	{
	public:
		enum class Panels
		{
			UIElements,
			Properties,
			Viewport,
			Script,
			CanvasSettings,
			LivePreview,
			Count
		};

		void Init(std::string_view path) override;
		void Update(float aTimeDelta, InputManager& inputManager) override;
		void Save() override;
		void OnAction(CommandManager::Action action) override;

		void DrawGeneralProperties(GeneralUIProperties& props, const char* label);
		void DrawSelectableProperties(SelectableUIProperties& selectableProps, GeneralUIProperties& props);
		void DrawUIElementGeneralProperties(UIElement& element);
		void DrawUIElementProperties(UIElement& element);

		void DrawImageProperties(UIImage& img, const char* label);
		void DrawTextProperties(UIText& text, const char* label);

#pragma region Must Have Overrides
		void HandleDrop() override;
		void BeginDragSelection(Vector2f mousePos) override;
		void EndDragSelection(Vector2f mousePos, bool isShiftDown) override;
		void ClickSelection(Vector2f mousePos, uint32_t selectedId, bool isShiftDown) override;

		void BeginTransformation() override;
		void UpdateTransformation(const Vector3f& referencePosition, const Matrix4x4f& transform) override;
		void EndTransformation() override;

		Vector3f CalculateSelectionPosition() override;
		Matrix4x4f CalculateSelectionOrientation() override;

		virtual bool HasTransformableSelection() override;
#pragma endregion

	private:
		CanvasObjectDefinition* myObjectDefinition;

		StringId mySelectedProperty;
		std::string mySelectedScript;
		std::string myActiveScript;
		std::string myPrevActiveScript;

		EditorViewport myViewport;
		Tga::Vector2i newViewportSize;
		SceneCache myCache;

		LivePreviewData myLivePreviewData;

		bool myIsDockingInitialized = false;

		std::string myPanelWindowNames[(size_t)Panels::Count];

		int mySelectedUIElement = -1;
	};

}
