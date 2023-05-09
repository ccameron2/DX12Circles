#pragma once
#include <vector>
#include "Timer.h"
#include <thread>
#include <condition_variable>

const uint32_t NUM_CIRCLES = 1000000;
const int MAX_POS = 5000;
const int MIN_POS = -5000;
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

	//---------------------------------------------------------------------------------------------------------------------
	// Thread Pools
	//---------------------------------------------------------------------------------------------------------------------

	// A worker thread wakes up when work is signalled to be ready, and signals back when the work is complete.
	// Same condition variable is used for signalling in both directions.
	// A mutex is used to guard data shared between threads
	struct WorkerThread
	{
		std::thread             thread;
		std::condition_variable workReady;
		std::mutex              lock;
	};

	// Data describing work to do by a worker thread - this task is collision detection between some sprites against some blockers
	struct BlockCirclesWork
	{
		bool complete = true;
		Circle* movingCircles; // The work is described simply as the parameters to the BlockSprites function
		uint32_t numMovingCircles;
		Circle* stillCircles;
		uint32_t numStillCircles;
		uint32_t startIndex;
		std::vector<Collision> collisions;
	};

	// A pool of worker threads, each with its associated work
	// A more flexible system could generalise the type of work that worker threads can do
	static const uint32_t MAX_WORKERS = 31;
	std::pair<WorkerThread, BlockCirclesWork> mBlockCirclesWorkers[MAX_WORKERS];
	uint32_t mNumWorkers;  // Actual number of worker threads being used in array above
public:
	~Circles();
	void InitCircles();
	void UpdateCircles(float frameTime = 1);
	void OutputFrame();
	void ClearMemory();
	void BlockCircles(Circle* movingCircles, uint32_t numMovingCircles, const Circle* stillCircles, uint32_t numStillCircles,
							uint32_t index, std::vector<Collision>& collisions);

	void BlockCirclesThread(uint32_t thread);

	std::vector<Collision> mCollisions;

	Circle* mMovingCircles;
	Circle* mStillCircles;

	Timer* mTimer;
};

