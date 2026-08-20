#pragma once
#include "../Scene.h"
#include "../helpers/Random.h"

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

class TransparencyScene : public Scene3D
{
public:
	TransparencyScene() : Scene3D() {
		ditheringShader = LoadShader(0, "shaders/dither.frag");
		depthPeelShader = LoadShader(0, "shaders/depth_peel.frag");

		shaders = { ditheringShader, depthPeelShader };

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
	}

	void Draw() override {
		shader = &shaders[selectedShader];
		switch (selectedShader)
		{
		case 0:
			ditheringDraw();
			break;
		case 1:
		default:
			depthPeelDraw();
			break;
		}
	}
	void DrawUI() override {
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

	~TransparencyScene() override {
		Scene::~Scene();

		opaqueObjects.clear();
		transparentObjects.clear();

		for (auto& shader : shaders)
			UnloadShader(shader);
	}

private:
	std::vector<Object> opaqueObjects;
	std::vector<Object> transparentObjects;

	Shader ditheringShader{};
	Shader depthPeelShader{};
	bool depthPeelDrawCalled{ false };
	std::vector<Shader> shaders{};
	int selectedShader{0};
	Shader* shader{ &ditheringShader };

	int objectCount{ 3000 };
	int objectSize{ 10 };

	void ditheringDraw()
	{
		BeginDrawing();

		ClearBackground(WHITE);

		DrawGrid(1000, 1.f);

		BeginMode3D(camera);

		int posLoc{ GetShaderLocation(*shader, "u_ObjectPos") };

		rlDisableBackfaceCulling();
		for (const auto& object : opaqueObjects)
			DrawCube(object.position, objectSize, objectSize, objectSize, object.color);

		BeginShaderMode(*shader);

		rlDisableDepthMask();
		for (const auto& object : transparentObjects)
		{
			SetShaderValue(*shader, posLoc, &object.position, SHADER_UNIFORM_VEC3);
			DrawCube(object.position, objectSize, objectSize, objectSize, object.color);
		}
		rlEnableDepthMask();
		EndShaderMode();

		rlEnableBackfaceCulling();

		EndMode3D();

		DrawUI();

		EndDrawing();
	}
	void depthPeelDraw()
	{
		const int MAX_LOOPS{ 4 };

		int opaqueDepthLoc{ GetShaderLocation(*shader, "u_OpaqueDepth") };
		int prevDepthLoc{ GetShaderLocation(*shader, "u_PrevDepth") };
		int screenSizeLoc{ GetShaderLocation(*shader, "u_ScreenSize") };
		int peeledLoc{ GetShaderLocation(*shader, "u_Peeled") };

		static RenderTexture2D opaqueDepthTarget{ LoadRenderTextureDepthTex(g_windowWidth, g_windowHeight) };
		static RenderTexture2D peelDepthTargets[2] = {
			LoadRenderTextureDepthTex(g_windowWidth, g_windowHeight),
			LoadRenderTextureDepthTex(g_windowWidth, g_windowHeight)
		};
		static RenderTexture2D resultTarget{ LoadRenderTextureDepthTex(g_windowWidth, g_windowHeight) };

		if (!depthPeelDrawCalled)
		{
			mainDeletionQueue.push_function([&]() {
				UnloadRenderTextureDepthTex(opaqueDepthTarget);
				UnloadRenderTextureDepthTex(peelDepthTargets[0]);
				UnloadRenderTextureDepthTex(peelDepthTargets[1]);
				UnloadRenderTextureDepthTex(resultTarget);
			});
			depthPeelDrawCalled = true;
		}

		Vector2 screenSize{ g_windowWidth, g_windowHeight };
		SetShaderValue(*shader, screenSizeLoc, &screenSize, SHADER_UNIFORM_VEC2);
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
			SetShaderValue(*shader, peeledLoc, &val, SHADER_UNIFORM_INT);

			rlEnableTexture(peelDepthTargets[prevIdx].depth.id);

			// draw to currIdx
			BeginTextureMode(peelDepthTargets[currIdx]);
			rlClearScreenBuffers();

			BeginMode3D(camera);
			BeginShaderMode(*shader);

			SetShaderValueTexture(*shader, opaqueDepthLoc, opaqueDepthTarget.depth);

			if (i > 0)
			{
				SetShaderValueTexture(*shader, prevDepthLoc, peelDepthTargets[prevIdx].depth);
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
				Rectangle{ 0, 0, (float)g_windowWidth, (float)-g_windowHeight },
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
			Rectangle{ 0, 0, (float)g_windowWidth, (float)-g_windowHeight },
			Vector2{ 0, 0 }, WHITE);

		DrawTextureRec(resultTarget.texture,
			Rectangle{
			0, 0, (float)g_windowWidth, (float)-g_windowHeight
			},
			Vector2{
			0, 0
			}, WHITE);

		rlSetBlendMode(RL_BLEND_ALPHA);

		DrawUI();

		EndDrawing();
	}
};

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