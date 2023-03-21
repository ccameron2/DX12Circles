#pragma once
#include <vector>
#include "Timer.h"
#include <thread>
#include <condition_variable>

const int NUM_CIRCLES = 60000;
const int MAX_POS = 5000;
const int MIN_POS = -5000;

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
		Float3* movingPositions; // The work is described simply as the parameters to the BlockSprites function
		uint32_t numMovingCircles;
		Float3* stillPositions;
		uint32_t numStillCircles;
		uint32_t startIndex;
	};

	// A pool of worker threads, each with its associated work
	// A more flexible system could generalise the type of work that worker threads can do
	static const uint32_t MAX_WORKERS = 31;
	std::pair<WorkerThread, BlockCirclesWork> mBlockCirclesWorkers[MAX_WORKERS];
	uint32_t mNumWorkers;  // Actual number of worker threads being used in array above
	bool mThreaded = true;
public:
	~Circles();
	void InitCircles();
	void UpdateCircles(float frameTime = 1);
	void OutputFrame();
	void ClearMemory();
	void BlockCircles(Float3* movingPositions, uint32_t numMovingCircles, Float3* stillPositions, uint32_t numStillCircles, uint32_t index);
	void BlockCirclesThread(uint32_t thread);
	Float3 mMovingPositions[NUM_CIRCLES / 2];
	Float3 mStillPositions[NUM_CIRCLES / 2];
	Float3 mVelocities[NUM_CIRCLES / 2];
	int mHealths[NUM_CIRCLES];

	Circle* mMovingCircles[NUM_CIRCLES / 2];
	Circle* mStillCircles[NUM_CIRCLES / 2];
	std::vector<Collision> mCollisions;

	Timer* mTimer;
};

