#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rlimgui.h"
#include "imgui.h"

#include "helpers/Random.h"

#include <iostream>
#include <vector>
#include <random>
#include <array>

#include "5-4_transparency_alpha_compositing/transparency_scene.h"
#include "6_textures/rain_scene.h"

#include "helpers/Globals.h"

int main()
{
	g_windowWidth = 1080;
	g_windowHeight = 720;
	InitWindow(g_windowWidth, g_windowHeight, "bahaha");

	rlImGuiSetup(true);
	SetTargetFPS(144);

	std::unique_ptr<Scene> currentScene{ nullptr };

	while (!WindowShouldClose())
	{
		if (currentScene != nullptr && currentScene->sceneExited)
			currentScene.reset();

		if (currentScene != nullptr)
		{
			SetExitKey(KEY_NULL);

			currentScene->Update(GetFrameTime());
			currentScene->Draw();
		}
		else
		{
			SetExitKey(KEY_ESCAPE);

			BeginDrawing();

			ClearBackground(WHITE);

			rlImGuiBegin();

			if (ImGui::Begin("Navigations"))
			{
				if (ImGui::Button("5-4 Transparency Scene")) currentScene = std::make_unique<TransparencyScene>();
				if (ImGui::Button("6 Rain Scene")) currentScene = std::make_unique<RainScene>();

				ImGui::End();
			}

			rlImGuiEnd();

			EndDrawing();
		}
	}

	rlImGuiShutdown();
	CloseWindow();

	return 0;
}