#pragma once
#include <random>

class RandomFloat
{
public:
	float GetRandomFloat(float min, float max)
	{
		std::random_device seed;
		std::uniform_real_distribution<float> dist(min, max);
		myRandomEngine = std::mt19937(seed());
		return dist(myRandomEngine);
	}
private:
	std::mt19937 myRandomEngine;
};