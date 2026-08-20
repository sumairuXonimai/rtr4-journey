#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlimgui.h"
#include "imgui.h"

#include "helpers/Globals.h"

#include <deque>
#include <functional>
#include <iostream>

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void flush()
	{
		//std::cout << deletors.size() << '\n';

		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)();
		}

		deletors.clear();
	}
};

class Scene {
protected:
	DeletionQueue mainDeletionQueue{};

public:
	Scene() = default;
	virtual ~Scene() {
		mainDeletionQueue.flush();
	}

	bool sceneExited{ false };
	virtual void Update(float deltaTime)
	{
		if (IsKeyPressed(KEY_ESCAPE))
		{
			sceneExited = true;
		}
	}
	virtual void Draw() = 0;
	virtual void DrawUI() = 0;
};

class Scene3D : public Scene {
protected:
	Camera3D camera{};
	int cameraMode{ CAMERA_FREE };
	float lookSpeed{ 0.005f };
	float moveSpeed{ 30.f };

public:
	Scene3D() {
		camera.position = Vector3{ -10.0f, 0.0f, 10.0f };
		camera.fovy = 45.0f;
		camera.projection = CAMERA_PERSPECTIVE;
		camera.target = Vector3{ 0.f, 0.f, 0.f };
		camera.up = Vector3{ 0.0f, 1.0f, 0.f };

		cameraMode = CAMERA_FREE;
	}

	~Scene3D() override = default;

	void Update(float deltaTime) override {
		Scene::Update(deltaTime);

		// input/cursor
		static bool cursorHidden{ true };
		if (IsKeyPressed(KEY_Q))
			cursorHidden = !cursorHidden;

		if (cursorHidden)
			HideCursor();
		else
			ShowCursor();

		if (!IsCursorOnScreen())
		{
			if (IsCursorHidden())
				SetMousePosition(g_windowWidth / 2, g_windowHeight / 2);
			else
			{
				auto mousePos{ GetMousePosition() };
				SetMousePosition(
					Clamp(mousePos.x, 0, g_windowWidth),
					Clamp(mousePos.y, 0, g_windowHeight)
				);
			}
		}

		// camera rotation and movement
		static float yaw = 0.0f;
		static float pitch = 0.0f;
		if (cursorHidden)
		{
			float dt = deltaTime;

			// Mouse rotation (Look around)
			Vector2 mouseDelta = GetMouseDelta();
			yaw += mouseDelta.x * lookSpeed;
			pitch -= mouseDelta.y * lookSpeed;

			// Clamp pitch to ~85 degrees up/down to prevent vertical flipping
			const float maxPitch = 1.48f;
			if (pitch > maxPitch) pitch = maxPitch;
			if (pitch < -maxPitch) pitch = -maxPitch;

			// Calculate forward vector
			Vector3 forward = { 0 };
			forward.x = sinf(yaw) * cosf(pitch);
			forward.y = sinf(pitch);
			forward.z = -cosf(yaw) * cosf(pitch);
			forward = Vector3Normalize(forward);

			// Calculate right vector
			Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
			Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, worldUp));

			// Keyboard movement
			Vector3 moveDirection = { 0 };

			if (IsKeyDown(KEY_W)) moveDirection = Vector3Add(moveDirection, forward);
			if (IsKeyDown(KEY_S)) moveDirection = Vector3Subtract(moveDirection, forward);
			if (IsKeyDown(KEY_D)) moveDirection = Vector3Add(moveDirection, right);
			if (IsKeyDown(KEY_A)) moveDirection = Vector3Subtract(moveDirection, right);
			if (IsKeyDown(KEY_SPACE)) moveDirection = Vector3Add(moveDirection, worldUp);
			if (IsKeyDown(KEY_LEFT_SHIFT)) moveDirection = Vector3Subtract(moveDirection, worldUp);

			if (Vector3LengthSqr(moveDirection) > 0.0f) {
				moveDirection = Vector3Normalize(moveDirection);
				Vector3 movement = Vector3Scale(moveDirection, moveSpeed * dt);
				camera.position = Vector3Add(camera.position, movement);
			}

			camera.target = Vector3Add(camera.position, forward);
		}
	}

	void Draw() override {}

	void DrawUI() override {}
};

class Scene2D : public Scene {
protected:
	Camera2D camera{};
	float moveSpeed{ 300.f };
	float zoomSpeed{ 0.1f };

public:
	Scene2D() {
		camera.target = Vector2{ 0.0f, 0.0f };
		camera.offset = Vector2{ g_windowWidth / 2.0f, g_windowHeight / 2.0f };
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;
	}

	~Scene2D() override = default;

	void Update(float deltaTime) override {
		Scene::Update(deltaTime);

		if (IsKeyDown(KEY_W)) camera.target.y -= moveSpeed * deltaTime;
		if (IsKeyDown(KEY_S)) camera.target.y += moveSpeed * deltaTime;
		if (IsKeyDown(KEY_A)) camera.target.x -= moveSpeed * deltaTime;
		if (IsKeyDown(KEY_D)) camera.target.x += moveSpeed * deltaTime;

		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f) {
			camera.zoom += wheel * zoomSpeed;
			if (camera.zoom < 0.1f) camera.zoom = 0.1f;
		}
	}

	void Draw() override {}

	void DrawUI() override {}
};