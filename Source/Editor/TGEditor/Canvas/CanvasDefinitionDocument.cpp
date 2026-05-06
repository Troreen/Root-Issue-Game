#include "stdafx.h"
#include "CanvasDefinitionDocument.h"

#include "imgui_widgets/imgui_widgets.h"
#include "imgui_internal.h" // for DockBuilder Api

#include <tge/imgui/ImGuiPropertyEditor.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <tge/texture/TextureManager.h>
#include <tge/script/ScriptManager.h>
#include <tge/script/ScriptRuntimeInstance.h>
#include <tge/script/contexts/ScriptUpdateContext.h>
#include <tge/script/BaseProperties.h>

#include <IconFontHeaders/IconsLucide.h>

#include <ObjectDefinition/Commands/ChangePropertiesCommand.h>
#include <ScriptEditor/ScriptEditor.h>
#include <ScriptEditor/Commands/CreateScriptCommand.h>

#include <Editor.h>
#include <p4/p4.h>

#include "tge/drawers/DebugDrawer.h"
#include "tge/text/text.h"

#include <algorithm>

constexpr int MAX_OBJECTDEFINTION_TEXT_LENGTH = 256;

using namespace Tga;

struct ObjectEditorPreviewSettings
{
	StringId previewPixelShaderPath;
	ModelShader previewModelShader;
	SpriteShader previewSpriteShader;

	StringId cubeMapPath;
	AmbientLight ambientLight;

	float directionalLightYaw = 45.f;
	float directionalLightPitch = -45.f;
	Color ambientColor = { 0.1f, 0.5f, 0.8f };
	float ambientColorMultiplier = 1.0f;
	Color directionalLightColor = { 0.9f, 0.7f, 0.5f };
	float directionalLightColorMultiplier = 1.4f;
};

static ObjectEditorPreviewSettings locPreviewSettings = {};

static void UpdatePreviewShaders()
{
	locPreviewSettings.previewModelShader = {};
	locPreviewSettings.previewModelShader.Init("Shaders/PbrModelShaderVS", locPreviewSettings.previewPixelShaderPath.GetString());

	locPreviewSettings.previewSpriteShader = {};
	locPreviewSettings.previewSpriteShader.Init("Shaders/instanced_sprite_shader_VS", locPreviewSettings.previewPixelShaderPath.GetString());
}

