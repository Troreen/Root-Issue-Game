#include "UICanvas.h"
#include "Essentials.h"
#include "tge/graphics/GraphicsEngine.h"
#include "tge/graphics/GraphicsStateStack.h"
#include "tge/text/textservice.h"
#include <variant>

std::vector<UICanvas*> UICanvas::ourUICanvases;

std::unique_ptr<Tga::Camera> UICanvas::ourUICamera;

UICanvas::UICanvas()
{
    if (ourUICamera == nullptr)
    {
        ourUICamera = std::make_unique<Tga::Camera>();
        auto& textService = Tga::Engine::GetInstance()->GetTextService();
        textService.Init();
    }

    ourUICanvases.push_back(this);

    myKeyboardSliderSpeed = 1.f;
    myMouseSliderSpeed = .15f;
	mySelectedElementSizeMultiplier = 1.2f;
	myUISizeModificationSpeed = 20.f;
	mySliderFocusElementSizeMultiplier = 1.3f;
}

UICanvas::~UICanvas()
{
	if(mySelectedElement != nullptr && !Essentials::ShutdownQueued)
		mySelectedElement->definition->generalProperties.size = mySelectedOriginalSize;

    auto it = std::find(ourUICanvases.begin(), ourUICanvases.end(), this);
    if (it != ourUICanvases.end())
    {
        ourUICanvases.erase(it);
    }
}

void UICanvas::Init(const std::string& aCanvasName, Tga::CanvasObjectDefinitionManager& aManager)
{
	Tga::StringId canvasName = Tga::StringRegistry::RegisterOrGetString(aCanvasName);
    myCanvas = aManager.Get(canvasName);

    if (!myCanvas)
    {
        printf("Failed to load canvas: %s\n", canvasName.GetString());
        return;
    }

    myElements.clear();
    for (UIElement& e : myCanvas->GetUIElements())
    {
        myElements.push_back({ &e });
    }

    myRenderOrder = myCanvas->GetRenderOrder();
}

void UICanvas::UpdateAll()
{
    for (int i = 0; i < ourUICanvases.size(); i++)
    {
        ourUICanvases[i]->Update();
    }
}

void UICanvas::Update()
{
    if (myIsHidden || mySelectedElement == nullptr || Essentials::globalSceneManager->HasRequestedScene())
        return;

	float t = myUISizeModificationSpeed * Essentials::GetUnscaledDeltaTime();
	mySelectedElement->definition->generalProperties.size = 
	{
		std::lerp(mySelectedElement->definition->generalProperties.size.x, mySelectedOriginalSize.x * myCurrentSelectedSizeMultiplier, t), 
		std::lerp(mySelectedElement->definition->generalProperties.size.y, mySelectedOriginalSize.y * myCurrentSelectedSizeMultiplier, t)
	};

	myInputTimer -= Essentials::GetUnscaledDeltaTime();

    NavigationControl();
    InteractionControl();
}

