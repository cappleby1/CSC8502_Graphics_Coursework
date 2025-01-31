#include "../nclgl/window.h"
#include "Renderer.h"

int main() {
	Window w("20094015 - Coursework", 1280, 720, true);
	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w);
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	renderer.InitialiseCamera();

	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {

		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		renderer.RenderScene();
		renderer.SwapBuffers();


		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) {
			renderer.autumn = false;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_2)) {
			renderer.autumn = true;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_3)) {
			renderer.freeCam = true;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_4)) {
			renderer.freeCam = false;
		}
	}

	return 0;
}
