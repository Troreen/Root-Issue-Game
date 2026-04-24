#include "stdafx.h"
#include "ObjectDefinitionDocument.h"

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
#include <unordered_set>

constexpr int MAX_OBJECTDEFINTION_TEXT_LENGTH = 256;
constexpr std::string_view ANIMATION_GRAPH_DIRECTORY = "animations/Animation Graphs";

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
	Color ambientColor = {0.1f, 0.5f, 0.8f};
	float ambientColorMultiplier = 1.0f;
	Color directionalLightColor = {0.9f, 0.7f, 0.5f };
	float directionalLightColorMultiplier = 1.4f;
};

static ObjectEditorPreviewSettings locPreviewSettings = {};

namespace
{
	std::string BuildAnimationGraphScriptPrefix(const std::string_view& aObjectPath)
	{
		std::filesystem::path scriptPrefix = std::filesystem::path(aObjectPath);
		scriptPrefix.replace_extension("");

		std::string prefix = scriptPrefix.generic_string();
		for (char& character : prefix)
		{
			if (character == '/')
			{
				character = '_';
			}
		}

		return prefix;
	}

	std::filesystem::path BuildAnimationGraphScriptPath(const std::string_view& aObjectPath, const std::string_view& aScriptName)
	{
		return std::filesystem::path(ANIMATION_GRAPH_DIRECTORY) / (BuildAnimationGraphScriptPrefix(aObjectPath) + "_" + std::string(aScriptName));
	}

	std::string GetAnimationGraphDirectoryString()
	{
		return std::filesystem::path(ANIMATION_GRAPH_DIRECTORY).string();
	}

	bool HasPrefix(const std::string_view& aValue, const std::string_view& aPrefix)
	{
		return aValue.size() >= aPrefix.size() && aValue.compare(0, aPrefix.size(), aPrefix) == 0;
	}

	std::string GetDisplayNameForScript(
		const std::string_view& aScriptPath,
		const std::string_view& aLegacyPrefix,
		const std::string_view& aAnimationGraphPrefix)
	{
		if (HasPrefix(aScriptPath, aLegacyPrefix) && aScriptPath.size() > aLegacyPrefix.size())
		{
			return std::string(aScriptPath.substr(aLegacyPrefix.size() + 1));
		}

		const std::string filename = std::filesystem::path(aScriptPath).filename().string();
		if (HasPrefix(filename, aAnimationGraphPrefix) && filename.size() > aAnimationGraphPrefix.size())
		{
			return filename.substr(aAnimationGraphPrefix.size());
		}

		return std::string(aScriptPath);
	}
}

static void UpdatePreviewShaders()
{
	locPreviewSettings.previewModelShader = {};
	locPreviewSettings.previewModelShader.Init("Shaders/PbrModelShaderVS", locPreviewSettings.previewPixelShaderPath.GetString());

	locPreviewSettings.previewSpriteShader = {};
	locPreviewSettings.previewSpriteShader.Init("Shaders/instanced_sprite_shader_VS", locPreviewSettings.previewPixelShaderPath.GetString());
}