void UICanvas::NavigationControl()
{
    Tga::Vector2f mousePos;
    mousePos = Essentials::globalInputManager->GetMousePosition();
    mousePos.y *= -1.f;
	mousePos.y += Essentials::GetResolution().y;

    Tga::Vector2f keyNavigationDirPressed;

	if (Essentials::globalInputManager->PressingToggleUp() || Essentials::globalInputManager->LeftStickHeldUp() && myInputTimer <= 0.0f)
	{
		keyNavigationDirPressed.y = 1.f;
		myInputTimer = myInputDelay;
	}
	else if (Essentials::globalInputManager->PressingToggleDown() || Essentials::globalInputManager->LeftStickHeldDown() && myInputTimer <= 0.0f)
	{
		keyNavigationDirPressed.y = -1.f;
		myInputTimer = myInputDelay;
	}

	if (Essentials::globalInputManager->PressingToggleRight() || Essentials::globalInputManager->LeftStickHeldRight() && myInputTimer <= 0.0f)
	{
		keyNavigationDirPressed.x = 1.f; 
		myInputTimer = myInputDelay;
	}
	else if (Essentials::globalInputManager->PressingToggleLeft() || Essentials::globalInputManager->LeftStickHeldLeft() && myInputTimer <= 0.0f)
	{
		keyNavigationDirPressed.x = -1.f; 
		myInputTimer = myInputDelay;
	}

	if (isFocused)
	{
		myCurrentSelectedSizeMultiplier = mySliderFocusElementSizeMultiplier;
		std::visit([&](auto& e) {

			using T = std::decay_t<decltype(e)>;
			if constexpr (std::is_same_v<T, UISlider>)
			{
				float dir = 0;

				if (Essentials::globalInputManager->IsKeyHeld(static_cast<int>(Keys::D))
					|| Essentials::globalInputManager->IsKeyHeld(static_cast<int>(Keys::RIGHT))
					|| Essentials::globalInputManager->LeftStickHeldRight())
				{
					dir = 1.f;
				}
				else if (Essentials::globalInputManager->IsKeyHeld(static_cast<int>(Keys::A))
					|| Essentials::globalInputManager->IsKeyHeld(static_cast<int>(Keys::LEFT))
					|| Essentials::globalInputManager->LeftStickHeldLeft())
				{
					dir = -1.f;
				}

				float speed = myKeyboardSliderSpeed;
				if (dir == 0)
				{
					dir = Essentials::globalInputManager->GetMouseDelta().x;
					speed = myMouseSliderSpeed;
				}

				if (dir != 0)
				{

					if (e.currentValue <= e.maxValue && e.currentValue >= e.minValue)
						e.currentValue += speed * dir * Essentials::GetUnscaledDeltaTime();
					else if (e.currentValue < e.minValue)
						e.currentValue = e.minValue;
					else if (e.currentValue > e.maxValue)
						e.currentValue = e.maxValue;
				}
			}

			}, mySelectedElement->definition->uiElementProperties);

		return;
	}
	else
	{
		myCurrentSelectedSizeMultiplier = mySelectedElementSizeMultiplier;
	}

	RuntimeUIElement* myPreviousSelectedElement = mySelectedElement;

	if (keyNavigationDirPressed != Tga::Vector2f{ 0.f,0.f })
	{
		//Essentials::myCursor->SetCursorVisible(false);
		myMouseActive = false;

		SelectableUIProperties* selectableProperties;

		std::visit([&](auto& e) {

			using T = std::decay_t<decltype(e)>;
			if constexpr (std::is_same_v<T, UISlider>)
			{
				selectableProperties = &e.selectable;
			}
			else if constexpr (std::is_same_v<T, UIToggle>)
			{
				selectableProperties = &e.selectable;
			}
			else if constexpr (std::is_same_v<T, UIButton>)
			{
				selectableProperties = &e.selectable;
			}

			}, mySelectedElement->definition->uiElementProperties);

		if (selectableProperties == nullptr)
			return;

		if (keyNavigationDirPressed.y == 1.f)
		{
			int nextIndex = selectableProperties->navigation[0];
			if (nextIndex != -1)
			{
				RuntimeUIElement* nextElement = &myElements[nextIndex];
				if (nextElement)
				{
					if (!GetIsHidden(nextElement->definition->generalProperties.name))
					{
						mySelectedElement = nextElement;
					}
				}
			}
		}
		else if (keyNavigationDirPressed.y == -1.f)
		{
			int nextIndex = selectableProperties->navigation[1];
			if (nextIndex != -1)
			{
				RuntimeUIElement* nextElement = &myElements[nextIndex];
				if (nextElement)
				{
					if (!GetIsHidden(nextElement->definition->generalProperties.name))
					{
						mySelectedElement = nextElement;
					}
				}
			}
		}

		if (keyNavigationDirPressed.x == -1.f)
		{
			int nextIndex = selectableProperties->navigation[2];
			if (nextIndex != -1)
			{
				RuntimeUIElement* nextElement = &myElements[nextIndex];
				if (nextElement)
				{
					if (!GetIsHidden(nextElement->definition->generalProperties.name))
					{
						mySelectedElement = nextElement;
					}
				}
			}
		}
		else if (keyNavigationDirPressed.x == 1.f)
		{
			int nextIndex = selectableProperties->navigation[3];
			if (nextIndex != -1)
			{
				RuntimeUIElement* nextElement = &myElements[nextIndex];
				if (nextElement)
				{
					if (!GetIsHidden(nextElement->definition->generalProperties.name))
					{
						mySelectedElement = nextElement;
					}
				}
			}

		}
	}
	else if(mousePos != myLastMousePosition)
	{
		Tga::Vector2f mouseDelta = Essentials::globalInputManager->GetMouseDelta();
		if (mouseDelta.x > 0.f || mouseDelta.y > 0.f)
		{
			//Essentials::myCursor->SetCursorVisible(true);
			myMouseActive = true;
		}

		myLastMousePosition = mousePos;

		for (int i = 0; i < myElements.size(); i++)
		{
			SelectableUIProperties* selectableProperties = nullptr;

			std::visit([&](auto& e) {

				using T = std::decay_t<decltype(e)>;
				if constexpr (std::is_same_v<T, UISlider>)
				{
					selectableProperties = &e.selectable;
				}
				else if constexpr (std::is_same_v<T, UIToggle>)
				{
					selectableProperties = &e.selectable;
				}
				else if constexpr (std::is_same_v<T, UIButton>)
				{
					selectableProperties = &e.selectable;
				}

			}, myElements[i].definition->uiElementProperties);

			if (selectableProperties == nullptr)
				continue;

			if (CanvasObjectDefinition::PosInside(mousePos, myElements[i].definition->generalProperties, *myCanvas, Tga::Vector2i(Essentials::globalEngine->GetRenderSize().x, Essentials::globalEngine->GetRenderSize().y), selectableProperties))
			{
				if (myElements[i].definition->generalProperties.hide || (myElements[i].definition->generalProperties.groupIndex != -1 && myElements[myElements[i].definition->generalProperties.groupIndex].definition->generalProperties.hide))
					continue;

				mySelectedElement = &myElements[i];
				break;
			}
		}
	}

	if (mySelectedElement == nullptr)
	{
		mySelectedElement = myPreviousSelectedElement;
	}

	if (mySelectedElement != myPreviousSelectedElement)
	{
		Essentials::globalAudioManager->PlaySFX(SoundID::eVineBoom);
		myPreviousSelectedElement->definition->generalProperties.size = mySelectedOriginalSize;
		mySelectedOriginalSize = mySelectedElement->definition->generalProperties.size;
	}
}

