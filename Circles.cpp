#include "Circles.h"
#include <string>
#include <random>
#include <iostream>

// Calculate the distance between two points
float Distance(float x1, float y1, float x2, float y2)
{
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
		if (frameTime != 1) frameTime *= 10;
		mMovingCircles[i].mPosition += mMovingCircles[i].mVelocity * frameTime;
	}

	if (THREADED)
	{
		auto movingCircles = &mMovingCircles[0];

		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			auto& work = mBlockCirclesWorkers[i].second;
			work.movingCircles = movingCircles;
			work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1);
			work.stillCircles = &mStillCircles[0];
			work.numStillCircles = NUM_CIRCLES / 2;
			work.collisions.clear();

			auto& workerThread = mBlockCirclesWorkers[i].first;
			{
				std::unique_lock<std::mutex> l(workerThread.lock);
				work.complete = false;
			}

			workerThread.workReady.notify_one();

			movingCircles += work.numMovingCircles;
		}

		uint32_t numRemainingParticles = (NUM_CIRCLES / 2) - static_cast<uint32_t>(movingCircles - &mMovingCircles[0]);
		BlockCirclesLS(movingCircles, numRemainingParticles, &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);

		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			auto& workerThread = mBlockCirclesWorkers[i].first;
			auto& work = mBlockCirclesWorkers[i].second;

			std::unique_lock<std::mutex> l(workerThread.lock);
			workerThread.workReady.wait(l, [&]() { return work.complete; });

			{
				for (auto& collision : work.collisions)
				{
					mCollisions.push_back(collision);
				}
			}					
		}	

	}
	else
	{
		BlockCirclesLS(&mMovingCircles[0], NUM_CIRCLES / 2, &mStillCircles[0], NUM_CIRCLES / 2,  mCollisions);
	}
}


Circles::~Circles()
{
	for (uint32_t i = 0; i < mNumWorkers; ++i)
	{
		mBlockCirclesWorkers[i].first.thread.detach();
	}
}

void Circles::InitCircles()
{
	if (THREADED)
	{
		mNumWorkers = std::thread::hardware_concurrency();
		if (mNumWorkers == 0)  mNumWorkers = 8;
		--mNumWorkers;
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			mBlockCirclesWorkers[i].first.thread = std::thread(&Circles::BlockCirclesThread, this, i);
		}
	}
	
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	std::uniform_int_distribution<> velDistr(MIN_VEL, MAX_VEL); // define the range
	std::uniform_int_distribution<> posDistr(MIN_POS, MAX_POS); // define the range
	std::uniform_int_distribution<> radDistr(MIN_RAD, MAX_RAD); // define the range
	std::vector<Float3> usedPositions;
	
	mMovingCircles = new Circle[NUM_CIRCLES / 2];
	mStillCircles = new Circle[NUM_CIRCLES / 2];
	Float3 position;
	float xrand;
	float yrand;
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mMovingCircles[i].mName = "Moving Circle " + std::to_string(i);
	
		bool posChecked = false;
		bool isSame = false;

		position = Float3{ 0,0,0 };
		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		position = Float3{ xrand, yrand, 0 };

		//while (!posChecked)
		//{
		//	// Generate a random position
		//	float xrand = posDistr(gen);
		//	float yrand = posDistr(gen);
		//	position = Float3{ xrand, yrand, 0 };

		//	// Check if the position has already been used
		//	auto it = std::find_if(usedPositions.begin(), usedPositions.end(),
		//		[position](const Float3& p) { return p.x == position.x && p.y == position.y; });
		//	if (it == usedPositions.end())
		//	{
		//		// Position is unique, so use it
		//		usedPositions.push_back(position);
		//		posChecked = true;
		//	}
		//}

		mMovingCircles[i].mPosition = position;

		
		mMovingCircles[i].mColour = { 1, 0, 1 };
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
		mMovingCircles[i].mVelocity = velocity;

		if (RANDOM_RADIUS) mMovingCircles[i].mRadius = radDistr(gen);
		else mMovingCircles[i].mRadius = 10;

		mMovingCircles[i].mHealth = 100;

		usedPositions.push_back(position);
	}

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mStillCircles[i].mName = "Still Circle " + std::to_string(i);

		bool posChecked = false;
		bool isSame = false;
		
		position = Float3{ 0,0,0 };
		posChecked = false;

		position = Float3{ 0,0,0 };

		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		position = Float3{ xrand, yrand, 0 };

		//while (!posChecked)
		//{
		//	// Generate a random position
		//	float xrand = posDistr(gen);
		//	float yrand = posDistr(gen);
		//	position = Float3{ xrand, yrand, 0 };

		//	// Check if the position has already been used
		//	auto it = std::find_if(usedPositions.begin(), usedPositions.end(),
		//		[position](const Float3& p) { return p.x == position.x && p.y == position.y; });
		//	if (it == usedPositions.end())
		//	{
		//		// Position is unique, so use it
		//		usedPositions.push_back(position);
		//		posChecked = true;
		//	}
		//}

		mStillCircles[i].mPosition = position;
		mStillCircles[i].mVelocity = { 0,0,0 };

		if (RANDOM_RADIUS) mStillCircles[i].mRadius = radDistr(gen);
		else mStillCircles[i].mRadius = 10;

		mStillCircles[i].mColour = { 1, 0.8, 0 };

		mStillCircles[i].mHealth = 100;

		usedPositions.push_back(position);
	}

	SortCirclesByX(mStillCircles, NUM_CIRCLES / 2);

	mTimer = new Timer();
	mTimer->Start();
}

