#pragma once
#include <vector>
#include "Timer.h"
#include <thread>
#include <condition_variable>
#include <cmath>

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
const bool RANDOM_RADIUS = false;
const bool SPHERES = false;
const bool CIRCLE_DEATH = false;

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
		float Distance(float x1, float y1, float x2, float y2)
		{
			return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
		}
		// Calculate the dot product of two vectors
		float DotProduct(float x1, float y1, float x2, float y2)
		{
			return x1 * x2 + y1 * y2;
		}
		// Reflect a circle around another circle
		void Reflect(Circle* stillCircle)
		{
			auto stillPos = stillCircle->mPosition;
			auto movingPos = this->mPosition;
			auto x1 = movingPos.x;
			auto y1 = movingPos.y;
			auto x2 = stillPos.x;
			auto y2 = stillPos.y;

			// Calculate the normal vector pointing from c1 to c2
			auto distance = Distance(x1,y1,x2,y2);
			float nx = (x2 - x1) / distance;
			float ny = (y2 - y1) / distance;

			auto movingVel = this->mVelocity;
			auto x = movingVel.x;
			auto y = movingVel.y;
			float dot = DotProduct(x, y, nx, ny);
			this->mVelocity.x = x - 2 * dot * nx;
			this->mVelocity.y = y - 2 * dot * ny;
		}
	};

	// To get templates used, use Circles and Spheres. Circles store a Float2 instead of a Float3.
	// Init spheres instead of circles in a diff function with a bool check and then in the collision,
	// use movingCircle->Reflect(stillcircle) or movingCircle->Distance(stillCircle)

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
	std::vector<Collision> mCollisions;

	Circle* mMovingCircles;
	Circle* mStillCircles;

	Timer* mTimer;
};

