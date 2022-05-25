export import Core.KeyCodes;
export import Core.MouseCodes;
export import <tuple>;

export module Core.Input;

struct GLFWwindow;
namespace NutCracker {
	export
	namespace Input
	{
		bool IsKeyPressed (GLFWwindow* window, KeyCode keycode);
		bool IsMouseButtonPressed (GLFWwindow* window, MouseCode button);

		std::pair<float, float> GetMousePosn (GLFWwindow* window);
	};
}

import <GLFW/glfw3.h>;
bool NutCracker::Input::IsKeyPressed (GLFWwindow* window, const KeyCode key)
{
	auto state = glfwGetKey(window, static_cast<int32_t>(key));
	return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool NutCracker::Input::IsMouseButtonPressed (GLFWwindow* window, const MouseCode button)
{
	auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
	return state == GLFW_PRESS;
}

std::pair<float, float> NutCracker::Input::GetMousePosn (GLFWwindow* window)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	return { (float)xpos, (float)ypos };
}