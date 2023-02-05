#pragma once
struct KeyboardState {
	bool wPressed;
	bool aPressed;
	bool sPressed;
	bool dPressed;
	bool shiftPressed;
	bool ctrlPressed;
	KeyboardState();
	void KeyboardCB(unsigned char key, int mouse_x, int mouse_y);
	void KeysUp(unsigned char key, int x, int y);
	void SpecialKeyboardCB(int key, int mouse_x, int mouse_y);
	void SpecialKeysUp(int key, int x, int y);

};

