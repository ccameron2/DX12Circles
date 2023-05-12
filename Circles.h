#pragma once
#include <vector>
#include "Timer.h"
#include <thread>
#include <condition_variable>
#include <cmath>

// TODO:
// Fix sphere collision
// Line sweep moving-moving
// 

const bool VISUAL = false;

const uint32_t NUM_CIRCLES = 1000000;
const int MAX_POS = 12000; //12000
const int MIN_POS = -12000;
const int MAX_VEL = 5;
const int MIN_VEL = -5;
const int MAX_RAD = 20;
const int MIN_RAD = 10;
const int WALL_DISTANCE_FROM_EDGE = 100;

const bool OUTPUT_COLLISIONS = false;
const bool THREADED = true;
const bool LINE_SWEEP = true;
const bool WALLS = true;
const bool RANDOM_RADIUS = false;
const bool SPHERES = false;
const bool CIRCLE_DEATH = false;
const bool MOVING_MOVING = false;

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
			//this->mPosition += Float3{ -nx,-ny,0 };
		}
	};

	struct Sphere
	{
		std::string mName;
		Float3 mPosition;
		Float3 mVelocity;
		int mRadius;
		int mHealth;
		Float3 mColour;
		float Distance(float x1, float y1, float x2, float y2, float z1, float z2)
		{
			return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z1 + z2) * (z1 + z2));
		}
		// Calculate the dot product of two vectors
		float DotProduct(float x1, float y1, float z1, float x2, float y2, float z2)
		{
			return x1 * x2 + y1 * y2 + z1 * z2;
		}
		// Reflect a circle around another circle
		void Reflect(Sphere* stillCircle)
		{
			auto stillPos = stillCircle->mPosition;
			auto movingPos = this->mPosition;
			auto x1 = movingPos.x;
			auto y1 = movingPos.y;
			auto x2 = stillPos.x;
			auto y2 = stillPos.y;
			auto z1 = movingPos.z;
			auto z2 = stillPos.z;
			// Calculate the normal vector pointing from c1 to c2
			auto distance = Distance(x1, y1, x2, y2, z1, z2);

			float nx = (x2 - x1) / distance;
			float ny = (y2 - y1) / distance;
			float nz = (z2 - z1) / distance;

			auto movingVel = this->mVelocity;
			auto x = movingVel.x;
			auto y = movingVel.y;
			auto z = movingVel.z;

			float dot = DotProduct(x, y, z, nx, ny, nz);
			this->mVelocity.x = x - 2 * dot * nx;
			this->mVelocity.y = y - 2 * dot * ny;
			this->mVelocity.z = z - 2 * dot * nz;
			this->mPosition += Float3{ -nx,-ny,-nz };
		}
	};

	struct WorkerThread
	{
		std::thread             thread;
		std::condition_variable workReady;
		std::mutex              lock;
	};

	template<class T>
	struct BlockCirclesWork
	{
		bool complete = true;
		T* movingCircles;
		uint32_t numMovingCircles;
		T* firstMovingCircle;
		T* stillCircles;
		uint32_t numStillCircles;
		std::vector<Collision> collisions;
	};

	static const uint32_t MAX_WORKERS = 31;
	std::pair<WorkerThread, BlockCirclesWork<Circle>> mBlockCirclesWorkers[MAX_WORKERS];
	std::pair<WorkerThread, BlockCirclesWork<Sphere>> mBlockSpheresWorkers[MAX_WORKERS];

	uint32_t mNumWorkers;
public:
	~Circles();
	void InitCircles();
	void UpdateCircles(float frameTime = 1);
	void OutputFrame();
	void ClearMemory();

	template<class T>
	void BlockCircles(T* movingCircles, uint32_t numMovingCircles, T* firstMovingCircle, T* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions);
	
	template<class T>
	void BlockCirclesLS(T* movingCircles, uint32_t numMovingCircles, T* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions);

	void BlockCirclesThread(uint32_t thread);

	template<class T>
	void SortCirclesByX(T* circles, int numCircles);
	std::vector<Collision> mCollisions;

	Circle* mMovingCircles;
	Circle* mStillCircles;
	Sphere* mMovingSpheres;
	Sphere* mStillSpheres;
	Timer* mTimer;
};