void UICanvas::InteractionControl()
{
	SelectableUIProperties* selectableProperties = nullptr;

	std::visit([&](auto& e) {

		using T = std::decay_t<decltype(e)>;
		if constexpr (std::is_same_v<T, UISlider>)
		{
			selectableProperties = &e.selectable;
		}
		else if constexpr (std::is_same_v<T, UIToggle>)
		{
			selectableProperties = &e.selectable;
		}
		else if constexpr (std::is_same_v<T, UIButton>)
		{
			selectableProperties = &e.selectable;
		}

	}, mySelectedElement->definition->uiElementProperties);

	if ((Essentials::globalInputManager->PressingConfirm() ||
		(Essentials::globalInputManager->IsKeyPressed(static_cast<int>(Keys::MOUSELBUTTON)
		&& CanvasObjectDefinition::PosInside(myLastMousePosition, mySelectedElement->definition->generalProperties, *myCanvas, 
			Tga::Vector2i(Essentials::globalEngine->GetRenderSize().x, Essentials::globalEngine->GetRenderSize().y), selectableProperties)))))
	{
		std::visit([&](auto& e) {

			using T = std::decay_t<decltype(e)>;
			if constexpr (std::is_same_v<T, UISlider>)
			{
				isFocused = !isFocused;
				//Essentials::myCursor->SetCursorVisible(false);
			}
			else if constexpr (std::is_same_v<T, UIToggle>)
			{
				e.isOn = !e.isOn;
			}
			}, mySelectedElement->definition->uiElementProperties);
	}
	else if (Essentials::globalInputManager->IsKeyReleased(static_cast<int>(Keys::MOUSELBUTTON)))
	{
		std::visit([&](auto& e) {

			using T = std::decay_t<decltype(e)>;
			if constexpr (std::is_same_v<T, UISlider>)
			{
				isFocused = false;
				//Essentials::myCursor->SetCursorVisible(!isFocused);
			}
			}, mySelectedElement->definition->uiElementProperties);
	}
}

