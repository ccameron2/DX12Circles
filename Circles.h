#pragma once
#include <vector>
#include "Timer.h"

const int NUM_CIRCLES = 2000;
const int MAX_POS = 1000;
const int MIN_POS = -1000;

class Circles
{
private:
	struct Float3
	{
		float x;
		float y;
		float z;
		Float3 operator* (float input)
		{
			Float3 result;
			result.x = this->x * input;
			result.y = this->y * input;
			result.z = this->z * input;
			return result;
		}
		Float3 operator* (Float3 input)
		{
			Float3 result;
			result.x = this->x * input.x;
			result.y = this->y * input.y;
			result.z = this->z * input.z;
			return result;
		}
		Float3 operator+= (Float3 input)
		{
			Float3 result;
			result.x = this->x += input.x;
			result.y = this->y += input.y;
			result.z = this->z += input.z;
			return result;
		}
		Float3 operator- (Float3 input)
		{
			Float3 result;
			result.x = this->x - input.x;
			result.y = this->y - input.y;
			result.z = this->z - input.z;
			return result;
		}
		Float3 operator- (float input)
		{
			Float3 result;
			result.x = this->x - input;
			result.y = this->y - input;
			result.z = this->z - input;
			return result;
		}
		bool operator==(Float3 input)
		{
			if (this->x == input.x && this->y == input.y && this->z == input.z)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	struct Collision
	{
		int circle1;
		int circle2;
		float time;
	};
	
	struct Circle
	{
		int mIndex;
		int mRadius;
		int mHealth;
	};

public:
	void InitCircles();
	void UpdateCircles(float frameTime = 1);
	void OutputFrame();
	void ClearMemory();

	Float3 mMovingPositions[NUM_CIRCLES / 2];
	Float3 mStillPositions[NUM_CIRCLES / 2];
	Float3 mVelocities[NUM_CIRCLES / 2];
	int mHealths[NUM_CIRCLES];

	Circle* mMovingCircles[NUM_CIRCLES / 2];
	Circle* mStillCircles[NUM_CIRCLES / 2];
	std::vector<Collision> mCollisions;

	Timer* mTimer;
};