void CanvasDefinitionDocument::Init(std::string_view aPath)
{
	Document::Init(aPath);

	std::filesystem::path path = aPath;
	std::string filename = path.stem().string();

	StringId nameId = StringRegistry::RegisterOrGetString(filename.data());
	myObjectDefinition = Editor::GetEditor()->GetCanvasObjectDefinitionManager().Get(nameId);

	myViewport = EditorViewport(true);
	myViewport.Init();
	myViewport.Resize(myObjectDefinition->GetReferenceWindowResolution());
	myViewport.GetGrid().SetGridLineExtreme(400.0f);

	if (locPreviewSettings.previewPixelShaderPath.IsEmpty())
	{
		locPreviewSettings.previewPixelShaderPath = "shaders/model_shader_PS"_tgaid;
		UpdatePreviewShaders();
	}

	char buffer[512];
	char asterix[2] = { 0, 0 };

	sprintf_s(buffer, "%s%s###Document:%s", myObjectDefinition->GetName().GetString(), asterix, myObjectDefinition->GetPath());
	myImGuiName = StringRegistry::RegisterOrGetString(buffer);

	sprintf_s(buffer, "UI Elements##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::UIElements] = buffer;
	sprintf_s(buffer, "UI Properties##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::Properties] = buffer;
	sprintf_s(buffer, "Viewport##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::Viewport] = buffer;
	sprintf_s(buffer, "Script##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::Script] = buffer;
	sprintf_s(buffer, "Canvas Settings##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::CanvasSettings] = buffer;
	sprintf_s(buffer, "Live Preview##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::LivePreview] = buffer;

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

	camera.GetTransform().SetPosition((camera.GetTransform().GetForward() * -myViewport.GetCameraFocusDistance()));
}

void CanvasDefinitionDocument::Save()
{
	myObjectDefinition->Save();

	mySaveUndoStackSize = myUndoStackSize;
}

void CanvasDefinitionDocument::Update(float aTimeDelta, InputManager& inputManager)
{
	aTimeDelta; inputManager;

	// clearing out caches every frame to support updates to assets while the editor is running
	myCache.ClearCache();

	auto& engine = *Tga::Engine::GetInstance();
	DX11::BackBuffer->SetAsActiveTarget(DX11::DepthBuffer);
	auto& graphicsStateStack = engine.GetGraphicsEngine().GetGraphicsStateStack();
	graphicsStateStack.Push();

	const Camera& renderCamera = myViewport.GetCamera();
	Frustum frustum = CalculateFrustum(renderCamera);

	std::vector<int> renderList;
	auto& elements = myObjectDefinition->GetUIElements();

	renderList.reserve(elements.size());
	for (int i = 0; i < elements.size(); ++i)
		renderList.push_back(i);

	std::stable_sort(renderList.begin(), renderList.end(),
		[&](int a, int b)
		{
			int renderOrderA = elements[a].generalProperties.renderOrder;
			if (elements[a].generalProperties.groupIndex != -1)
			{
				renderOrderA += elements[elements[a].generalProperties.groupIndex].generalProperties.renderOrder;
			}

			int renderOrderB = elements[b].generalProperties.renderOrder;
			if (elements[b].generalProperties.groupIndex != -1)
			{
				renderOrderB += elements[elements[b].generalProperties.groupIndex].generalProperties.renderOrder;
			}

			return renderOrderA > renderOrderB;
		});

	myObjectDefinition->SetRenderOrder(renderList);

	{
		myViewport.BeginDraw();

		{
			myViewport.SetupIdPass();

			CanvasDrawParameters drawParameters = {
				.useIdShader = true,
				.showBounds = myObjectDefinition->GetShowBounds(),
				.resolution = myViewport.GetViewportSize(),
			};

			for (int idx : renderList)
			{
				UIElement& prop = elements[idx];

				myViewport.SetObjectAndSelectionId(1 + idx, prop.generalProperties.name == mySelectedProperty.GetString() ? 1 + idx : 0, P4::FileInfo());

				CanvasObjectDefinition::DrawCanvasElement(*myObjectDefinition, prop, drawParameters);
			}
		}

		{
			CanvasDrawParameters drawParameters = {
				.useIdShader = false,
				.showBounds = myObjectDefinition->GetShowBounds(),
				.resolution = myViewport.GetViewportSize(),
			};


			myViewport.SetupColorPass();

			locPreviewSettings.ambientLight.color = locPreviewSettings.ambientColorMultiplier * locPreviewSettings.ambientColor;

			for (int idx : renderList)
			{
				UIElement& prop = elements[idx];

				myViewport.SetObjectAndSelectionId(1 + idx, prop.generalProperties.name == mySelectedProperty.GetString() ? 1 + idx : 0, P4::FileInfo());

				CanvasObjectDefinition::DrawCanvasElement(*myObjectDefinition, prop, drawParameters);
			}
		}

		graphicsStateStack.SetSamplerState(SamplerFilter::Bilinear, SamplerAddressMode::Wrap);
		graphicsStateStack.SetBlendState(BlendState::AlphaBlend);
		graphicsStateStack.SetDepthStencilState(DepthStencilState::ReadOnlyLess);

		CanvasObjectDefinition::DrawQueued();
		Tga::Engine::GetInstance()->GetDebugDrawer().DrawPendingDebugLines();
		myViewport.EndDraw();

		graphicsStateStack.Pop();
	}

	char buffer[512];
	char asterix[2] = { 0, 0 };

	if (mySaveUndoStackSize != myUndoStackSize)
		asterix[0] = '*';

	sprintf_s(buffer, "%s%s###Document:%s", myObjectDefinition->GetName().GetString(), asterix, myObjectDefinition->GetPath());

	if (!myIsDockingInitialized)
	{
		ImGui::DockBuilderSetNodeSize(Editor::GetEditor()->GetDocumentDockSpaceId(), Editor::GetEditor()->GetDocumentDockSpaceSize());
		ImGui::DockBuilderDockWindow(buffer, Editor::GetEditor()->GetDocumentDockSpaceId());

		ImGui::DockBuilderFinish(Editor::GetEditor()->GetDocumentDockSpaceId());
	}

	ImGui::SetNextWindowClass(Editor::GetEditor()->GetDocumentWindowClass());
	ImGui::SetNextWindowDockID(Editor::GetEditor()->GetDocumentDockSpaceId(), ImGuiCond_Once);

	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);

		bool open = true;
		ImGui::Begin(buffer, &open);
		if (myState == Document::State::Open && !open)
		{
			myState = Document::State::CloseRequested;
		}
		ImGui::PopStyleVar(2);

		ImVec2 docSpaceSize = ImGui::GetContentRegionAvail();
		ImGuiID dockSpaceId = ImGui::GetID("Document Dockspace");
		// todo: ImGui::GetContentRegionAvail() returns wrong result first time it seems. What to do instead?
		ImGui::DockSpace(dockSpaceId, docSpaceSize, ImGuiDockNodeFlags_None, &myDocumentWindowClass);

		if (!myIsDockingInitialized)
		{
			ImGuiID center = 0, left = 0, right = 0;

			ImGui::DockBuilderRemoveNode(dockSpaceId); // clear any previous layout
			ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockSpaceId, docSpaceSize);

			center = dockSpaceId;

			ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.2f, &left, &center);
			ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, &right, &center);

			ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::Properties].c_str(), right);
			ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::CanvasSettings].c_str(), left);

			if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
			{
				ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::LivePreview].c_str(), left);
			}

			ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::UIElements].c_str(), left);

			if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
			{
				ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::Script].c_str(), center);
			}

			ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::Viewport].c_str(), center);

			ImGui::DockBuilderFinish(dockSpaceId);

			myIsDockingInitialized = true;
		}

		ImGui::End();
	}

	const Tga::Color color = Tga::Engine::GetInstance()->GetClearColor();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(color.myR, color.myG, color.myB, color.myA));
	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	bool isViewportOrPropertiesFocused = false;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	if (newViewportSize != Vector2i{ 0,0 })
	{
		ImGuiWindow* window = ImGui::FindWindowByName(myPanelWindowNames[(size_t)Panels::Viewport].c_str());
		ImGui::SetNextWindowSize(ImVec2(static_cast<float>(newViewportSize.x), static_cast<float>(newViewportSize.y + window->TitleBarHeight)), ImGuiCond_Always);
		newViewportSize = { 0,0 };
	}

	ImGui::Begin(myPanelWindowNames[(size_t)Panels::Viewport].c_str());
	ImGui::PopStyleVar(1);

	isViewportOrPropertiesFocused = isViewportOrPropertiesFocused || ImGui::IsWindowFocused();
	myViewport.DrawAndUpdateViewportWindow(aTimeDelta, *this);

	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	ImGui::Begin(myPanelWindowNames[(size_t)Panels::UIElements].c_str());

	if (ImGui::Button("Add"))
	{
		ImGui::OpenPopup("uitype_popup");
	}

	ImGui::SameLine();

	int selectedUIType = -1;

	if (ImGui::BeginPopup("uitype_popup"))
	{
		ImGui::SeparatorText("UI Type");
		for (int i = 0; i < static_cast<int>(UIElementType::Count); i++)
			if (ImGui::Selectable(ToString(static_cast<UIElementType>(i))))
				selectedUIType = i;
		ImGui::EndPopup();
	}

	if (selectedUIType != -1)
	{
		UIElement newElement;

		switch (static_cast<UIElementType>(selectedUIType))
		{
		case UIElementType::Image:
			newElement.uiElementProperties = UIImage{};
			break;

		case UIElementType::Text:
			newElement.uiElementProperties = UIText{};
			break;

		case UIElementType::Button:
			newElement.uiElementProperties = UIButton{};
			break;

		case UIElementType::Toggle:
			newElement.uiElementProperties = UIToggle{};
			break;

		case UIElementType::Slider:
			newElement.uiElementProperties = UISlider{};
			break;

		case UIElementType::ElementGroup:
			newElement.uiElementProperties = UIElementGroup{};
			break;
		}

		myObjectDefinition->AddUIElement(newElement);
	}

	ImGui::SameLine();

	if (ImGui::Button("Delete"))
	{
		if (mySelectedUIElement != -1)
		{
			myObjectDefinition->RemoveUIElement(mySelectedUIElement);
			mySelectedUIElement--;
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Duplicate"))
	{
		if (mySelectedUIElement != -1)
		{
			std::vector<UIElement>& uiElements = myObjectDefinition->GetUIElements();
			UIElement newElement = uiElements[mySelectedUIElement];
			myObjectDefinition->AddUIElement(newElement);
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Save"))
	{
		myObjectDefinition->Save();
	}

	std::vector<UIElement>& uiElements = myObjectDefinition->GetUIElements();

	std::vector<int> sortedIndices(uiElements.size());
	for (int i = 0; i < uiElements.size(); ++i)
		sortedIndices[i] = i;

	std::sort(sortedIndices.begin(), sortedIndices.end(),
		[&](int a, int b)
		{
			return uiElements[a].generalProperties.hiercharyDisplayOrder <
				uiElements[b].generalProperties.hiercharyDisplayOrder;
		});

	std::unordered_map<int, std::vector<int>> groupChildren;
	std::vector<int> rootElements;

	for (int idx : sortedIndices)
	{
		if (uiElements[idx].generalProperties.groupIndex == -1)
		{
			rootElements.push_back(idx);
		}
		else
		{
			groupChildren[uiElements[idx].generalProperties.groupIndex].push_back(idx);
		}
	}

	ImGui::SameLine();

	if (ImGui::Button(ICON_LC_ARROW_DOWN))
	{
		int selected = mySelectedUIElement;
		if (selected == -1)
			return;

		int groupIndex = uiElements[selected].generalProperties.groupIndex;

		std::vector<int> siblings;

		for (int i = 0; i < uiElements.size(); ++i)
		{
			if (uiElements[i].generalProperties.groupIndex == groupIndex)
				siblings.push_back(i);
		}

		std::sort(siblings.begin(), siblings.end(),
			[&](int a, int b)
			{
				return uiElements[a].generalProperties.hiercharyDisplayOrder <
					uiElements[b].generalProperties.hiercharyDisplayOrder;
			});

		auto it = std::find(siblings.begin(), siblings.end(), selected);

		if (it != siblings.end() && (it + 1) != siblings.end())
		{
			int nextIndex = *(it + 1);

			std::swap(
				uiElements[selected].generalProperties.hiercharyDisplayOrder,
				uiElements[nextIndex].generalProperties.hiercharyDisplayOrder
			);
		}
	}

	ImGui::SameLine();

	ImGui::SameLine();

	if (ImGui::Button(ICON_LC_ARROW_UP))
	{
		int selected = mySelectedUIElement;
		if (selected == -1)
			return;

		int groupIndex = uiElements[selected].generalProperties.groupIndex;

		std::vector<int> siblings;

		for (int i = 0; i < uiElements.size(); ++i)
		{
			if (uiElements[i].generalProperties.groupIndex == groupIndex)
				siblings.push_back(i);
		}

		std::sort(siblings.begin(), siblings.end(),
			[&](int a, int b)
			{
				return uiElements[a].generalProperties.hiercharyDisplayOrder <
					uiElements[b].generalProperties.hiercharyDisplayOrder;
			});

		auto it = std::find(siblings.begin(), siblings.end(), selected);

		if (it != siblings.end() && it != siblings.begin())
		{
			int prevIndex = *(it - 1);

			std::swap(
				uiElements[selected].generalProperties.hiercharyDisplayOrder,
				uiElements[prevIndex].generalProperties.hiercharyDisplayOrder
			);
		}
	}

	for (int index : rootElements)
	{
		UIElement& element = uiElements[index];

		if (element.elementType == UIElementType::ElementGroup)
		{

			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_SpanFullWidth;

			if (mySelectedUIElement == index)
				flags |= ImGuiTreeNodeFlags_Selected;

			bool open = ImGui::TreeNodeEx(
				(void*)(intptr_t)index,
				flags,
				"%s",
				element.generalProperties.name
			);

			if (ImGui::IsItemClicked())
				mySelectedUIElement = index;

			if (ImGui::IsItemClicked())
				mySelectedUIElement = index;

			if (open)
			{
				for (int childIndex : groupChildren[index])
				{
					UIElement& child = uiElements[childIndex];

					ImGuiTreeNodeFlags childFlags =
						ImGuiTreeNodeFlags_Leaf |
						ImGuiTreeNodeFlags_NoTreePushOnOpen |
						ImGuiTreeNodeFlags_SpanFullWidth;

					if (mySelectedUIElement == childIndex)
						childFlags |= ImGuiTreeNodeFlags_Selected;

					ImGui::TreeNodeEx(
						(void*)(intptr_t)childIndex,
						childFlags,
						"%s",
						child.generalProperties.name
					);

					if (ImGui::IsItemClicked())
						mySelectedUIElement = childIndex;
				}

				ImGui::TreePop();
			}
		}
		else
		{
			if (ImGui::Selectable(
				element.generalProperties.name,
				mySelectedUIElement == index))
			{
				mySelectedUIElement = index;
			}
		}
	}

	ImGui::End();

	if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
	{
		ImGui::SetNextWindowClass(&myDocumentWindowClass);

		if (myActiveScript != myPrevActiveScript)
		{
			ImGui::SetNextWindowFocus();
			myPrevActiveScript = myActiveScript;
		}

		ImGui::Begin(myPanelWindowNames[(size_t)Panels::Script].c_str());

		if (!myActiveScript.empty())
		{
			EditorScriptManager::GetInstance().DisplayEditor(myActiveScript);
		}

		ImGui::End();
	}

	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	isViewportOrPropertiesFocused = isViewportOrPropertiesFocused || ImGui::IsWindowFocused();
	ImGui::Begin(myPanelWindowNames[(size_t)Panels::Properties].c_str());

	if (mySelectedUIElement != -1)
	{
		UIElement& element = uiElements[mySelectedUIElement];

		DrawUIElementGeneralProperties(element);
		if (element.elementType != UIElementType::ElementGroup)
		{
			DrawUIElementProperties(element);
		}
	}

	ImGui::End();

	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	ImGui::Begin(myPanelWindowNames[(size_t)Panels::CanvasSettings].c_str());

	ImGui::Text("Reference Window Size:");
	Tga::Vector2i& referenceWindowSizeVector = myObjectDefinition->GetReferenceWindowResolution();
	int referenceWindowSize[2] = { referenceWindowSizeVector.x, referenceWindowSizeVector.y };
	ImGui::InputInt2("##ReferenceWindowSize", referenceWindowSize);
	myObjectDefinition->SetReferenceWindowResolution({ referenceWindowSize[0], referenceWindowSize[1] });

	Tga::Vector2i referenceWindowSizeVectorCopy = referenceWindowSizeVector;
	while (referenceWindowSizeVectorCopy.y != 0)
	{
		int r = referenceWindowSizeVectorCopy.x % referenceWindowSizeVectorCopy.y;
		referenceWindowSizeVectorCopy.x = referenceWindowSizeVectorCopy.y;
		referenceWindowSizeVectorCopy.y = r;
	}
	int gcd = referenceWindowSizeVectorCopy.x;

	std::string aspectRatio = std::to_string(referenceWindowSizeVector.x / gcd) + ":" + std::to_string(referenceWindowSizeVector.y / gcd);

	ImGui::NewLine();
	ImGui::Text("Current Aspect Ratio:");
	ImGui::Text(aspectRatio.c_str());

	ImGui::NewLine();
	if (ImGui::Button("Snap Viewport To Aspect Ratio"))
	{
		newViewportSize = myViewport.GetViewportSize();
		newViewportSize.x = static_cast<int>(static_cast<float>(newViewportSize.y) * (static_cast<float>(referenceWindowSizeVector.x) / static_cast<float>(referenceWindowSizeVector.y)));
		myViewport.Resize(newViewportSize);
	}
	ImGui::Text("This only works if the viewport is undocked.");

	ImGui::Text("Gizmos");
	bool& showBounds = myObjectDefinition->GetShowBounds();
	ImGui::Checkbox("##Gizmos", &showBounds);
	myObjectDefinition->SetShowBounds(showBounds);

	ImGui::End();

	if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
	{
		ImGui::Begin(myPanelWindowNames[(size_t)Panels::LivePreview].c_str());

		ImGui::End();
	}

}

void CanvasDefinitionDocument::OnAction(CommandManager::Action action)
{
	if (action == CommandManager::Action::Do)
	{
		if (myUndoStackSize == 0)
		{
			P4::CheckoutFile(myObjectDefinition->GetPath());
		}

		// If doing something when the undo stack is lower than when we saved, it means we can't get back to the saved state
		if (myUndoStackSize < mySaveUndoStackSize)
			mySaveUndoStackSize = -1;

		myUndoStackSize++;
	}
	if (action == CommandManager::Action::PostRedo)
	{
		myUndoStackSize++;
	}
	if (action == CommandManager::Action::PostUndo)
	{
		myUndoStackSize--;
	}
	if (action == CommandManager::Action::Clear)
	{
		myUndoStackSize = 0;
	}
}

void Tga::CanvasDefinitionDocument::DrawUIElementGeneralProperties(UIElement& element)
{
	DrawGeneralProperties(element.generalProperties, "General");
}

void Tga::CanvasDefinitionDocument::DrawGeneralProperties(GeneralUIProperties& props, const char* label)
{
	if (!ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen))
		return;

	bool hide = props.hide;
	if (ImGui::Checkbox("Hide", &hide))
	{
		props.hide = hide;
	}

	char nameBuffer[128];
	std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", props.name);
	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
	{
		if (strcmp(nameBuffer, props.name) != 0)
		{
			snprintf(props.name, 128, nameBuffer);
			myObjectDefinition->RenameUIElement(mySelectedUIElement);
		}
	}

	std::vector<UIElement>& uiElements = myObjectDefinition->GetUIElements();
	if (uiElements[mySelectedUIElement].elementType != UIElementType::ElementGroup)
	{
		int& selectedIndex = props.groupIndex;

		const char* previewName = "None";

		if (selectedIndex >= 0 &&
			selectedIndex < static_cast<int>(uiElements.size()))
		{
			previewName = uiElements[selectedIndex].generalProperties.name;
		}

		if (ImGui::BeginCombo("Element Group", previewName))
		{
			bool noneSelected = (selectedIndex == -1);
			if (ImGui::Selectable("None", noneSelected))
			{
				selectedIndex = -1;
			}
			if (noneSelected)
				ImGui::SetItemDefaultFocus();

			ImGui::Separator();

			for (int i = 0; i < uiElements.size(); ++i)
			{
				if (uiElements[i].elementType != UIElementType::ElementGroup || strcmp(uiElements[i].generalProperties.name, props.name) == 0)
					continue;

				bool isSelected = (selectedIndex == i);

				if (ImGui::Selectable(uiElements[i].generalProperties.name, isSelected))
				{
					selectedIndex = i;
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		float pos[2] = { props.pos.x, props.pos.y };
		if (ImGui::DragFloat2("Position", pos, 1.0f))
		{
			props.pos = { pos[0], pos[1] };
		}

		float anchor[2] = { props.anchorPoint.x, props.anchorPoint.y };
		if (ImGui::DragFloat2("Anchor", anchor, 0.01f, -1.f, 1.f))
		{
			props.anchorPoint = { anchor[0], anchor[1] };
		}

		float size[2] = { props.size.x, props.size.y };
		if (ImGui::DragFloat2("Size", size, 1.0f, 0.0f))
		{
			props.size = { size[0], size[1] };
		}


		bool scaleUniformly = props.scaleUniformly;
		if (ImGui::Checkbox("Scale Uniformly", &scaleUniformly))
		{
			props.scaleUniformly = scaleUniformly;
		}

		float pivot[2] = { props.pivot.x, props.pivot.y };
		if (ImGui::DragFloat2("Pivot", pivot, 0.01f, -1.f, 1.f))
		{
			props.pivot = { pivot[0], pivot[1] };
		}
	}

	int renderOrder = -props.renderOrder;
	if (ImGui::InputInt("Render Order", &renderOrder))
	{
		props.renderOrder = -renderOrder;
	}
}

void Tga::CanvasDefinitionDocument::DrawSelectableProperties(SelectableUIProperties& selectableProps, GeneralUIProperties& props)
{
	if (!ImGui::CollapsingHeader("Navigation", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	std::vector<UIElement>& elements = myObjectDefinition->GetUIElements();

	auto DrawNavigationCombo = [&](const char* comboLabel, int directionIndex)
		{
			int& selectedIndex = selectableProps.navigation[directionIndex];

			const char* previewName = "None";

			if (selectedIndex >= 0 &&
				selectedIndex < static_cast<int>(elements.size()))
			{
				previewName = elements[selectedIndex].generalProperties.name;
			}

			if (ImGui::BeginCombo(comboLabel, previewName))
			{
				bool noneSelected = (selectedIndex == -1);
				if (ImGui::Selectable("None", noneSelected))
				{
					selectedIndex = -1;
				}
				if (noneSelected)
					ImGui::SetItemDefaultFocus();

				ImGui::Separator();

				for (int i = 0; i < elements.size(); ++i)
				{
					if (strcmp(elements[i].generalProperties.name, props.name) == 0 || elements[i].elementType == UIElementType::Text
						|| elements[i].elementType == UIElementType::Image || elements[i].elementType == UIElementType::ElementGroup)
						continue;

					bool isSelected = (selectedIndex == i);

					if (ImGui::Selectable(elements[i].generalProperties.name, isSelected))
					{
						selectedIndex = i;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
		};

	// 0 = Up, 1 = Down, 2 = Left, 3 = Right
	DrawNavigationCombo("Up", 0);
	DrawNavigationCombo("Down", 1);
	DrawNavigationCombo("Left", 2);
	DrawNavigationCombo("Right", 3);

	bool customMouseSelectionBounds = selectableProps.customMouseSelectionBounds;
	if (ImGui::Checkbox("Custom Mouse Selection Bounds", &customMouseSelectionBounds))
	{
		selectableProps.customMouseSelectionBounds = customMouseSelectionBounds;
	}

	if (selectableProps.customMouseSelectionBounds)
	{
		float mouseSelectionBounds[2] = { selectableProps.mouseSelectionBounds.x, selectableProps.mouseSelectionBounds.y };
		if (ImGui::DragFloat2("Custom Mouse Selection Bounds", mouseSelectionBounds, 1.0f, 0.0f))
		{
			selectableProps.mouseSelectionBounds = { mouseSelectionBounds[0], mouseSelectionBounds[1] };
		}
	}
}

void Tga::CanvasDefinitionDocument::DrawUIElementProperties(UIElement& element)
{
	std::visit([&](auto& e)
		{
			using T = std::decay_t<decltype(e)>;

			if constexpr (std::is_same_v<T, UIImage>)
			{
				DrawImageProperties(e, "Image");
			}
			else if constexpr (std::is_same_v<T, UIText>)
			{
				DrawTextProperties(e, "Text");
			}
			else if constexpr (std::is_same_v<T, UIButton>)
			{
				DrawSelectableProperties(e.selectable, element.generalProperties);
				DrawImageProperties(e.buttonImage, "Button Background");
				DrawTextProperties(e.buttonText, "Button Text");
			}
			else if constexpr (std::is_same_v<T, UIToggle>)
			{
				DrawSelectableProperties(e.selectable, element.generalProperties);
				DrawImageProperties(e.backgroundImage, "Toggle Background");
				DrawImageProperties(e.checkmarkImage, "Toggle Checkmark");

				if (ImGui::CollapsingHeader("Toggle Values", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Default On", &e.defaultOn);
					e.isOn = e.defaultOn;
				}
			}
			else if constexpr (std::is_same_v<T, UISlider>)
			{
				DrawSelectableProperties(e.selectable, element.generalProperties);
				DrawImageProperties(e.backgroundImage, "Slider Background");
				DrawImageProperties(e.fillImage, "Slider Fill");
				DrawImageProperties(e.handleBarImage, "Slider Handle");

				if (ImGui::CollapsingHeader("Slider Values", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::DragFloat("Default Value", &e.defaultValue, 0.01f, e.minValue, e.maxValue);
					e.currentValue = e.defaultValue;
					ImGui::DragFloat("Min Value", &e.minValue, 0.01f, 0.f, e.maxValue);
					ImGui::DragFloat("Max Value", &e.maxValue, 0.01f, e.minValue, 100.f);
				}
			}
		}, element.uiElementProperties);
}

void Tga::CanvasDefinitionDocument::DrawImageProperties(UIImage& img, const char* label)
{
	if (!ImGui::CollapsingHeader(label))
		return;

	ImGui::PushID(label);

	const SceneReference& ref = img.sceneReference.Get();

	ImGui::Text("Sprite:");
	ImGui::Text(ref.path.IsEmpty() ? "None" : ref.path.GetString());

	StringId newValue = Tga::GetAssetBrowserSelection();
	if (!newValue.IsEmpty())
	{
		if (ImGui::Button("Set From Asset Browser"))
		{
			img.sceneReference.Edit().path = newValue;
		}
	}

	if (ImGui::Button("Clear"))
	{
		img.sceneReference.Edit().path = {};
	}

	ImGui::Separator();

	float tint[4] = { img.tint.r, img.tint.g, img.tint.b, img.tint.a };
	if (ImGui::ColorEdit4("Tint", tint))
	{
		img.tint = { tint[0], tint[1], tint[2], tint[3] };
	}

	ImGui::PopID();
}

void Tga::CanvasDefinitionDocument::DrawTextProperties(UIText& text, const char* label)
{
	if (!ImGui::CollapsingHeader(label))
		return;

	ImGui::PushID(label);

	ImGui::InputTextMultiline("Text", text.text, sizeof(text.text));

	ImGui::Separator();

	ImGui::DragFloat("Font Scale", &text.fontScale, 0.1f, 0.f, 100.f);

	const char* fontSizeNames[] =
	{
		"6", "8", "9", "10", "11", "12",
		"14", "18", "24", "30", "36",
		"48", "60", "72"
	};

	int fontSize = FontSizeToEnumIndex(static_cast<FontSize>(text.fontSize));
	if (ImGui::BeginCombo("Font Size", fontSizeNames[fontSize]))
	{
		for (int i = 0; i < 14; ++i)
		{
			bool selected = (fontSize == i);
			if (ImGui::Selectable(fontSizeNames[i], selected))
				fontSize = i;

			if (selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	text.fontSize = static_cast<int>(EnumIndexToFontSize(fontSize));

	const char* hAlignNames[] = { "Left", "Center", "Right" };
	int h = static_cast<int>(text.horizontalAlign);

	if (ImGui::BeginCombo("Horizontal Align", hAlignNames[h]))
	{
		for (int i = 0; i < 3; ++i)
		{
			bool selected = (h == i);
			if (ImGui::Selectable(hAlignNames[i], selected))
				h = i;

			if (selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	text.horizontalAlign = static_cast<HorizontalAlign>(h);

	const char* vAlignNames[] = { "Top", "Middle", "Bottom" };
	int v = static_cast<int>(text.verticalAlign);

	if (ImGui::BeginCombo("Vertical Align", vAlignNames[v]))
	{
		for (int i = 0; i < 3; ++i)
		{
			bool selected = (v == i);
			if (ImGui::Selectable(vAlignNames[i], selected))
				v = i;

			if (selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	text.verticalAlign = static_cast<VerticalAlign>(v);

	ImGui::Separator();

	float tint[4] = { text.tint.r, text.tint.g, text.tint.b, text.tint.a };
	if (ImGui::ColorEdit4("Tint", tint))
	{
		text.tint = { tint[0], tint[1], tint[2], tint[3] };
	}

	ImGui::Separator();

	const SceneReference& ref = text.sceneReference.Get();

	ImGui::Text("Font:");
	ImGui::Text(ref.path.IsEmpty() ? "None" : ref.path.GetString());

	StringId newValue = Tga::GetAssetBrowserSelection();
	if (!newValue.IsEmpty())
	{
		if (ImGui::Button("Set Font From Asset Browser"))
		{
			text.sceneReference.Edit().path = newValue;
		}
	}

	if (ImGui::Button("Clear Font"))
	{
		text.sceneReference.Edit().path = {};
	}

	ImGui::PopID();
}

struct CreateScriptData
{
	char name[MAX_OBJECTDEFINTION_TEXT_LENGTH];
};

struct CreateVariableData
{
	char name[MAX_OBJECTDEFINTION_TEXT_LENGTH];
	StringId typeName;
};

#pragma region Must Have Overrides


void CanvasDefinitionDocument::HandleDrop()
{
}

void CanvasDefinitionDocument::BeginDragSelection(Vector2f mousePos)
{
	mousePos;
}

void CanvasDefinitionDocument::EndDragSelection(Vector2f mousePos, bool isShiftDown)
{
	mousePos;
	isShiftDown;
}

void CanvasDefinitionDocument::ClickSelection(Vector2f mousePos, uint32_t selectedId, bool isShiftDown)
{
	mousePos;
	isShiftDown;
	selectedId;
}

void CanvasDefinitionDocument::BeginTransformation()
{

}

void CanvasDefinitionDocument::UpdateTransformation(const Vector3f& referencePosition, const Matrix4x4f& transform)
{
	referencePosition;
	transform;
}

void CanvasDefinitionDocument::EndTransformation()
{
}

Vector3f CanvasDefinitionDocument::CalculateSelectionPosition()
{
	return {};
}

Matrix4x4f CanvasDefinitionDocument::CalculateSelectionOrientation()
{
	return {};
}

bool CanvasDefinitionDocument::HasTransformableSelection()
{
	return false;
}
#pragma endregion


