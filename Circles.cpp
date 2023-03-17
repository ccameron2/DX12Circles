#include "Circles.h"
#include <string>
#include <random>
#include <time.h> 
#include <iostream>
	
float Sqrt(const float& n)
{
	static union { int i; float f; } u;
	u.i = 0x5F375A86 - (*(int*)&n >> 1);
	return (int(3) - n * u.f * u.f) * n * u.f * 0.5f;
}

// Calculate the distance between two points
float Distance(float x1, float y1, float x2, float y2)
{
	return Sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

// Calculate the dot product of two vectors
float DotProduct(float x1, float y1, float x2, float y2)
{
	return x1 * x2 + y1 * y2;
}

// Reflect a vector around another vector
void Reflect(float& x, float& y, float nx, float ny)
{
	float dot = DotProduct(x, y, nx, ny);
	x = x - 2 * dot * nx;
	y = y - 2 * dot * ny;
}


void Circles::UpdateCircles(float frameTime)
{
	for (auto& circle : mMovingCircles)
	{
		circle->mPosition += circle->mVelocity;// * frameTime;
		for (auto& stillCircle : mStillCircles)
		{
			auto pos1 = circle->mPosition;
			auto pos2 = stillCircle->mPosition;
			// Calculate distance between circles (keep squared for performance)
			auto distanceBetweenCirclesSquared = ((pos2.x - pos1.x) * (pos2.x - pos1.x)) + ((pos2.y - pos1.y) * (pos2.y - pos1.y));

			// Check for collision
			if (distanceBetweenCirclesSquared <= 100)
			{
				// collision detected
				circle->mHealth -= 20;
				stillCircle->mHealth -= 20;

				mCollisions.push_back(Collision{ circle->mIndex, stillCircle->mIndex });

				// Calculate the normal vector pointing from c1 to c2
				auto distance = Distance(pos1.x, pos1.y, pos2.x, pos2.y);
				float nx = (pos2.x - pos1.x) / distance;
				float ny = (pos2.y - pos1.y) / distance;

				// v - 2 * (v . n) * n
				Reflect(circle->mVelocity.x, circle->mVelocity.y, nx, ny);
			}
		}
	}
}

void Circles::InitCircles()
{
	int maxPos = 1000;
	int minPos = -1000;

	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	std::uniform_int_distribution<> velDistr(-5, 5); // define the range
	std::uniform_int_distribution<> posDistr(minPos, maxPos); // define the range

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		auto circle = new Circle();
		circle->mIndex = i;

		// Set random value for position
		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		circle->mPosition = Float3{ xrand,yrand,0 };

		circle->mRadius = 10;

		// Set random value for velocity
		xrand = velDistr(gen);
		yrand = velDistr(gen);

		circle->mVelocity = Float3{ xrand,yrand,0 };

		circle->mHealth = 100;

		mMovingCircles[i] = circle;
	}

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		auto circle = new Circle();
		circle->mIndex = i;

		// Set random value for position
		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		circle->mPosition = Float3{ xrand,yrand,0 };

		circle->mRadius = 10;

		circle->mVelocity = Float3{ 0,0,0 };

		circle->mHealth = 100;

		mStillCircles[i] = circle;
	}
}

void Circles::OutputFrame()
{
	for (auto& collision : mCollisions)
	{
		int index1 = collision.circle1;
		int index2 = collision.circle2;
		auto sIndex1 = std::to_string(index1);
		auto sIndex2 = std::to_string(index2);
		std::cout << "Moving Circle " << sIndex1 << " collided with " << "Still Circle " << sIndex2 << std::endl;
	}
	mCollisions.clear();
}

void Circles::ClearMemory()
{
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete mMovingCircles[i];
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete mStillCircles[i];
}