void UICanvas::RenderAll()
{
	if (UICanvas::ourUICamera == nullptr)
	{
		return;
	}

	auto& engine = *Tga::Engine::GetInstance();
	auto& stack = engine.GetGraphicsEngine().GetGraphicsStateStack();

	Tga::Vector2ui res = Tga::Engine::GetInstance()->GetRenderSize();
	UICanvas::ourUICamera->SetPerspectiveProjection(90.f, Tga::Vector2f((float)res.x, (float)res.y), 0.1f, 1000.f);
	stack.SetCamera(*UICanvas::ourUICamera);

	stack.Push();
	stack.SetAllStatesToDefault();
	stack.SetBlendState(Tga::BlendState::AlphaBlend);
	stack.SetDepthStencilState(Tga::DepthStencilState::ReadOnlyLess);

	for (int i = 0; i < ourUICanvases.size(); i++)
	{
		ourUICanvases[i]->Render();
	}

	stack.Pop();
}

void UICanvas::SetIsHidden(bool someIsHidden)
{
	myIsHidden = someIsHidden;
}

bool UICanvas::GetIsHidden()
{
	return myIsHidden;
}

void UICanvas::SetSelectedElement(const std::string aID)
{
	if(mySelectedElement != nullptr)
		mySelectedElement->definition->generalProperties.size = mySelectedOriginalSize;
	mySelectedElement = GetElement(aID);
	if (mySelectedElement == nullptr)
	{
		printf("Failed to select UI element: %s\n", aID.c_str());
		return;
	}

	mySelectedOriginalSize = mySelectedElement->definition->generalProperties.size;
}

const std::string UICanvas::GetSelectedElementName()
{
	if (mySelectedElement == nullptr)
	{
		return {};
	}

	return mySelectedElement->definition->generalProperties.name;
}

RuntimeUIElement* UICanvas::GetSelectedElement()
{
	return mySelectedElement;
}

RuntimeUIElement* UICanvas::GetElement(const std::string aID)
{
	for (int i = 0; i < myElements.size(); i++)
	{
		if (myElements[i].definition->generalProperties.name == aID)
		{
			return &myElements[i];
		}
	}

	return nullptr;
}

UIButton* UICanvas::GetButton(const std::string aID)
{
	auto* element = GetElement(aID);
	if (!element)
		return nullptr;

	return std::get_if<UIButton>(&element->definition->uiElementProperties);
}

UIToggle* UICanvas::GetToggle(const std::string aID)
{
	auto* element = GetElement(aID);
	if (!element)
		return nullptr;

	return std::get_if<UIToggle>(&element->definition->uiElementProperties);
}

UISlider* UICanvas::GetSlider(const std::string aID)
{
	auto* element = GetElement(aID);
	if (!element)
		return nullptr;

	return std::get_if<UISlider>(&element->definition->uiElementProperties);
}

UIImage* UICanvas::GetImage(const std::string aID)
{
	auto* element = GetElement(aID);
	if (!element)
		return nullptr;

	return std::get_if<UIImage>(&element->definition->uiElementProperties);
}

