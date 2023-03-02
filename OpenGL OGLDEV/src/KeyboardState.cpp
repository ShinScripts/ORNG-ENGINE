#include <stdio.h>
#include <ctype.h>
#include "KeyboardState.h"

KeyboardState::KeyboardState() {
	wPressed = false;
	aPressed = false;
	ePressed = false;
	qPressed = false;
	sPressed = false;
	dPressed = false;
	shiftPressed = false;
	ctrlPressed = false;
}

void KeyboardState::KeyboardCB(unsigned char key, int mouse_x, int mouse_y)
{
	switch (static_cast<char>(tolower(key))) {
	case 27: //ESC
		mouse_locked = !mouse_locked;
		break;
	case 'w':
		wPressed = true;
		break;

	case 's':
		sPressed = true;
		break;

	case 'a':
		aPressed = true;
		break;

	case 'd':
		dPressed = true;
		break;

	case 'q':
		qPressed = true;
		break;

	case 'e':
		ePressed = true;
		break;
	case 'g':
		g_pressed = true;
		break;
	}

}

void KeyboardState::SpecialKeysUp(int key, int x, int y) {
	/*switch (key) {
	case GLUT_KEY_SHIFT_L:
		shiftPressed = false;
		break;

	case GLUT_KEY_CTRL_L:
		ctrlPressed = false;
		break;
	}*/
}

void KeyboardState::KeysUp(unsigned char key, int x, int y) {

	switch (static_cast<char>(tolower(key))) {
	case 'w':
		wPressed = false;
		break;

	case 's':
		sPressed = false;
		break;

	case 'a':
		aPressed = false;
		break;

	case 'd':
		dPressed = false;
		break;

	case 'q':
		qPressed = false;
		break;

	case 'e':
		ePressed = false;
		break;
	case 'g':
		g_pressed = false;
		break;
	}
}

void KeyboardState::SpecialKeyboardCB(int key, int mouse_x, int mouse_y)
{
	/*switch (key) {

		case GLUT_KEY_SHIFT_L:
			shiftPressed = true;
			break;

		case GLUT_KEY_CTRL_L:
			ctrlPressed = true;
			break;
		}*/
}