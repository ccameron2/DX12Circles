#pragma once
#include <vector>
#include "Timer.h"
#include <thread>
#include <condition_variable>
#include <cmath>
#include <string>

// Visualisation toggle
const bool VISUAL = true;

// Number of circles to spawn
const uint32_t NUM_CIRCLES = 100000;

const int MAX_POS = 12000; // Max position of spawn area
const int MIN_POS = -12000; // Min position of spawn area
const int MAX_VEL = 5; // Max velocity of circle
const int MIN_VEL = -5; // Min velocity of circle
const int MAX_RAD = 20; // Max radius of circle
const int MIN_RAD = 10; // Min radius of circle
const int WALL_DISTANCE_FROM_EDGE = 100; // Wall distance from outside spawn area

const bool OUTPUT_COLLISIONS = false; // Toggle output collisions in console mode
const bool THREADED = true; // Toggle if collision detection uses multithreading
const bool LINE_SWEEP = true; // Toggle line sweep collision detection
const bool WALLS = true; // Toggle walls
const bool RANDOM_RADIUS = false; // Toggle random radius for circles
const bool SPHERES = false;  // Toggle 3D spheres
const bool CIRCLE_DEATH = false; // Toggle circle death when HP <= 0. No visualisation for circle death yet, they just dont affect collision or move anymore
const bool MOVING_MOVING = false; // Toggle moving-moving collisions

struct Float3
{
	float x;
	float y;
	float z;
	Float3 operator* (float input) // Mul float input
	{
		Float3 result;
		result.x = this->x * input;
		result.y = this->y * input;
		result.z = this->z * input;
		return result;
	}
	Float3 operator* (Float3 input) // Mul float3 input
	{
		Float3 result;
		result.x = this->x * input.x;
		result.y = this->y * input.y;
		result.z = this->z * input.z;
		return result;
	}
	Float3 operator+= (Float3 input) // Add float3
	{
		Float3 result;
		result.x = this->x += input.x;
		result.y = this->y += input.y;
		result.z = this->z += input.z;
		return result;
	}
	Float3 operator- (Float3 input) // Take float3
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
	Float3 operator- (float input) // Take float
	{
		Float3 result;
		result.x = this->x - input;
		result.y = this->y - input;
		result.z = this->z - input;
		return result;
	}
	bool operator==(Float3 input) // Compare float3 
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

class Circles
{
private:

	// Struct to hold collisions for console output
	struct Collision
	{
		// Names
		std::string Circle1Name;
		std::string Circle2Name;

		// Health
		int Circle1Health;
		int Circle2Health;

		// Time of collision
		float TimeOfCollision;
	};
	
	struct Circle
	{
		std::string mName; // Name
		Float3 mPosition; // Position
		Float3 mVelocity; // Velocity
		int mRadius; // Radius
		int mHealth; // Health
		Float3 mColour; // Colour

		// Cakculate distance between 2D points
		float Distance(float x1, float y1, float x2, float y2)
		{
			return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
		}

		// Calculate the dot product of 2D points
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

			this->mPosition += Float3{ -nx, -ny, 0 };
		}
	};

	struct Sphere
	{
		std::string mName; // Name
		Float3 mPosition; // Position
		Float3 mVelocity; // Velocity
		int mRadius; // Radius
		int mHealth; // Health
		Float3 mColour; // Colour

		// Calculat distance between two 3D points
		float Distance(float x1, float y1, float x2, float y2, float z1, float z2)
		{
			return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z1 + z2) * (z1 + z2));
		}

		// Calculate the dot product of two 3D points
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

	// Multithreading worker
	struct WorkerThread
	{
		std::thread             thread;
		std::condition_variable workReady;
		std::mutex              lock;
	};

	// Mulithreading work (Spheres or circles)
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

	// Multithreading data
	static const uint32_t MAX_WORKERS = 63;
	std::pair<WorkerThread, BlockCirclesWork<Circle>> mBlockCirclesWorkers[MAX_WORKERS];
	std::pair<WorkerThread, BlockCirclesWork<Sphere>> mBlockSpheresWorkers[MAX_WORKERS];
	uint32_t mNumWorkers;
public:
	~Circles();
	
	// Create circles
	void InitCircles();

	// Update circles for this frame
	void UpdateCircles(float frameTime = 1);

	// Output frametime and collisions
	void OutputFrame();

	// Destroy circles and close workers
	void ClearMemory();

	// Block circles brute force (supports moving-moving)
	template<class T>
	void BlockCircles(T* movingCircles, uint32_t numMovingCircles, T* firstMovingCircle, T* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions);
	
	// Block circles line sweep
	template<class T>
	void BlockCirclesLS(T* movingCircles, uint32_t numMovingCircles, T* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions);

	// Multithreading call to BlockCircles
	void BlockCirclesThread(uint32_t thread);

	// Sort circles by ascending x coordinate
	template<class T>
	void SortCirclesByX(T* circles, int numCircles);
	std::vector<Collision> mCollisions;

	// Circle arrays
	Circle* mMovingCircles;
	Circle* mStillCircles;

	// Sphere arrays
	Sphere* mMovingSpheres;
	Sphere* mStillSpheres;

	// Performance timer
	Timer* mTimer;
};