void ObjectDefinitionDocument::Init(std::string_view aPath)
{
	Document::Init(aPath);

	myViewport.Init();
	myViewport.GetGrid().SetGridLineExtreme(400.0f);

	if (locPreviewSettings.previewPixelShaderPath.IsEmpty())
	{
		locPreviewSettings.previewPixelShaderPath = "shaders/model_shader_PS"_tgaid;
		UpdatePreviewShaders();
	}

	std::filesystem::path path = aPath;
	std::string filename = path.stem().string();

	StringId nameId = StringRegistry::RegisterOrGetString(filename.data());
	myObjectDefinition = Editor::GetEditor()->GetSceneObjectDefinitionManager().Get(nameId);

	char buffer[512];
	char asterix[2] = { 0, 0 };

	sprintf_s(buffer, "%s%s###Document:%s", myObjectDefinition->GetName().GetString(), asterix, myObjectDefinition->GetPath());
	myImGuiName = StringRegistry::RegisterOrGetString(buffer);

	sprintf_s(buffer, "ObjectDefinition##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::ObjectDefinition] = buffer;
	sprintf_s(buffer, "Properties##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::Properties] = buffer;
	sprintf_s(buffer, "Viewport##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::Viewport] = buffer;
	sprintf_s(buffer, "Script##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::Script] = buffer;
	sprintf_s(buffer, "Visual Preview Settings##Document:%s", aPath.data());
	myPanelWindowNames[(size_t)Panels::VisualPreviewSettings] = buffer;
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

	Vector3f cameraRotation = { 45, 45, 0 };

	camera.GetTransform().SetRotation(cameraRotation);
	myViewport.SetCameraRotation(cameraRotation);
	camera.GetTransform().SetPosition((camera.GetTransform().GetForward() * -myViewport.GetCameraFocusDistance()));
}

void ObjectDefinitionDocument::Save()
{
	myObjectDefinition->Save();

	mySaveUndoStackSize = myUndoStackSize;
}

void ObjectDefinitionDocument::Update(float aTimeDelta, InputManager& inputManager)
{
	aTimeDelta; inputManager;

	// clearing out caches every frame to support updates to assets while the editor is running
	myCache.ClearCache();

	const Camera& renderCamera = myViewport.GetCamera();
	Frustum frustum = CalculateFrustum(renderCamera);

	{
		auto& engine = *Tga::Engine::GetInstance();
		auto& graphicsStateStack = engine.GetGraphicsEngine().GetGraphicsStateStack();

		myViewport.BeginDraw();
		
		{
			myViewport.SetupIdPass();

			DrawParameters drawParameters = {
				.useIdShader = true,
				.drawBounds = false,
				.boundsColor = {},
				.cache = myCache,
				.frustum = frustum,
				.viewport = myViewport,
				.overrideModelShader = nullptr,
				.previewPoses = &myLivePreviewData.poses
			};

			std::span<const ScenePropertyDefinition> properties = myObjectDefinition->GetProperties();
			for (int propertyIndex = 0; propertyIndex < properties.size(); propertyIndex++)
			{
				const ScenePropertyDefinition& prop = properties[propertyIndex];

				myViewport.SetObjectAndSelectionId(1 + propertyIndex, prop.name == mySelectedProperty ? 1 + propertyIndex : 0, P4::FileInfo());

				DrawSceneProperty(prop, 1.f, drawParameters);
			}
		}

		{
			DrawParameters drawParameters = {
				.useIdShader = false,
				.drawBounds = false,
				.boundsColor = {},
				.cache = myCache,
				.frustum = frustum,
				.viewport = myViewport,
				.overrideModelShader = &locPreviewSettings.previewModelShader,
				.previewPoses = &myLivePreviewData.poses
			};


			myViewport.SetupColorPass();

			locPreviewSettings.ambientLight.color = locPreviewSettings.ambientColorMultiplier * locPreviewSettings.ambientColor;

			graphicsStateStack.SetAmbientLight(locPreviewSettings.ambientLight);
			graphicsStateStack.SetDirectionalLight(DirectionalLight{ Matrix4x4f::CreateFromRollPitchYaw({locPreviewSettings.directionalLightPitch, locPreviewSettings.directionalLightYaw, 0.f}), locPreviewSettings.directionalLightColorMultiplier * locPreviewSettings.directionalLightColor, 0.f });

			std::span<const ScenePropertyDefinition> properties = myObjectDefinition->GetProperties();
			for (int propertyIndex = 0; propertyIndex < properties.size(); propertyIndex++)
			{
				const ScenePropertyDefinition& prop = properties[propertyIndex];

				DrawSceneProperty(prop, 1.f, drawParameters);
			}
		}

		myViewport.EndDraw();
	}

	char buffer[512];
	char asterix[2] = { 0, 0 };

	// Todo: all of this base imgui stuff should move to the Document base class
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
			ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::VisualPreviewSettings].c_str(), left);
			
			if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
			{
				ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::LivePreview].c_str(), left);
			}

			ImGui::DockBuilderDockWindow(myPanelWindowNames[(size_t)Panels::ObjectDefinition].c_str(), left);

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
	ImGui::Begin(myPanelWindowNames[(size_t)Panels::Viewport].c_str());
	ImGui::PopStyleVar(1);

	isViewportOrPropertiesFocused = isViewportOrPropertiesFocused || ImGui::IsWindowFocused();
	myViewport.DrawAndUpdateViewportWindow(aTimeDelta, *this);

	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	ImGui::Begin(myPanelWindowNames[(size_t)Panels::ObjectDefinition].c_str());

	DrawObjectDefinitionPanel();

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
			EditorScriptManager::GetInstance().DisplayEditor(myActiveScript, myLivePreviewData.pinToTrigger, myLivePreviewData.mode==LivePreviewMode::Running);
		}
		ImGui::End();
	}

	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	isViewportOrPropertiesFocused = isViewportOrPropertiesFocused || ImGui::IsWindowFocused();
	ImGui::Begin(myPanelWindowNames[(size_t)Panels::Properties].c_str());

	DrawPropertyPanel();

	ImGui::End();

	ImGui::SetNextWindowClass(&myDocumentWindowClass);

	ImGui::Begin(myPanelWindowNames[(size_t)Panels::VisualPreviewSettings].c_str());

	DrawPreviewSettings();

	ImGui::End();

	if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
	{
		ImGui::Begin(myPanelWindowNames[(size_t)Panels::LivePreview].c_str());

		DrawAndUpdateLivePreview(aTimeDelta);

		ImGui::End();
	}
}

void ObjectDefinitionDocument::OnAction(CommandManager::Action action)
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

struct CreateScriptData
{
	char name[MAX_OBJECTDEFINTION_TEXT_LENGTH];
};

struct CreateVariableData
{
	char name[MAX_OBJECTDEFINTION_TEXT_LENGTH];
	StringId typeName;
};