UIText* UICanvas::GetText(const std::string aID)
{
	auto* element = GetElement(aID);
	if (!element)
		return nullptr;

	return std::get_if<UIText>(&element->definition->uiElementProperties);
}

UIElementGroup* UICanvas::GetElementGroup(const std::string aID)
{
	auto* element = GetElement(aID);
	if (!element)
		return nullptr;

	return std::get_if<UIElementGroup>(&element->definition->uiElementProperties);
}

bool UICanvas::GetElementPressed(std::string aID)
{
	RuntimeUIElement* element = GetElement(aID);
	if (element == nullptr || mySelectedElement == nullptr)
		return false;

	SelectableUIProperties* selectableProperties = nullptr;

	std::visit([&](auto& e) {

		using T = std::decay_t<decltype(e)>;
		if constexpr (std::is_same_v<T, UISlider>)
		{
			selectableProperties = &e.selectable;
		}
		else if constexpr (std::is_same_v<T, UIToggle>)
		{
			selectableProperties = &e.selectable;
		}
		else if constexpr (std::is_same_v<T, UIButton>)
		{
			selectableProperties = &e.selectable;
		}

	}, mySelectedElement->definition->uiElementProperties);

	if ((Essentials::globalInputManager->PressingConfirm()
		|| (Essentials::globalInputManager->IsKeyPressed(static_cast<int>(Keys::MOUSELBUTTON)
		&& CanvasObjectDefinition::PosInside(myLastMousePosition, mySelectedElement->definition->generalProperties, *myCanvas, 
			Tga::Vector2i(Essentials::globalEngine->GetRenderSize().x, Essentials::globalEngine->GetRenderSize().y), selectableProperties))))
		&& mySelectedElement == element)
	{
		Essentials::globalAudioManager->PlaySFX(SoundID::eVineBoom);
		return true;
	}

	return false;
}

bool UICanvas::GetToggleValue(std::string aID)
{
	RuntimeUIElement* toggle = GetElement(aID);
	if (toggle == nullptr || toggle->definition->elementType != UIElementType::Toggle)
		return false;

	return std::get_if<UIToggle>(&toggle->definition->uiElementProperties)->isOn;
}

float UICanvas::GetSliderValue(std::string aID)
{
	RuntimeUIElement* slider = GetElement(aID);
	if (slider == nullptr || slider->definition->elementType != UIElementType::Slider)
		return false;

	return std::get_if<UISlider>(&slider->definition->uiElementProperties)->currentValue;
}

void UICanvas::SetIsHidden(std::string aID, const bool aIsHidden)
{
	RuntimeUIElement* element = GetElement(aID);
	if (element == nullptr)
	{
		printf("Failed to set UI element hidden state: %s\n", aID.c_str());
		return;
	}

	element->definition->generalProperties.hide = aIsHidden;
}

bool UICanvas::GetIsHidden(std::string aID)
{
	RuntimeUIElement* element = GetElement(aID);
	if (element == nullptr)
	{
		return true;
	}

	return element->definition->generalProperties.hide;
}

void UICanvas::ResetIsFocused()
{
	isFocused = false;
}

CanvasObjectDefinition* UICanvas::GetCanvas()
{
	return myCanvas;
}

void UICanvas::Render()
{
	if (!myCanvas || myIsHidden)
		return;

	CanvasDrawParameters params = {
		.useIdShader = false,
		.showBounds = false,
		.resolution = Tga::Vector2i(Essentials::globalEngine->GetRenderSize().x, Essentials::globalEngine->GetRenderSize().y)
	};

	for (int i = 0; i < myRenderOrder.size(); i++)
	{
		CanvasObjectDefinition::DrawCanvasElement(
			*myCanvas,
			*myElements[myRenderOrder[i]].definition,
			params
		);
	}

	CanvasObjectDefinition::DrawQueued();
}

