#pragma once
#include "raylib.h"
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
		std::cout << deletors.size() << '\n';

		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)();
		}

		deletors.clear();
	}
};

class Scene {
protected:
	Camera3D camera{};
	int cameraMode{ CAMERA_FREE };
	float lookSpeed{ 0.005f };
	float moveSpeed{ 30.f };
	DeletionQueue mainDeletionQueue{};

public:
    Scene() {
        camera.position = Vector3{ -10.0f, 0.0f, 10.0f };
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        camera.target = Vector3{ 0.f, 0.f, 0.f };
        camera.up = Vector3{ 0.0f, 1.0f, 0.f };

        cameraMode = CAMERA_FREE;
    }

    virtual ~Scene() {
		mainDeletionQueue.flush();
	}

    virtual void Update(float deltaTime) {
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

		// camera
		static float yaw = 0.0f;
		static float pitch = 0.0f;
		if (cursorHidden)
		{
			float dt = deltaTime;

			// 1. Mouse rotation (Look around)
			Vector2 mouseDelta = GetMouseDelta();
			yaw += mouseDelta.x * lookSpeed;
			pitch -= mouseDelta.y * lookSpeed;

			// Clamp pitch to ~85 degrees up/down to prevent vertical flipping (gimbal lock)
			const float maxPitch = 1.48f;
			if (pitch > maxPitch) pitch = maxPitch;
			if (pitch < -maxPitch) pitch = -maxPitch;

			// Calculate the camera's forward vector from spherical angles
			Vector3 forward = { 0 };
			forward.x = sinf(yaw) * cosf(pitch);
			forward.y = sinf(pitch);
			forward.z = -cosf(yaw) * cosf(pitch);
			forward = Vector3Normalize(forward);

			// Calculate the camera's right vector (perpendicular to look direction and world up)
			Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
			Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, worldUp));

			// 2. Keyboard movement collection
			Vector3 moveDirection = { 0 };

			// W/S - Move Forward/Backward (3D space)
			if (IsKeyDown(KEY_W)) moveDirection = Vector3Add(moveDirection, forward);
			if (IsKeyDown(KEY_S)) moveDirection = Vector3Subtract(moveDirection, forward);

			// A/D - Strafe Left/Right
			if (IsKeyDown(KEY_D)) moveDirection = Vector3Add(moveDirection, right);
			if (IsKeyDown(KEY_A)) moveDirection = Vector3Subtract(moveDirection, right);

			// SPACE/L-SHIFT - Move Up/Down (Relative to the world axis)
			if (IsKeyDown(KEY_SPACE)) moveDirection = Vector3Add(moveDirection, worldUp);
			if (IsKeyDown(KEY_LEFT_SHIFT)) moveDirection = Vector3Subtract(moveDirection, worldUp);

			// Normalize movement direction so moving diagonally isn't faster
			if (Vector3LengthSqr(moveDirection) > 0.0f) {
				moveDirection = Vector3Normalize(moveDirection);

				// Apply speed and delta time to position
				Vector3 movement = Vector3Scale(moveDirection, moveSpeed * dt);
				camera.position = Vector3Add(camera.position, movement);
			}

			// Keep the camera's target directly in front of the camera position
			camera.target = Vector3Add(camera.position, forward);
		}
    }

	virtual void Draw() {}

	virtual void Draw3D() {};
    virtual void Draw2D() {}
    virtual void DrawUI() {}
};