void ObjectDefinitionDocument::DrawObjectDefinitionPanel()
{
	static CreateVariableData locCreateVariableData;
	static CreateVariableData locCreateScriptData;

	// todo: add option to select parent object definition here
	// todo: add list of scripts here!

	ImGuiTreeNodeFlags sectionFlags = ImGuiTreeNodeFlags_DefaultOpen;
	ImGuiTreeNodeFlags categoryFlags = ImGuiTreeNodeFlags_DefaultOpen;
	ImGuiTreeNodeFlags itemFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
	{
		bool showScripts = ImGui::TreeNodeEx("Scripts", sectionFlags);
		ImGui::SameLine();
		if (ImGui::SmallButton("Add##Scripts"))
		{
			strncpy_s(locCreateScriptData.name, "untitled", sizeof(locCreateScriptData.name));
			locCreateScriptData.name[sizeof(locCreateScriptData.name) - 1] = '\0';

			ImGui::OpenPopup("Add Script");
		}

		if (ImGui::BeginPopupModal("Add Script", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{


			ImGui::InputText("##Name", locCreateScriptData.name, IM_ARRAYSIZE(locCreateScriptData.name), ImGuiInputTextFlags_AutoSelectAll);

			ImGui::Separator();

			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				std::filesystem::path path = BuildAnimationGraphScriptPath(myObjectDefinition->GetPath(), locCreateScriptData.name);

				std::string pathString = path.string();

				EditorScriptManager& editorScriptManager = EditorScriptManager::GetInstance();

				auto& script = editorScriptManager.CreateNewScript(pathString);

				std::shared_ptr<CreateScriptCommand> createCommand = std::make_shared<CreateScriptCommand>(pathString, script, editorScriptManager.GetSelection(pathString) );
				CommandManager::DoCommand(createCommand);

				ImGui::CloseCurrentPopup();
			}

			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (showScripts)
		{
			static std::vector<std::string_view> scripts;
			static std::unordered_set<std::string_view> uniqueScripts;

			std::filesystem::path objectPath = myObjectDefinition->GetPath();
			objectPath.replace_extension("");

			scripts.clear();
			uniqueScripts.clear();

			EditorScriptManager& editorScriptManager = EditorScriptManager::GetInstance();

			std::string objectPathString = objectPath.string();
			const std::string animationGraphDirectory = GetAnimationGraphDirectoryString();
			const std::string animationGraphPrefix = BuildAnimationGraphScriptPrefix(myObjectDefinition->GetPath()) + "_";
			editorScriptManager.GetAllScriptsThatStartsWithPath(objectPathString, scripts);

			std::vector<std::string_view> animationGraphScripts;
			editorScriptManager.GetAllScriptsThatStartsWithPath(animationGraphDirectory, animationGraphScripts);

			for (const std::string_view& script : animationGraphScripts)
			{
				const std::filesystem::path scriptPath(script);
				const std::string filename = scriptPath.filename().string();

				if (HasPrefix(filename, animationGraphPrefix))
				{
					scripts.push_back(script);
				}
			}

			for (auto s : scripts)
			{
				if (!uniqueScripts.insert(s).second)
				{
					continue;
				}

				ImGuiTreeNodeFlags flags = itemFlags;

				if (mySelectedScript == s)
					flags |= ImGuiTreeNodeFlags_Selected;

				const std::string displayName = GetDisplayNameForScript(s, objectPathString, animationGraphPrefix);
				ImGui::TreeNodeEx(displayName.c_str(), flags);

				if (ImGui::IsItemClicked())
				{
					mySelectedProperty = {};
					mySelectedScript = s;
				}
				
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
				{
					myActiveScript = s;
				}
					

			}
			ImGui::TreePop();
		}
	}


	bool showVariables = ImGui::TreeNodeEx("Properties", sectionFlags);
	ImGui::SameLine();
	if (ImGui::SmallButton("Add##Variable"))
	{
		strncpy_s(locCreateVariableData.name, "untitled", sizeof(locCreateVariableData.name));
		locCreateVariableData.name[sizeof(locCreateVariableData.name) - 1] = '\0';
		locCreateVariableData.typeName = PropertyTypeRegistry::GetAllPropertyNames()[0];

		ImGui::OpenPopup("Add Property");
	}

	if (ImGui::BeginPopupModal("Add Property", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{

		ImGui::InputText("##Name", locCreateVariableData.name, IM_ARRAYSIZE(locCreateVariableData.name), ImGuiInputTextFlags_AutoSelectAll);

		if (ImGui::BeginCombo("##Type", locCreateVariableData.typeName.GetString()))
		{
			std::span<StringId> allTypes = PropertyTypeRegistry::GetAllPropertyNames();

			for (const StringId& name : allTypes)
			{
				bool isSelected = name == locCreateVariableData.typeName;
				if (ImGui::Selectable(name.GetString(), isSelected))
				{
					locCreateVariableData.typeName = name;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Separator();

		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			const PropertyTypeBase* type = PropertyTypeRegistry::GetPropertyType(locCreateVariableData.typeName);

			ScenePropertyDefinition newProperty = {};
			newProperty.name = StringRegistry::RegisterOrGetString(locCreateVariableData.name);
			newProperty.type = type;
			newProperty.value = Property(type);
			newProperty.flags = ScenePropertyFlags::None;

			std::shared_ptr<ChangePropertiesCommand> command = std::make_shared<ChangePropertiesCommand>(*myObjectDefinition, ChangePropertiesCommand::Action::Add, newProperty, ScenePropertyDefinition{});
			CommandManager::DoCommand(command);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (showVariables)
	{
		StringId groupName;
		bool showGroup = true;

		std::span<const ScenePropertyDefinition> properties = myObjectDefinition->GetProperties();
		for (int i = 0; i < properties.size(); i++)
		{
			if (properties[i].groupName != groupName)
			{
				if (!groupName.IsEmpty() && showGroup)
				{
					ImGui::TreePop();
				}

				groupName = properties[i].groupName;
				showGroup = ImGui::TreeNodeEx(groupName.GetString(), categoryFlags);
			}

			if (showGroup)
			{
				ImGuiTreeNodeFlags flags = itemFlags;

				if (mySelectedProperty == properties[i].name)
					flags |= ImGuiTreeNodeFlags_Selected;

				ImGui::TreeNodeEx(properties[i].name.GetString(), flags);
				if(ImGui::BeginDragDropSource()) 
				{
					struct Payload { PropertyTypeId type; StringId name; };
					Payload payload = { .type = properties[i].type->GetTypeId(), .name = properties[i].name };

					ImGui::SetDragDropPayload("property_payload", (void*)&payload, sizeof(payload));
					char buffer[255];
					sprintf_s(buffer, "property_name_payload");
					sprintf_s(buffer, "%s, (Type: %s)", properties[i].name.GetString(), properties[i].type->GetName().GetString());
					ImGui::Text(buffer);
					ImGui::EndDragDropSource();
				}
				// Todo: probably show type, and value if compact enough.

				else if (ImGui::IsItemClicked())
				{
					mySelectedProperty = properties[i].name;
					mySelectedScript.clear();
				}

				if (ImGui::BeginPopupContextItem(properties[i].name.GetString()))
				{
					if (ImGui::Selectable("Remove"))
					{
						std::shared_ptr<ChangePropertiesCommand> command = std::make_shared<ChangePropertiesCommand>(*myObjectDefinition, ChangePropertiesCommand::Action::Remove, ScenePropertyDefinition{}, properties[i]);
						CommandManager::DoCommand(command);
					}
					ImGui::EndPopup();
				}
			}
		}

		if (!groupName.IsEmpty() && showGroup)
		{
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}

void ObjectDefinitionDocument::DrawPropertyPanel()
{
	char buffer[MAX_OBJECTDEFINTION_TEXT_LENGTH];

	std::span<const ScenePropertyDefinition> properties = myObjectDefinition->GetProperties();

	int selectedPropertyIndex = -1;

	for (int i = 0; i < properties.size(); i++)
	{
		if (mySelectedProperty == properties[i].name)
		{
			selectedPropertyIndex = i;
			break;
		}
	}

	if (selectedPropertyIndex != -1)
	{
		const ScenePropertyDefinition& property = properties[selectedPropertyIndex];
		ScenePropertyDefinition newProperty = property;
		bool hasChange = false;

		if (PropertyEditor::PropertyHeader("Variable Definition"))
		{
			if (PropertyEditor::BeginPropertyTable())
			{
				{
					strncpy_s(buffer, property.name.GetString(), sizeof(buffer));
					buffer[sizeof(buffer) - 1] = '\0';

					PropertyEditor::PropertyLabel();

					ImGui::Text("Name");
					PropertyEditor::HelpMarker("Used to identify a variable. Has to be unique within an Object Definition");

					PropertyEditor::PropertyValue();

					ImGui::InputText("##Name", buffer, IM_ARRAYSIZE(buffer));

					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						StringId newName = StringRegistry::RegisterOrGetString(buffer);
						if (newProperty.name != newName)
						{
							newProperty.name = newName;
							hasChange = true;
						}
					}

				}

				{
					strncpy_s(buffer, property.groupName.GetString(), sizeof(buffer));
					buffer[sizeof(buffer) - 1] = '\0';

					PropertyEditor::PropertyLabel();
					ImGui::Text("Group Name");
					PropertyEditor::HelpMarker("Group name is used to group properties in the property list");

					PropertyEditor::PropertyValue();
					ImGui::InputText("##Group Name", buffer, IM_ARRAYSIZE(buffer));

					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						StringId newGroupName = StringRegistry::RegisterOrGetString(buffer);
						if (newProperty.groupName != newGroupName)
						{
							newProperty.groupName = newGroupName;
							hasChange = true;
						}
					}
				}

				{
					strncpy_s(buffer, property.description.GetString(), sizeof(buffer));
					buffer[sizeof(buffer) - 1] = '\0';

					PropertyEditor::PropertyLabel();
					ImGui::Text("Description");
					PropertyEditor::HelpMarker("Description of what the property is used for. Shows up in tooltips");

					PropertyEditor::PropertyValue();
					ImGui::InputText("##Description", buffer, IM_ARRAYSIZE(buffer));

					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						StringId newDescription = StringRegistry::RegisterOrGetString(buffer);
						if (newProperty.description != newDescription)
						{
							newProperty.description = newDescription;
							hasChange = true;
						}
					}
				}

				{
					bool currentAllowInstanceOverride = (property.flags & ScenePropertyFlags::IsPerInstance) != ScenePropertyFlags::None;
					bool newAllowInstanceOverride = currentAllowInstanceOverride;

					PropertyEditor::PropertyLabel();
					ImGui::Text("Instance Editable");
					PropertyEditor::HelpMarker("Controls if the property is editable in instances of the objects. If true you can edit this property in scenes the object is placed");

					PropertyEditor::PropertyValue();
					ImGui::Checkbox("##Instance Editable", &newAllowInstanceOverride);

					if (newAllowInstanceOverride != currentAllowInstanceOverride)
					{
						newProperty.flags = (property.flags & ~ScenePropertyFlags::IsPerInstance) | (newAllowInstanceOverride ? ScenePropertyFlags::IsPerInstance : ScenePropertyFlags::None);
						hasChange = true;
					}
				}

				if (Editor::GetEditor()->GetEditorConfiguration().enableVisualScripts)
				{
					bool currentIsDynamic = (property.flags & ScenePropertyFlags::IsDynamic) != ScenePropertyFlags::None;
					bool newIsDynamic = currentIsDynamic;

					PropertyEditor::PropertyLabel();
					ImGui::Text("Is Dynamic");
					PropertyEditor::HelpMarker("Dynamic properties are editable from scripts");

					PropertyEditor::PropertyValue();
					ImGui::Checkbox("##Is Dynamic", &newIsDynamic);

					if (newIsDynamic != currentIsDynamic)
					{
						newProperty.flags = (property.flags & ~ScenePropertyFlags::IsDynamic) | (newIsDynamic ? ScenePropertyFlags::IsDynamic : ScenePropertyFlags::None);
						hasChange = true;
					}
				}

				{
					StringId currentPropertyType = newProperty.type->GetName();
					StringId newPropertyType = currentPropertyType;

					PropertyEditor::PropertyLabel();
					ImGui::Text("Type");
					PropertyEditor::PropertyValue();
					if (ImGui::BeginCombo("##Type", currentPropertyType.GetString()))
					{
						std::span<StringId> allTypes = PropertyTypeRegistry::GetAllPropertyNames();

						for (const StringId& name : allTypes)
						{
							bool isSelected = name == currentPropertyType;
							if (ImGui::Selectable(name.GetString(), isSelected))
							{
								newPropertyType = name;
							}

							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}

						ImGui::EndCombo();
					}

					if (newPropertyType != currentPropertyType)
					{
						newProperty.type = PropertyTypeRegistry::GetPropertyType(newPropertyType);
						newProperty.value = Property(newProperty.type);

						hasChange = true;
					}
				}

				PropertyEditor::EndPropertyTable();
			}
		}

		if (PropertyEditor::PropertyHeader("Default Value"))
		{
			if (PropertyEditor::BeginPropertyTable())
			{			
				if (newProperty.value.ShowImGuiEditor(property.name.GetString(), property.description.GetString()))
				{
					hasChange = true;
				}	

				PropertyEditor::EndPropertyTable();
			}
		}


		if (hasChange)
		{
			std::shared_ptr<ChangePropertiesCommand> command = std::make_shared<ChangePropertiesCommand>(*myObjectDefinition, ChangePropertiesCommand::Action::Edit, newProperty, property);
			CommandManager::DoCommand(command);

			mySelectedProperty = newProperty.name;
		}
	}
}

void ObjectDefinitionDocument::DrawPreviewSettings()
{
	if (PropertyEditor::PropertyHeader("Default Value"))
	{
		if (PropertyEditor::BeginPropertyTable())
		{
			PropertyEditor::PropertyLabel();
			ImGui::Text("Preview Pixel Shader");
			PropertyEditor::PropertyValue();
			ImGui::Text(locPreviewSettings.previewPixelShaderPath.GetString());


			if (ImGui::Button("Set To Unlit Textured"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/model_shader_PS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Unlit Vertex Color"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/model_shader_vertex_color_PS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Unlit Textured + Vertex Color"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/model_shader_vertex_color_textured_PS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Lambert Lighting"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/LambertModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To PBR Lighting"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/PbrModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Vertex Normal Debug"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/DebugVertexNormalModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Pixel Normal Debug"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/DebugPixelNormalModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Roughness Debug"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/DebugRoughnessModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Metalness Debug"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/DebugMetalnessModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Ambient Occlusion Debug"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/DebugAmbientOcclusionModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set To Emissive Debug"))
			{
				locPreviewSettings.previewPixelShaderPath = "shaders/DebugEmissiveModelShaderPS"_tgaid;
				UpdatePreviewShaders();
			}
			if (ImGui::Button("Set From AssetBrowser"))
			{
				StringId newValue = Editor::GetEditor()->GetAssetBrowser().GetSelectedAsset();
				std::string stringWithExtension = newValue.GetString();
				std::string::size_type pos = stringWithExtension.find(".hlsl");
				if (pos != std::string::npos)
				{
					std::string withoutPath = stringWithExtension.substr(0, pos);
					locPreviewSettings.previewPixelShaderPath = StringRegistry::RegisterOrGetString(withoutPath);
					UpdatePreviewShaders();
				}
			}
			PropertyEditor::PropertyLabel();
			ImGui::Text("Directional Light Yaw");
			PropertyEditor::PropertyValue();
			ImGui::DragFloat("##Light Yaw", &locPreviewSettings.directionalLightYaw);

			PropertyEditor::PropertyLabel();
			ImGui::Text("Directional Light Pitch");
			PropertyEditor::PropertyValue();
			ImGui::DragFloat("##Light Pitch", &locPreviewSettings.directionalLightPitch);

			PropertyEditor::PropertyLabel();
			ImGui::Text("Directional Light Color");
			PropertyEditor::PropertyValue();
			ImGui::ColorEdit3("##Directional Light Color", &locPreviewSettings.directionalLightColor.r, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

			PropertyEditor::PropertyLabel();
			ImGui::Text("Directional Light Multiplier");
			PropertyEditor::PropertyValue();
			ImGui::DragFloat("##Directional Light Color", &locPreviewSettings.directionalLightColorMultiplier);

			PropertyEditor::PropertyLabel();
			ImGui::Text("Ambient Light Color");
			PropertyEditor::PropertyValue();
			ImGui::ColorEdit3("##Ambient Light Color", &locPreviewSettings.ambientColor.r, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

			PropertyEditor::PropertyLabel();
			ImGui::Text("Ambient Light Multiplier");
			PropertyEditor::PropertyValue();
			ImGui::DragFloat("##Ambient Light Color", &locPreviewSettings.ambientColorMultiplier);

			ImGui::PushID("Cubemap");

			PropertyEditor::PropertyLabel();
			ImGui::Text("Ambient Cube Map");
			PropertyEditor::PropertyValue();
			ImGui::Text(locPreviewSettings.cubeMapPath.GetString());

			if (ImGui::Button("Set From AssetBrowser"))
			{
				StringId newValue = Editor::GetEditor()->GetAssetBrowser().GetSelectedAsset();
				std::string stringWithExtension = newValue.GetString();
				std::string::size_type pos = stringWithExtension.find(".dds");
				if (pos != std::string::npos)
				{
					locPreviewSettings.cubeMapPath = StringRegistry::RegisterOrGetString(stringWithExtension);

					locPreviewSettings.ambientLight.type = AmbientLightType::Custom;
					locPreviewSettings.ambientLight.cubemap = Engine::GetInstance()->GetTextureManager().GetTexture(locPreviewSettings.cubeMapPath.GetString());
				}
			}

			if (ImGui::Button("Set To Uniform"))
			{
				locPreviewSettings.cubeMapPath = {};
				locPreviewSettings.ambientLight.type = AmbientLightType::Uniform;
			}

			if (ImGui::Button("Set To Above Horizon"))
			{
				locPreviewSettings.cubeMapPath = {};
				locPreviewSettings.ambientLight.type = AmbientLightType::UniformAboveHorizon;
			}

			ImGui::PopID();

			PropertyEditor::EndPropertyTable();

		}
	}
}

void ObjectDefinitionDocument::DrawAndUpdateLivePreview(float deltaTime)
{
	static std::vector<StringId> scriptNames;
	static std::vector<std::string_view> scriptPaths;


	{
		scriptNames.clear();
		scriptPaths.clear();

		std::filesystem::path objectPath = myObjectDefinition->GetPath();
		objectPath.replace_extension("");

		EditorScriptManager& editorScriptManager = EditorScriptManager::GetInstance();

		std::string objectPathString = objectPath.string();
		editorScriptManager.GetAllScriptsThatStartsWithPath(objectPathString, scriptPaths);

		for (std::string_view s : scriptPaths)
		{
			scriptNames.push_back(StringRegistry::RegisterOrGetString(s.data() + objectPathString.length() + 1));
		}
	}

	if (scriptNames.empty())
	{
		ImGui::Text("No scripts found");
		ImGui::Text("Add scripts first to preview");

	}

	if (ImGui::BeginTable("Toolbar", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImVec2 toolbarItemSize = ImVec2(26, 28);

		if (ImGui::Selectable(ICON_LC_PLAY, myLivePreviewData.mode == LivePreviewMode::Running, myLivePreviewData.mode == LivePreviewMode::Running ? ImGuiSelectableFlags_Disabled : 0, toolbarItemSize))
		{
			if (myLivePreviewData.mode == LivePreviewMode::Stopped)
			{
				myLivePreviewData.poses.clear();
				myLivePreviewData.dynamicProperties.clear();
				myLivePreviewData.staticProperties.clear();
				myLivePreviewData.scriptInstances.clear();
				myLivePreviewData.frameNumber = 0;

				std::span<const ScenePropertyDefinition> properties = myObjectDefinition->GetProperties();

				for (auto p : properties)
				{
					if ((p.flags & ScenePropertyFlags::IsDynamic) != ScenePropertyFlags::None)
					{
						myLivePreviewData.dynamicProperties[p.name] = p.value;
					}
					else
					{
						myLivePreviewData.staticProperties[p.name] = p.value;
					}
				}

				if (myLivePreviewData.enabledScripts.empty())
				{
					for (const StringId scriptName : scriptNames)
					{
						myLivePreviewData.enabledScripts.insert(scriptName);
					}
				}

				for (int i = 0; i<scriptNames.size(); i++)
				{
					if (myLivePreviewData.enabledScripts.find(scriptNames[i]) == myLivePreviewData.enabledScripts.end())
						continue;

					std::shared_ptr<const Script> script = ScriptManager::GetScript(scriptPaths[i]);
					if (!script)
					{
						continue;
					}
					myLivePreviewData.scriptInstances.emplace_back(std::make_pair(scriptNames[i], ScriptRuntimeInstance{script}));
					myLivePreviewData.scriptInstances.back().second.Init();
				}
				
			}

			myLivePreviewData.mode = LivePreviewMode::Running;
		}
	
		ImGui::TableSetColumnIndex(1);

		if (ImGui::Selectable(ICON_LC_PAUSE, myLivePreviewData.mode == LivePreviewMode::Paused, myLivePreviewData.mode == LivePreviewMode::Running ? 0 : ImGuiSelectableFlags_Disabled, toolbarItemSize))
		{
			myLivePreviewData.mode = LivePreviewMode::Paused;
		}

		ImGui::TableSetColumnIndex(2);

		if (ImGui::Selectable(ICON_LC_SQUARE, myLivePreviewData.mode == LivePreviewMode::Stopped, myLivePreviewData.mode != LivePreviewMode::Stopped ? 0 : ImGuiSelectableFlags_Disabled, toolbarItemSize))
		{
			myLivePreviewData.mode = LivePreviewMode::Stopped;
			myLivePreviewData.poses.clear();
			myLivePreviewData.dynamicProperties.clear();
			myLivePreviewData.staticProperties.clear();
			myLivePreviewData.scriptInstances.clear();
			myLivePreviewData.frameNumber = 0;
		}

		ImGui::EndTable();
	}

	if (PropertyEditor::PropertyHeader("Scripts to preview") && PropertyEditor::BeginPropertyTable())
	{
		for (int i = 0; i < scriptNames.size(); i++)
		{
			StringId scriptName = scriptNames[i];
			ImGui::PushID(i);

			PropertyEditor::PropertyLabel();
			ImGui::Text(scriptName.GetString());

			PropertyEditor::PropertyValue();
			bool wasActive = myLivePreviewData.enabledScripts.find(scriptName) != myLivePreviewData.enabledScripts.end();
			bool isActive = wasActive;

			ImGui::Checkbox("##IsEnabled", &isActive);

			if (!myLivePreviewData.scriptInstances.empty())
			{
				int index = -1;
				for (int j = 0; j < myLivePreviewData.scriptInstances.size(); j++)
				{
					if (myLivePreviewData.scriptInstances[j].first == scriptName)
					{
						index = j;
						break;
					}
				}
				if (index != -1)
				{
					int previewSequenceNumber = myLivePreviewData.scriptInstances[index].second.GetScript().GetSequenceNumber();
					int currentSequenceNumber = ScriptManager::GetEditableScript(scriptPaths[i])->GetSequenceNumber();

					if (previewSequenceNumber != currentSequenceNumber)
					{
						// Script is running but out of date, show a warning
						ImGui::SameLine();

						ImGui::Text(ICON_LC_TRIANGLE_ALERT);
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("The running script is out of date. Restart the live preview to get latest changes.");
						}

					}
				}
				

			}

			if (isActive && !wasActive)
			{
				myLivePreviewData.enabledScripts.insert(scriptName);

				if (myLivePreviewData.mode != LivePreviewMode::Stopped)
				{
					std::shared_ptr<const Script> script = ScriptManager::GetScript(scriptPaths[i]);
					myLivePreviewData.scriptInstances.emplace_back(std::make_pair(scriptName, ScriptRuntimeInstance{ script }));
					myLivePreviewData.scriptInstances.back().second.Init();
				}
			}
			else if (!isActive && wasActive)
			{
				myLivePreviewData.enabledScripts.erase(scriptName);

				if (myLivePreviewData.mode != LivePreviewMode::Stopped)
				{
					for (int j = 0; j < myLivePreviewData.scriptInstances.size(); j++)
					{
						if (myLivePreviewData.scriptInstances[j].first == scriptName)
						{
							myLivePreviewData.scriptInstances.erase(myLivePreviewData.scriptInstances.begin() + j);
						}
					}
				}
			}

			ImGui::PopID();
		}

		PropertyEditor::EndPropertyTable();
	}

	if (myLivePreviewData.mode == LivePreviewMode::Running)
	{
		myLivePreviewData.frameNumber++;

		ScriptUpdateContext scriptUpdateContext;

		scriptUpdateContext.deltaTime = deltaTime;
		scriptUpdateContext.frameNumber = myLivePreviewData.frameNumber;
		scriptUpdateContext.dynamicProperties = &myLivePreviewData.dynamicProperties;
		scriptUpdateContext.staticProperties = &myLivePreviewData.staticProperties;

		for (auto& pair : myLivePreviewData.scriptInstances)
		{
			if (myLivePreviewData.pinToTrigger.id != ScriptPinId::InvalidId)
			{
				pair.second.TriggerPin(myLivePreviewData.pinToTrigger, scriptUpdateContext);
				myLivePreviewData.pinToTrigger.id = ScriptPinId::InvalidId;
			}
			pair.second.Update(scriptUpdateContext);
		}
		
		std::span<const ScenePropertyDefinition> properties = myObjectDefinition->GetProperties();
		for (int propertyIndex = 0; propertyIndex < properties.size(); propertyIndex++)
		{
			const ScenePropertyDefinition& property = properties[propertyIndex];

			if (property.type == GetPropertyType<CopyOnWriteWrapper<SceneModel>>())
			{
				StringId path = property.value.Get<CopyOnWriteWrapper<SceneModel>>()->Get().path;
				std::shared_ptr<Model> model = myCache.GetModelUsingCache(path);

				if (model)
				{
					// todo: remove this per frame allocation...
					std::string poseNameString = property.name.GetString();
					poseNameString += "_pose";

					StringId poseName = StringRegistry::RegisterOrGetString(poseNameString);

					auto it = myLivePreviewData.dynamicProperties.find(poseName);
					if (it != myLivePreviewData.dynamicProperties.end())
					{
						PoseAndMotion* poseAndMotion = it->second.Get<PoseAndMotion>();

						if (poseAndMotion && poseAndMotion->poseGenerator)
						{
							std::string syncedTimeNameString = property.name.GetString();
							syncedTimeNameString += "_syncedTime";

							StringId syncedTimeName = StringRegistry::RegisterOrGetString(syncedTimeNameString);

							auto timeIt = myLivePreviewData.dynamicProperties.find(syncedTimeName);

							float syncedTime = 0.f;
							if (timeIt != myLivePreviewData.dynamicProperties.end())
							{
								float* time = timeIt->second.Get<float>();
								if (time)
									syncedTime = *time;
							}

							if (poseAndMotion->desiredSyncedPlaybackRateWeight > 0.f)
							{
								syncedTime += poseAndMotion->desiredSyncedPlaybackRate * deltaTime;
								syncedTime -= floor(syncedTime);
							}

							myLivePreviewData.dynamicProperties[syncedTimeName] = Property::Create<float>(syncedTime);

							PoseGenerationContext context = {};
							context.deltaTime = deltaTime;
							context.frameNumber = myLivePreviewData.frameNumber;
							context.syncedPlaybackTime = syncedTime;
							context.model = model;

							LocalSpacePose pose;
							poseAndMotion->poseGenerator->GeneratePose(context, pose);

							model->GetSkeleton()->ConvertPoseToModelSpace(pose, myLivePreviewData.poses[property.name]);
						}

						// removing pose every frame in case a script is stopped and the pose remains
						// otherwise we could get a dangling pointer
						myLivePreviewData.dynamicProperties.erase(it);
					}

				}
			}
		}
	}


	static std::set<StringId> propertyNames;

	propertyNames.clear();

	for (auto p : myLivePreviewData.dynamicProperties)
	{
		propertyNames.insert(p.first);
	}

	for (auto p : myLivePreviewData.staticProperties)
	{
		propertyNames.insert(p.first);
	}

	if (!propertyNames.empty() && PropertyEditor::PropertyHeader("Live Property Values") && PropertyEditor::BeginPropertyTable())
	{
		for (auto propertyName : propertyNames)
		{
			ImGui::PushID(propertyName.GetString());

			auto it = myLivePreviewData.dynamicProperties.find(propertyName);
			if (it == myLivePreviewData.dynamicProperties.end())
				it = myLivePreviewData.staticProperties.find(propertyName);

			it->second.ShowImGuiEditor(propertyName.GetString());

			ImGui::PopID();
		}

		PropertyEditor::EndPropertyTable();
	}
	
}

void ObjectDefinitionDocument::HandleDrop()
{

}

void ObjectDefinitionDocument::BeginDragSelection(Vector2f mousePos)
{
	mousePos;
}

void ObjectDefinitionDocument::EndDragSelection(Vector2f mousePos, bool isShiftDown)
{
	mousePos;
	isShiftDown;
}

void ObjectDefinitionDocument::ClickSelection(Vector2f mousePos, uint32_t selectedId, bool isShiftDown)
{
	mousePos;
	isShiftDown;

	if (selectedId > 0)
	{
		int propertyIndex = selectedId - 1;

		std::span<const ScenePropertyDefinition> properties = myObjectDefinition->GetProperties();

		if (propertyIndex < properties.size())
		{
			mySelectedProperty = properties[propertyIndex].name;
		}
	}
	else
	{
		mySelectedProperty = {};
	}
}

void ObjectDefinitionDocument::BeginTransformation()
{

}

void ObjectDefinitionDocument::UpdateTransformation(const Vector3f& referencePosition, const Matrix4x4f& transform)
{
	referencePosition;
	transform;
}

void ObjectDefinitionDocument::EndTransformation()
{
}

Vector3f ObjectDefinitionDocument::CalculateSelectionPosition()
{
	return {};
}

Matrix4x4f ObjectDefinitionDocument::CalculateSelectionOrientation()
{
	return {};
}

bool ObjectDefinitionDocument::HasTransformableSelection()
{
	return false;
}

void ObjectDefinitionDocument::SetActiveScript(const std::string_view& aScriptPath)
{
	mySelectedProperty = {};
	mySelectedScript = std::string(aScriptPath);
	myActiveScript = mySelectedScript;
}
