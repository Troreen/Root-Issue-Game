#pragma once
#include "UICanvas.h"

class Cursor
{
	public: 
		Cursor();
		~Cursor();
		
		void SceneLoaded();
		void UpdatePosition();
		
		bool GetCursorVisible();
		void SetCursorVisible(bool visible);

	private:
		UICanvas myUICanvas;
		UIImage* myCursor;
		UIElement* myCursorElement;
};