void Circles::OutputFrame()
{
	std::cout << std::endl << "FrameTime: " << mTimer->GetLapTime() << std::endl;

	if (!OUTPUT_COLLISIONS) return;

	for (auto& collision : mCollisions)
	{
		auto name1 = collision.Circle1Name;
		auto name2 = collision.Circle2Name;
		auto health1 = collision.Circle1Health;
		auto health2 = collision.Circle2Health;
		auto time = collision.TimeOfCollision;

		std::cout << name1 << " with " << health1 << " health remaining, collided with " << name2 << " with " << health2 << " health remaining, at " << time << "." << std::endl;
	}

	mCollisions.clear();
}

void Circles::ClearMemory()
{
	delete mTimer;
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete[] mMovingCircles;
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete[] mStillCircles;
}

void Circles::BlockCirclesLS(Circle* movingCircles, uint32_t numMovingCircles, Circle* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions)
{
	Circle* movingEnd = movingCircles + numMovingCircles - 1;

	const Circle* stillEnd = stillCircles + numStillCircles - 1;
	while (movingCircles != movingEnd)
	{
		auto x1 = movingCircles->mPosition.x;
		auto y1 = movingCircles->mPosition.y;

		auto still = stillCircles;
		int stillIndex = 0;
		bool col = false;

		int mLSweepSide = 15;
		int mRSweepSide = 15;
		if (RANDOM_RADIUS)
		{
			mLSweepSide = movingCircles->mRadius + 5;
			mRSweepSide = movingCircles->mRadius + 5;
		}

		auto start = still;
		auto end = stillEnd;
		Circle* mid;
		bool found = false;
		do
		{
			mid = start + (end - start) / 2;
			if (movingCircles->mPosition.x + mRSweepSide <= mid->mPosition.x)  end = mid;
			else if (movingCircles->mPosition.x - mLSweepSide >= mid->mPosition.x)  start = mid;
			else found = true;
		} while (!found && end - start > 1);

		if (found)
		{
			// Search from circle found rightwards 
			auto blocker = mid;
			while (movingCircles->mPosition.x + mRSweepSide > blocker->mPosition.x && blocker != stillEnd)
			{
				auto minColDistance = 400;
				if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + blocker->mRadius)* (movingCircles->mRadius + blocker->mRadius);

				auto x2 = blocker->mPosition.x;
				auto y2 = blocker->mPosition.y;
				auto distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
				if (distanceBetweenCirclesSquared <= minColDistance)
				{
					if(movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
					if(blocker->mHealth - 20 >= 0) blocker->mHealth -= 20;

					if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, blocker->mName, movingCircles->mHealth, blocker->mHealth, mTimer->GetTime()});

					auto distance = Distance(x1, y1, x2, y2);
					float nx = (x2 - x1) / distance;
					float ny = (y2 - y1) / distance;

					// v - 2 * (v . n) * n
					Reflect(movingCircles->mVelocity.x, movingCircles->mVelocity.y, nx, ny);
					col = true;
					break;
				}
				++blocker;
			}

			if (col)
			{
				++movingCircles;
				continue;
			}

			blocker = mid;
			while (blocker-- != stillCircles && movingCircles->mPosition.x - mLSweepSide < blocker->mPosition.x)
			{
				auto minColDistance = 400;
				if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + blocker->mRadius) * (movingCircles->mRadius + blocker->mRadius);

				auto x2 = blocker->mPosition.x;
				auto y2 = blocker->mPosition.y;
				auto distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
				if (distanceBetweenCirclesSquared <= minColDistance)
				{
					if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
					if (blocker->mHealth - 20 >= 0) blocker->mHealth -= 20;

					if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, blocker->mName, movingCircles->mHealth, blocker->mHealth, mTimer->GetTime() });

					auto distance = Distance(x1, y1, x2, y2);
					float nx = (x2 - x1) / distance;
					float ny = (y2 - y1) / distance;

					// v - 2 * (v . n) * n
					Reflect(movingCircles->mVelocity.x, movingCircles->mVelocity.y, nx, ny);
					col = true;
					break;
				}
			}
			if (col)
			{
				++movingCircles;
				continue;
			}
		}

		if (!WALLS) 
		{
			++movingCircles;
			continue;
		} 

		int maxPos = MAX_POS + WALL_DISTANCE_FROM_EDGE;
		int minPos = MIN_POS - WALL_DISTANCE_FROM_EDGE;

		bool wallCollision = false;
		bool wallLeftCollision = false;
		bool wallRightCollision = false;
		bool wallUpCollision = false;
		bool wallDownCollision = false;

		if (x1 >= maxPos) { wallRightCollision = true; wallCollision = true; }
		else if (y1 >= maxPos) { wallUpCollision = true;	  wallCollision = true; }
		else if (x1 <= minPos) { wallLeftCollision = true;  wallCollision = true; }
		else if (y1 <= minPos) { wallDownCollision = true;  wallCollision = true; }

		if (wallCollision)
		{
			int x2 = 0;
			int y2 = 0;

			if (wallLeftCollision) { x2 = minPos;  y2 = y1; }
			else if (wallRightCollision) { x2 = maxPos; y2 = y1; }
			else if (wallUpCollision) { x2 = x1; y2 = maxPos; }
			else if (wallDownCollision) { x2 = x1; y2 = minPos; }

			// Calculate the normal vector pointing from c1 to c2
			auto distance = Distance(x1, y1, x2, y2);
			float nx = (x2 - x1) / distance;
			float ny = (y2 - y1) / distance;

			// v - 2 * (v . n) * n
			Reflect(movingCircles->mVelocity.x, movingCircles->mVelocity.y, nx, ny);
			++movingCircles;
			continue;
		}
		++movingCircles;
	}
}
void Circles::BlockCircles(Circle* movingCircles, uint32_t numMovingCircles, Circle* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions)
{
	Circle* nextCircle = movingCircles + 1;
	Circle* movingEnd = movingCircles + numMovingCircles - 1;

	const Circle* stillEnd = stillCircles + numStillCircles - 1;
	while (movingCircles != movingEnd)
	{
		auto x1 = movingCircles->mPosition.x;
		auto y1 = movingCircles->mPosition.y;

		auto still = stillCircles;
		bool col = false;
		while (still != stillEnd)
		{
			auto minColDistance = 400;
			if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + still->mRadius) * (movingCircles->mRadius + still->mRadius);

			auto x2 = still->mPosition.x;
			auto y2 = still->mPosition.y;
			auto distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

			// Check for collision
			if (distanceBetweenCirclesSquared <= minColDistance)
			{
				// collision detected
				
				if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
				if (still->mHealth - 20 >= 0) still->mHealth -= 20;

				if(OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, still->mName, movingCircles->mHealth, still->mHealth, mTimer->GetTime() });

				// Calculate the normal vector pointing from c1 to c2
				auto distance = Distance(x1, y1, x2, y2);
				float nx = (x2 - x1) / distance;
				float ny = (y2 - y1) / distance;

				// v - 2 * (v . n) * n
				Reflect(movingCircles->mVelocity.x, movingCircles->mVelocity.y, nx, ny);
				col = true;
				break;
			}
			++still;
		}		
		if (col)
		{
			++movingCircles;
			continue;
		}

		if (!WALLS)
		{
			++movingCircles;
			continue;
		}

		int maxPos = MAX_POS + WALL_DISTANCE_FROM_EDGE;
		int minPos = MIN_POS - WALL_DISTANCE_FROM_EDGE;

		bool wallCollision = false;
		bool wallLeftCollision = false;
		bool wallRightCollision = false;
		bool wallUpCollision = false;
		bool wallDownCollision = false;

		if (x1 >= maxPos) { wallRightCollision = true; wallCollision = true; }
		else if (y1 >= maxPos) { wallUpCollision = true;	  wallCollision = true; }
		else if (x1 <= minPos) { wallLeftCollision = true;  wallCollision = true; }
		else if (y1 <= minPos) { wallDownCollision = true;  wallCollision = true; }

		if (wallCollision)
		{
			int x2 = 0;
			int y2 = 0;

			if (wallLeftCollision) { x2 = minPos;  y2 = y1; }
			else if (wallRightCollision) { x2 = maxPos; y2 = y1; }
			else if (wallUpCollision) { x2 = x1; y2 = maxPos; }
			else if (wallDownCollision) { x2 = x1; y2 = minPos; }

			// Calculate the normal vector pointing from c1 to c2
			auto distance = Distance(x1, y1, x2, y2);
			float nx = (x2 - x1) / distance;
			float ny = (y2 - y1) / distance;

			// v - 2 * (v . n) * n
			Reflect(movingCircles->mVelocity.x, movingCircles->mVelocity.y, nx, ny);
			++movingCircles;
			continue;
		}
		++movingCircles;
	}
}

