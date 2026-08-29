#include <stdio.h>
#include <GL/freeglut.h>

static void onKey(unsigned char k, int x, int y) {
	printf("key %d\n", k); fflush(stdout);
}
static void onSpecial(int k, int x, int y) {
	printf("special %d\n", k); fflush(stdout);
}
static void onDisplay(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();
}
int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(400, 300);
	glutCreateWindow("keytest");
	glutDisplayFunc(onDisplay);
	glutKeyboardFunc(onKey);
	glutSpecialFunc(onSpecial);
	for (;;) { glutPostRedisplay(); glutMainLoopEvent(); }
	return 0;
}
