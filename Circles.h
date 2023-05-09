#pragma once
#include <vector>
#include "Timer.h"
#include <thread>
#include <condition_variable>

const uint32_t NUM_CIRCLES = 100000;
const int MAX_POS = 10000;
const int MIN_POS = -10000;
const bool OUTPUT_COLLISIONS = false;
const bool THREADED = true;

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
		Float3 operator-= (Float3 input)
		{
			Float3 result;
			result.x = this->x -= input.x;
			result.y = this->y -= input.y;
			result.z = this->z -= input.z;
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
		Float3 mPosition;
		Float3 mVelocity;
		//int mRadius;
		//int mHealth;
	};

	struct WorkerThread
	{
		std::thread             thread;
		std::condition_variable workReady;
		std::mutex              lock;
	};

	struct BlockCirclesWork
	{
		bool complete = true;
		Circle* movingCircles;
		uint32_t numMovingCircles;
		Circle* stillCircles;
		uint32_t numStillCircles;
		uint32_t startIndex;
		std::vector<Collision> collisions;
	};

	static const uint32_t MAX_WORKERS = 31;
	std::pair<WorkerThread, BlockCirclesWork> mBlockCirclesWorkers[MAX_WORKERS];
	uint32_t mNumWorkers;
public:
	~Circles();
	void InitCircles();
	void UpdateCircles(float frameTime = 1);
	void OutputFrame();
	void ClearMemory();
	void BlockCircles(Circle* movingCircles, uint32_t numMovingCircles, Circle* stillCircles, uint32_t numStillCircles,
							uint32_t index, std::vector<Collision>& collisions);
	void BlockCirclesLS(Circle* movingCircles, uint32_t numMovingCircles, Circle* stillCircles, uint32_t numStillCircles,
		uint32_t startIndex, std::vector<Collision>& collisions);

	void BlockCirclesThread(uint32_t thread);
	void SortCirclesByX(Circle* circles, int numCircles);
	bool CheckWalls(Circle* circle);
	std::vector<Collision> mCollisions;

	Circle* mMovingCircles;
	Circle* mStillCircles;

	Timer* mTimer;
};

