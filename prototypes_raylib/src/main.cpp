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

#include "5-4_transparency_alpha_compositing/5-4.h"

#include "helpers/Globals.h"

int main()
{
	g_windowWidth = 1080;
	g_windowHeight = 720;
	InitWindow(g_windowWidth, g_windowHeight, "bahaha");

	rlImGuiSetup(true);
	SetTargetFPS(144);

	std::unique_ptr<Scene> currentScene{ std::make_unique<TransparencyScene>() };

	while (!WindowShouldClose())
	{
		currentScene->Update(GetFrameTime());

		currentScene->Draw();
	}

	rlImGuiShutdown();
	CloseWindow();
}