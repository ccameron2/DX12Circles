#pragma once
#include <vector>
#include "Timer.h"
#include <thread>
#include <condition_variable>

const bool VISUAL = true;

const uint32_t NUM_CIRCLES = 100000;
const int MAX_POS = 12000;
const int MIN_POS = -12000;
const int MAX_VEL = 5;
const int MIN_VEL = -5;
const int MAX_RAD = 20;
const int MIN_RAD = 10;
const int WALL_DISTANCE_FROM_EDGE = 100;

const bool OUTPUT_COLLISIONS = false;
const bool THREADED = true;
const bool WALLS = true;
const bool RANDOM_RADIUS = true;

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
		std::string Circle1Name;
		std::string Circle2Name;
		int Circle1Health;
		int Circle2Health;
		float TimeOfCollision;
	};
	
	struct Circle
	{
		std::string mName;
		Float3 mPosition;
		Float3 mVelocity;
		int mRadius;
		int mHealth;
		Float3 mColour;
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
	void BlockCircles(Circle* movingCircles, uint32_t numMovingCircles, Circle* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions);
	void BlockCirclesLS(Circle* movingCircles, uint32_t numMovingCircles, Circle* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions);

	void BlockCirclesThread(uint32_t thread);
	void SortCirclesByX(Circle* circles, int numCircles);
	bool CheckWalls(Circle* circle);
	std::vector<Collision> mCollisions;

	Circle* mMovingCircles;
	Circle* mStillCircles;

	Timer* mTimer;
};

