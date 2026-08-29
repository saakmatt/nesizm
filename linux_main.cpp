/*******************************************************************
  Linux replacement for winsim's winmain.cpp.
  Opens a GLUT window, maps keys into g_keyState, and calls simmain().
 *******************************************************************/

#include <stdio.h>
#include <string.h>
#include <GL/freeglut.h>

#include "linux_compat.h"
#include "prizmsim.h"

// referenced by prizmsim.h / sim_misc.cpp
HDC    renderContext = 0;
GLuint screenTexture = 0;
HWND   GWnd = 0;

// virtual-key state read by GetAsyncKeyState() in linux_compat.h
unsigned char g_keyState[256] = { 0 };

extern void DisplayGLUTScreen();
extern void _CallQuit();

// map a GLUT key to the VK_* code the winsim key table expects
static int VKFromAscii(unsigned char k) {
	if (k >= 'a' && k <= 'z') return k - 'a' + 'A';   // table uses uppercase
	if (k >= 'A' && k <= 'Z') return k;
	if (k >= '0' && k <= '9') return k;
	switch (k) {
		case 27:  return VK_ESCAPE;
		case 13:  return VK_RETURN;
		case 9:   return VK_TAB;
		case 8:   return VK_BACK;
		case ' ': return VK_SPACE;
		case ';': return VK_OEM_1;
		case '-': return VK_OEM_MINUS;
		case '=': return VK_OEM_PLUS;
		case '.': return VK_OEM_PERIOD;
		case '*': return VK_MULTIPLY;
		case '/': return VK_DIVIDE;
	}
	return -1;
}

static int VKFromSpecial(int k) {
	switch (k) {
		case GLUT_KEY_LEFT:  return VK_LEFT;
		case GLUT_KEY_RIGHT: return VK_RIGHT;
		case GLUT_KEY_UP:    return VK_UP;
		case GLUT_KEY_DOWN:  return VK_DOWN;
		case GLUT_KEY_HOME:  return VK_HOME;
		case GLUT_KEY_END:   return VK_END;
		case GLUT_KEY_F1:    return VK_F1;
		case GLUT_KEY_F2:    return VK_F2;
		case GLUT_KEY_F3:    return VK_F3;
		case GLUT_KEY_F4:    return VK_F4;
		case GLUT_KEY_F5:    return VK_F5;
		case GLUT_KEY_F6:    return VK_F6;
	}
	return -1;
}

static void SetKey(int vk, unsigned char down) {
	if (vk >= 0 && vk < 256) g_keyState[vk] = down;
}

static void OnKeyDown(unsigned char k, int, int) {
	int vk = VKFromAscii(k);
	fprintf(stderr, "keydown %d -> vk %d\n", k, vk);
	SetKey(vk, 1);
}
static void OnKeyUp(unsigned char k, int, int)      { SetKey(VKFromAscii(k), 0); }
static void OnSpecialDown(int k, int, int)          { SetKey(VKFromSpecial(k), 1); }
static void OnSpecialUp(int k, int, int)            { SetKey(VKFromSpecial(k), 0); }

static void StartGlut(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(384 * 2, 216 * 2);
	glutCreateWindow("Prizm Sim");

	glutDisplayFunc(DisplayGLUTScreen);
	glutKeyboardFunc(OnKeyDown);
	glutKeyboardUpFunc(OnKeyUp);
	glutSpecialFunc(OnSpecialDown);
	glutSpecialUpFunc(OnSpecialUp);

  //glutIgnoreKeyRepeat(1);

	// keep the emulator loop in control; don't let GLUT exit the process
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

	// GetFocus() == GWnd must be true for keys to register, so use any non-null value
	GWnd = (HWND)1;

	glGenTextures(1, &screenTexture);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 384, 216, 0,
	             GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
}

int main(int argc, char** argv) {
	printf("Starting Linux Simulator...\n");

	StartGlut(argc, argv);

	int ret = simmain();

	_CallQuit();

	printf("Shutting down Linux Simulator...\n");
	return ret;
}
