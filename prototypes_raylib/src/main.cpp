#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rlimgui.h"
#include "imgui.h"

#include <iostream>
#include <vector>
#include <random>
#include <array>

// Load custom render texture with depth texture attached
static RenderTexture2D LoadRenderTextureDepthTex(int width, int height);

// Unload render texture from GPU memory (VRAM)
static void UnloadRenderTextureDepthTex(RenderTexture2D target);

struct Object
{
	Vector3 position;
	Color color;
};

float getRandomFloat(float min, float max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(min, max);
	return dis(gen);
}
const int windowWidth{ 1080 };
const int windowHeight{ 720 };

static float moveSpeed{ 30.0f };
static float lookSpeed{ 0.005f };
static int objectCount{ 3000 };
static int objectSize{ 10 };

static int selectedShader{ 1 };

void renderUI()
{
	DrawFPS(0, 0);

	rlImGuiBegin();

	if (ImGui::Begin("Params"))
	{
		ImGui::SliderFloat("cam speed", &moveSpeed, 1.f, 100.f);
		ImGui::SliderFloat("cam sens", &lookSpeed, 0.001f, 0.05f);
		ImGui::SliderInt("object count", &objectCount, 100, 1000);
		ImGui::SliderInt("object size", &objectSize, 1, 100);

		ImGui::End();
	}

	if (ImGui::Begin("Mode"))
	{
		const char* labels[]{ "dithering transparency", "depth peeling" };
		for (int n = 0; n < 2; n++)
		{
			if (ImGui::Selectable(labels[n], selectedShader == n))
				selectedShader = n;
		}

		ImGui::End();
	}

	rlImGuiEnd();
}

void ditheringDraw(Camera& camera, Shader& shader, const std::vector<Object>& opaqueObjects, const std::vector<Object>& transparentObjects)
{
	BeginDrawing();

	ClearBackground(WHITE);

	DrawGrid(1000, 1.f);

	BeginMode3D(camera);

	int posLoc{ GetShaderLocation(shader, "u_ObjectPos") };

	rlDisableBackfaceCulling();
	for (const auto& object : opaqueObjects)
		DrawCube(object.position, objectSize, objectSize, objectSize, object.color);

	BeginShaderMode(shader);

	rlDisableDepthMask();
	for (const auto& object : transparentObjects)
	{
		SetShaderValue(shader, posLoc, &object.position, SHADER_UNIFORM_VEC3);
		DrawCube(object.position, objectSize, objectSize, objectSize, object.color);
	}
	rlEnableDepthMask();
	EndShaderMode();

	rlEnableBackfaceCulling();

	EndMode3D();
	
	renderUI();
	
	EndDrawing();
}

void depthPeelDraw(Camera& camera, Shader& shader, std::vector<Object>& opaqueObjects, std::vector<Object>& transparentObjects)
{
	const int MAX_LOOPS{ 4 };

	static int opaqueDepthLoc{ GetShaderLocation(shader, "u_OpaqueDepth") };
	static int prevDepthLoc{ GetShaderLocation(shader, "u_PrevDepth") };
	static int screenSizeLoc{ GetShaderLocation(shader, "u_ScreenSize") };
	static int peeledLoc{ GetShaderLocation(shader, "u_Peeled") };

	static RenderTexture2D opaqueDepthTarget{ LoadRenderTextureDepthTex(windowWidth, windowHeight) };
	static RenderTexture2D peelDepthTargets[2] = {
		LoadRenderTextureDepthTex(windowWidth, windowHeight),
		LoadRenderTextureDepthTex(windowWidth, windowHeight)
	};
	static RenderTexture2D resultTarget{ LoadRenderTextureDepthTex(windowWidth, windowHeight) };

	Vector2 screenSize{ windowWidth, windowHeight };
	SetShaderValue(shader, screenSizeLoc, &screenSize, SHADER_UNIFORM_VEC2);
	static int opaqueDepthTexID{ 1 };
	static int prevDepthTexID{ 2 };

	BeginTextureMode(opaqueDepthTarget);
		ClearBackground(SKYBLUE);
		BeginMode3D(camera);

		for (const auto& object : opaqueObjects)
			DrawCube(object.position, objectSize, objectSize, objectSize, object.color);

		EndMode3D();
	EndTextureMode();

	BeginTextureMode(resultTarget);
	ClearBackground(BLANK);
	EndTextureMode();

	int prevIdx{ 0 };
	int currIdx{ 1 };

	for (int i = 0; i < MAX_LOOPS; i++)
	{
		int val{ (i == 0) ? 0 : 1 };
		SetShaderValue(shader, peeledLoc, &val, SHADER_UNIFORM_INT);

		rlEnableTexture(peelDepthTargets[prevIdx].depth.id);
		
		// draw to currIdx
		BeginTextureMode(peelDepthTargets[currIdx]);
			rlClearScreenBuffers();

			BeginMode3D(camera);
			BeginShaderMode(shader);

			SetShaderValueTexture(shader, opaqueDepthLoc, opaqueDepthTarget.depth);

			if (i > 0)
			{
				SetShaderValueTexture(shader, prevDepthLoc, peelDepthTargets[prevIdx].depth);
			}

			//rlDisableBackfaceCulling();
			for (const auto& object : transparentObjects)
			{
				DrawCube(object.position, objectSize, objectSize, objectSize, object.color);
			}
			//rlEnableBackfaceCulling();

			EndShaderMode();
			EndMode3D();

		EndTextureMode();

		// draw to accumulated target
		BeginTextureMode(resultTarget);
			
			// front to back
			rlSetBlendFactors(RL_ONE_MINUS_DST_ALPHA, RL_ONE, RL_FUNC_ADD);
			rlSetBlendMode(RL_BLEND_CUSTOM);

			DrawTextureRec(peelDepthTargets[currIdx].texture,
				Rectangle{ 0, 0, (float)windowWidth, (float)-windowHeight },
				Vector2{ 0, 0 }, WHITE);

			rlSetBlendMode(RL_BLEND_ALPHA);

		EndTextureMode();

		std::swap(prevIdx, currIdx);
	}

	BeginDrawing();

	ClearBackground(WHITE);

	DrawGrid(1000, 1.f);

	// back to front
	rlSetBlendFactors(RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD);
	rlSetBlendMode(RL_BLEND_CUSTOM);

	DrawTextureRec(opaqueDepthTarget.texture,
		Rectangle{ 0, 0, (float)windowWidth, (float)-windowHeight },
		Vector2{ 0, 0 }, WHITE);

	DrawTextureRec(resultTarget.texture,
		Rectangle{
		0, 0, (float)windowWidth, (float)-windowHeight
	},
		Vector2{
		0, 0
	}, WHITE);
	
	rlSetBlendMode(RL_BLEND_ALPHA);

	renderUI();

	EndDrawing();
}

