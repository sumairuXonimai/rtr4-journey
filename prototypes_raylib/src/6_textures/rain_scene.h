/// this scene renders rain particles as a transparent to opaque gradient 1d texture

#pragma once
#include "../Scene.h"

struct RainParticle
{
	float x{};
	float y{};
	float length{};
	float width{};
	float speed{};

	bool alive{ false };

	void reset(float lineLength, float lineWidth, float rainSpeed, float depthFactor, float depth) {
		x = getRandomFloat(0, g_windowWidth); y = getRandomFloat(-100, -10); alive = true;
		depth *= depthFactor;
		length = lineLength + (depth * lineLength);
		width = lineWidth + (depth * lineWidth);
		speed = rainSpeed + (depth * rainSpeed);
	}

	void update(float dt, float windAngle)
	{
		y += dt * speed;

		x += dt * speed * int(windAngle * DEG2RAD);
	}
};

class RainParticlePool
{
public:
	RainParticlePool(size_t object_count)
	{
		pool.resize(object_count);
		available_indices.reserve(object_count);
		for (int i = 0; i < object_count; i++)
			available_indices.push_back(i);
	}

	RainParticle& getParticle(float lineLength, float lineWidth, float rainSpeed, float depthFactor, float depth)
	{
		if (!available_indices.size()) {
			exit(-1);
		}

		size_t idx{ available_indices.back() };
		available_indices.pop_back();

		pool[idx].reset(lineLength, lineWidth, rainSpeed, depthFactor, depth);
		return pool[idx];
	}

	void despawn(RainParticle* particle)
	{
		if (!particle || !particle->alive) return;

		particle->alive = false;

		auto idx{ particle - pool.data() };
		available_indices.push_back(idx);
	}

private:
	std::vector<RainParticle> pool;
	std::vector<size_t> available_indices;
};

class RainScene : public Scene2D
{
public:
	RainScene()
	{
		Image img = GenImageColor(1, 64, BLANK);
		for (int y = 0; y < 64; y++)
		{
			float t{ (float)y / (float)63 };
			unsigned char alpha{ (unsigned char)(t * 255) };

			ImageDrawPixel(&img, 0, y, Color{ 255, 255, 255, alpha });
		}
		gradTexture = LoadTextureFromImage(img);
		UnloadImage(img);
	}

	void Draw() override
	{
		float dt{ GetFrameTime() };
		currentTime += dt;
		if (currentTime > timer)
		{
			for (int i = 0; i < rainCountPerPeriod; i++)
			{
				particles.push_back(&particlePool.getParticle(lineLength, lineWidth, rainSpeed, depthFactor, getRandomFloat(0.1f, 1.f)));
			}
			currentTime = 0.f;
		}

		//std::cout << particles.size() << '\n';

		for (auto& particle : particles)
			particle->update(dt, windAngle);

		particles.erase(std::remove_if(particles.begin(), particles.end(), [&](RainParticle* particle) {
			if (particle->y > g_windowHeight + 100.f || particle->x < -50.f || particle->x > g_windowWidth + 50.f)
			{
				particlePool.despawn(particle);
				return true;
			}
			return false;
			}), particles.end());

		BeginDrawing();

		ClearBackground(SKYBLUE);

		//DrawTexture(gradTexture, 0, 0, WHITE);

		for (auto* particle : particles)
		{
			Rectangle srcRec{ 0.f, 0.f, (float)gradTexture.width, (float)gradTexture.height };
			Rectangle dstRec{ particle->x, particle->y, particle->width, particle->length };

			Vector2 origin{ particle->width / 2.f, 0.f };

			//std::cout << particle->x << ' ' << particle->y << '\n';
			//std::cout << gradTexture.width << ' ' << gradTexture.height << '\n';

			DrawTexturePro(gradTexture,
				srcRec,
				dstRec,
				origin,
				windAngle,
				WHITE);
		}

		DrawUI();

		EndDrawing();
	}

	void DrawUI() override
	{
		DrawFPS(0, 0);

		rlImGuiBegin();

		if (ImGui::Begin("Params"))
		{
			//float lineLength{ 50 };
			//float lineWidth{ 3 };
			//int rainCountPerPeriod{ 5 };
			//float timer{ 0.01f };
			//float rainSpeed{ 500.f };
			//float depthFactor{ 1.f };
			//float windAngle{ 10.f };

			ImGui::SliderFloat("line length", &lineLength, 10, 100);
			ImGui::SliderFloat("line width", &lineWidth, 1, 10);
			ImGui::SliderFloat("rain speed", &rainSpeed, 400.f, 2000.f, "%.0f");
			ImGui::SliderFloat("wind angle", &windAngle, 5.f, 30.f);
			ImGui::SliderFloat("depth factor", &depthFactor, 0.5f, 2.0f);
			ImGui::SliderInt("rain count per spawn rate", &rainCountPerPeriod, 1, 10);
			ImGui::SliderFloat("spawn rate", &timer, 0.01f, 0.05f);

			ImGui::End();
		}

		rlImGuiEnd();
	}

	~RainScene() override
	{
		Scene2D::~Scene2D();

		UnloadTexture(gradTexture);
	}

private:
	Texture2D gradTexture{};

	RainParticlePool particlePool{ 10000 };
	std::vector<RainParticle*> particles;

	float lineLength{ 50 };
	float lineWidth{ 3 };
	int rainCountPerPeriod{ 5 };
	float timer{ 0.01f };
	float currentTime{ 0.f };
	float rainSpeed{ 500.f };
	float depthFactor{ 1.f };
	float windAngle{ 10.f };
};