#pragma once
#include <vector>

const int NUM_CIRCLES = 500;
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
		Float3 mPosition;
		Float3 mVelocity;
		int mHealth;
	};

public:
	void InitCircles();
	void UpdateCircles(float frameTime);
	void OutputFrame();
	void ClearMemory();

	Circle* mMovingCircles[NUM_CIRCLES / 2];
	Circle* mStillCircles[NUM_CIRCLES / 2];
	std::vector<Collision> mCollisions;

};

