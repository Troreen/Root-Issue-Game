#include "Cursor.h"
#include "Essentials.h"
#include "Windows.h"

Cursor::Cursor()
{
}

Cursor::~Cursor()
{
}

void Cursor::SceneLoaded()
{
	myUICanvas.Init(Tga::StringRegistry::RegisterOrGetString("CursorCanvas"),  *Essentials::globalCanvasManager);
	myCursor = myUICanvas.GetImage("CursorImage");
	myCursorElement = myUICanvas.GetElement("CursorImage")->definition;
	ShowCursor(false);
}

void Cursor::UpdatePosition()
{
	if (myCursorElement->generalProperties.hide)
		return;

	Tga::Vector2f res = Essentials::GetResolution();
	std::cout << "RES: " << res << std::endl;

    Tga::Vector2f mousePos;
    mousePos = Essentials::globalInputManager->GetMousePosition() - (res/2.f);
	Tga::Vector2f screenPos;
	screenPos.x = mousePos.x + res.x * 0.5f;
	screenPos.y = res.y * 0.5f - mousePos.y;
	myCursorElement->generalProperties.pos = Tga::CanvasObjectDefinition::ScreenPosToUIPos(screenPos, myCursorElement->generalProperties, *myUICanvas.GetCanvas(), Essentials::GetResolutionInt());
}

bool Cursor::GetCursorVisible()
{
	return !myCursorElement->generalProperties.hide;
}

void Cursor::SetCursorVisible(bool visible)
{
	myCursorElement->generalProperties.hide = !visible;
}