int main()
{
	InitWindow(windowWidth, windowHeight, "bahaha");

	rlImGuiSetup(true);

	SetTargetFPS(144);

	Camera3D camera{};
	camera.position = Vector3{ -10.0f, 0.0f, 10.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;
	camera.target = Vector3{ 0.f, 0.f, 0.f };
	camera.up = Vector3{ 0.0f, 1.0f, 0.f };

	// position, color
	std::vector<Object> opaqueObjects;
	std::vector<Object> transparentObjects;

	for (int i = 1; i <= objectCount / 4 * 2; i++)
	{
		Vector3 position{ getRandomFloat(-100, 100), getRandomFloat(-100, 100), getRandomFloat(-100, 100) };
		Color color{ GetRandomValue(0, 255), GetRandomValue(0, 255), GetRandomValue(0, 255), 255 };
		opaqueObjects.push_back(Object{ position, color });
	}

	for (int i = 1; i <= objectCount / 4 * 2; i++)
	{
		Vector3 position{ getRandomFloat(-100, 100), getRandomFloat(-100, 100), getRandomFloat(-100, 100) };
		Color color{ GetRandomValue(0, 255), GetRandomValue(0, 255), GetRandomValue(0, 255), GetRandomValue(100, 200) };
		transparentObjects.push_back(Object{ position, color });
	}

	Shader ditheringShader{ LoadShader(0, "shaders/dither.frag") };
	Shader depthPeelShader{ LoadShader(0, "shaders/depth_peel.frag") };

	std::array shaders{ ditheringShader, depthPeelShader };

	while (!WindowShouldClose())
	{
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
				SetMousePosition(windowWidth / 2, windowHeight / 2);
			else
			{
				auto mousePos{ GetMousePosition() };
				SetMousePosition(
					Clamp(mousePos.x, 0, windowWidth),
					Clamp(mousePos.y, 0, windowHeight)
				);
			}
		}

		// camera
		static float yaw = 0.0f;
		static float pitch = 0.0f;
		if (cursorHidden)
		{
			float dt = GetFrameTime();

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

		Shader& shader{ shaders[selectedShader] };
		switch (selectedShader)
		{
		case 0:
			ditheringDraw(camera, shader, opaqueObjects, transparentObjects);
			break;
		case 1:
		default:
			depthPeelDraw(camera, shader, opaqueObjects, transparentObjects);
			break;
		}
	}

	for (auto& shader: shaders)
		UnloadShader(shader);
	rlImGuiShutdown();
	CloseWindow();
}

static RenderTexture2D LoadRenderTextureDepthTex(int width, int height)
{
	RenderTexture2D target = { 0 };

	target.id = rlLoadFramebuffer(); // Load an empty framebuffer

	if (target.id > 0)
	{
		rlEnableFramebuffer(target.id);

		// Create color texture (default to RGBA)
		target.texture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
		target.texture.width = width;
		target.texture.height = height;
		target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
		target.texture.mipmaps = 1;

		// Create depth texture buffer (instead of raylib default renderbuffer)
		target.depth.id = rlLoadTextureDepth(width, height, false);
		target.depth.width = width;
		target.depth.height = height;
		target.depth.format = 19;       // DEPTH_COMPONENT_24BIT: Not defined in raylib
		target.depth.mipmaps = 1;

		// Attach color texture and depth texture to FBO
		rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
		rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

		// Check if fbo is complete with attachments (valid)
		if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

		rlDisableFramebuffer();
	}
	else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

	return target;
}

// Unload render texture from GPU memory (VRAM)
void UnloadRenderTextureDepthTex(RenderTexture2D target)
{
	if (target.id > 0)
	{
		// Color texture attached to FBO is deleted
		rlUnloadTexture(target.texture.id);
		rlUnloadTexture(target.depth.id);

		// NOTE: Depth texture is automatically
		// queried and deleted before deleting framebuffer
		rlUnloadFramebuffer(target.id);
	}
}