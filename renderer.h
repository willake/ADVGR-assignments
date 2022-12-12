#pragma once

namespace Tmpl8
{

class Renderer : public TheApp
{
public:
	// game flow methods
	void Init();
	float3 Trace( Ray& ray );
	void Tick( float deltaTime );
	void Shutdown() { /* implement if you want to do something on exit */ }
	// input handling
	void MouseUp(int button) { if (button == GLFW_MOUSE_BUTTON_RIGHT) isMouseButtonRightDown = false; }
	void MouseDown(int button) { if (button == GLFW_MOUSE_BUTTON_RIGHT) isMouseButtonRightDown = true; }
	void MouseMove(int x, int y) {
		mouseOffset.x = x - lastMousePos.x;
		mouseOffset.y = lastMousePos.y - y; // reversed since y-coordinates range from bottom to top
		lastMousePos.x = x;
		lastMousePos.y = y;

		mouseOffset.x *= mouseSensitivity;
		mouseOffset.y *= mouseSensitivity;
	}
	void MouseWheel(float y) {  }
	void KeyUp(int key)
	{
		if (key == GLFW_KEY_W || key == GLFW_KEY_S) verticalInput = 0;

		if (key == GLFW_KEY_A || key == GLFW_KEY_D) horizontalInput = 0;
	}
	void KeyDown(int key)
	{
		if (key == GLFW_KEY_W) verticalInput = 1;
		else if (key == GLFW_KEY_S) verticalInput = -1;

		if (key == GLFW_KEY_A) horizontalInput = -1;
		else if (key == GLFW_KEY_D) horizontalInput = 1;
	}
	// data members
	bool isMouseButtonRightDown;
	int verticalInput;
	int horizontalInput;
	int mouseWheel;
	float mouseSensitivity = 0.1f;
	int2 lastMousePos = int2(SCRWIDTH / 2, SCRHEIGHT / 2);
	int2 mouseOffset = int2(0, 0);
	float4* accumulator;
	Scene scene;
	Camera camera;
};

} // namespace Tmpl8