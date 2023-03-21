#include "Circles.h"
#include <string>
#include <random>
#include <iostream>

// Calculate the distance between two points
float Distance(float x1, float y1, float x2, float y2)
{
	// THIS IS AN ISSUE AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
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
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mMovingPositions[i] += mVelocities[i] * frameTime;
		for (int j = 0; j < NUM_CIRCLES / 2; j++)
		{
			auto pos1 = mMovingPositions[i];
			auto pos2 = mStillPositions[j];
			// Calculate distance between circles (keep squared for performance)
			auto distanceBetweenCirclesSquared = ((pos2.x - pos1.x) * (pos2.x - pos1.x)) + ((pos2.y - pos1.y) * (pos2.y - pos1.y));

			// Check for collision
			if (distanceBetweenCirclesSquared <= 400)
			{
				// collision detected
				mHealths[i] -= 20;
				mHealths[j] -= 20;

				mCollisions.push_back(Collision{ i, j });

				// Calculate the normal vector pointing from c1 to c2
				auto distance = Distance(pos1.x, pos1.y, pos2.x, pos2.y);
				float nx = (pos2.x - pos1.x) / distance;
				float ny = (pos2.y - pos1.y) / distance;

				// v - 2 * (v . n) * n
				Reflect(mVelocities[i].x, mVelocities[i].y, nx, ny);
			}
		}
	}
}

void Circles::InitCircles()
{
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	std::uniform_int_distribution<> velDistr(-5, 5); // define the range
	std::uniform_int_distribution<> posDistr(MIN_POS, MAX_POS); // define the range
	std::vector<Float3> usedPositions;
	
	Float3 position;
	float xrand;
	float yrand;
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		auto circle = new Circle();
		circle->mIndex = i;
		bool posChecked = false;
		bool isSame = false;
		while (!posChecked)
		{
			// Set random value for position
			xrand = posDistr(gen);
			yrand = posDistr(gen);
			position = Float3{ xrand,yrand,0 };
/*			for (auto pos : usedPositions)
			{
				if (pos == position)
				{
					isSame = true;
					break;
				}
			}
			if (!isSame) */posChecked = true;
		}
		mMovingPositions[i] = position;

		circle->mRadius = 10;

		bool velicityZero = true;
		auto velocity = Float3{ 0,0,0 };
		while (velicityZero)
		{
			// Set random value for velocity
			xrand = velDistr(gen);
			yrand = velDistr(gen);
			velocity = Float3{ xrand,yrand,0 };
			if (velocity != Float3{ 0,0,0 }) velicityZero = false;
		}
		mVelocities[i] = velocity;
		circle->mHealth = 100;

		mMovingCircles[i] = circle;
		usedPositions.push_back(position);
	}

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		auto circle = new Circle();
		circle->mIndex = i;

		bool posChecked = false;
		bool isSame = false;
		while (!posChecked)
		{
			// Set random value for position
			xrand = posDistr(gen);
			yrand = posDistr(gen);
			position = Float3{ xrand,yrand,0 };
/*			for (auto pos : usedPositions)
			{
				if (pos == position)
				{
					isSame = true;
					break;
				}
			}
			if (!isSame)*/ posChecked = true;
		}

		mStillPositions[i] = position;

		circle->mRadius = 10;

		circle->mHealth = 100;

		mStillCircles[i] = circle;
		usedPositions.push_back(position);
	}
	mTimer = new Timer();
	mTimer->Start();
}

void Circles::OutputFrame()
{
	std::cout << std::endl << "FrameTime: " << mTimer->GetLapTime() << std::endl;
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
	delete mTimer;
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete mMovingCircles[i];
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete mStillCircles[i];
}