void Circles::BlockCirclesThread(uint32_t thread)
{
	auto& worker = mBlockCirclesWorkers[thread].first;
	auto& work = mBlockCirclesWorkers[thread].second;
	while (true)
	{
		{
			std::unique_lock<std::mutex> l(worker.lock);
			worker.workReady.wait(l, [&]() { return !work.complete; });
		}
		//BlockCircles(work.movingCircles, work.numMovingCircles, work.stillCircles, work.numStillCircles, work.collisions);
		BlockCirclesLS(work.movingCircles, work.numMovingCircles, work.stillCircles, work.numStillCircles, work.collisions);
		{
			std::unique_lock<std::mutex> l(worker.lock);
			work.complete = true;
		}
		worker.workReady.notify_one();
	}
}

void Circles::SortCirclesByX(Circle* circles, int numCircles)
{
	if (numCircles <= 1)
		return;

	int pivotIndex = numCircles / 2;
	float pivotX = circles[pivotIndex].mPosition.x;

	Circle* left = circles;
	Circle* right = circles + numCircles - 1;

	while (left <= right)
	{
		if (left->mPosition.x < pivotX)
		{
			left++;
			continue;
		}
		if (right->mPosition.x > pivotX)
		{
			right--;
			continue;
		}

		Circle temp = *left;
		*left = *right;
		*right = temp;
		left++;
		right--;
	}

	SortCirclesByX(circles, right - circles + 1);
	SortCirclesByX(left, circles + numCircles - left);
}