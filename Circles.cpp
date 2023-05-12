#include "Circles.h"
#include <string>
#include <random>
#include <iostream>

// Calculate the distance between two points
float DistanceWall(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
}

// Calculate the dot product of two vectors
float DotProduct(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return x1 * x2 + y1 * y2 + z1 * z2;
}

// Reflect a vector around another vector
void ReflectWall(float& x, float& y, float& z, float nx, float ny, float nz)
{
	float dot = DotProduct(x, y, z, nx, ny, nz);
	x = x - 2 * dot * nx;
	y = y - 2 * dot * ny;
	z = z - 2 * dot * nz;
}

void Circles::UpdateCircles(float frameTime)
{
	if (frameTime != 1) frameTime *= 100;
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{		
		if(SPHERES) mMovingSpheres[i].mPosition += mMovingSpheres[i].mVelocity * frameTime;
		else mMovingCircles[i].mPosition += mMovingCircles[i].mVelocity * frameTime;
	}

	if (LINE_SWEEP && MOVING_MOVING)
	{
		if(SPHERES) SortCirclesByX(mMovingSpheres, NUM_CIRCLES / 2);
		else SortCirclesByX(mMovingCircles, NUM_CIRCLES / 2);
	}

	if (THREADED)
	{
		auto movingCircles = &mMovingCircles[0];
		auto movingSpheres = &mMovingSpheres[0];

		if (SPHERES)
		{
			for (uint32_t i = 0; i < mNumWorkers; ++i)
			{
				auto& work = mBlockSpheresWorkers[i].second;
				work.movingCircles = movingSpheres;
				work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1);
				work.firstMovingCircle = &mMovingSpheres[0];
				work.stillCircles = &mStillSpheres[0];
				work.numStillCircles = NUM_CIRCLES / 2;
				work.collisions.clear();

				auto& workerThread = mBlockSpheresWorkers[i].first;
				{
					std::unique_lock<std::mutex> l(workerThread.lock);
					work.complete = false;
				}

				workerThread.workReady.notify_one();

				movingSpheres += work.numMovingCircles;
			}

			uint32_t numRemainingParticles = (NUM_CIRCLES / 2) - static_cast<uint32_t>(movingSpheres - &mMovingSpheres[0]);
			if (LINE_SWEEP) BlockCirclesLS(movingSpheres, numRemainingParticles, &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(movingSpheres, numRemainingParticles, &mMovingSpheres[0], &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);

			for (uint32_t i = 0; i < mNumWorkers; ++i)
			{
				auto& workerThread = mBlockSpheresWorkers[i].first;
				auto& work = mBlockSpheresWorkers[i].second;

				std::unique_lock<std::mutex> l(workerThread.lock);
				workerThread.workReady.wait(l, [&]() { return work.complete; });

				if(OUTPUT_COLLISIONS)
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
			for (uint32_t i = 0; i < mNumWorkers; ++i)
			{
				auto& work = mBlockCirclesWorkers[i].second;
				work.movingCircles = movingCircles;
				work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1);
				work.firstMovingCircle = &mMovingCircles[0];
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
			if(LINE_SWEEP) BlockCirclesLS(movingCircles, numRemainingParticles, &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(movingCircles, numRemainingParticles, &mMovingCircles[0], &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
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
	}
	else
	{
		if (SPHERES) 
		{
			if(LINE_SWEEP) BlockCirclesLS(&mMovingSpheres[0], NUM_CIRCLES / 2, &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(&mMovingSpheres[0], NUM_CIRCLES / 2, &mMovingSpheres[0], &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);
		} 
		else
		{
			if (LINE_SWEEP) BlockCirclesLS(&mMovingCircles[0], NUM_CIRCLES / 2, &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(&mMovingCircles[0], NUM_CIRCLES / 2, &mMovingCircles[0], &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
		}
	}
}


Circles::~Circles()
{
	if (SPHERES)
	{
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			mBlockSpheresWorkers[i].first.thread.detach();
		}
	}
	else
	{
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			mBlockCirclesWorkers[i].first.thread.detach();
		}
	}
	
	ClearMemory();
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
			if(SPHERES) mBlockSpheresWorkers[i].first.thread = std::thread(&Circles::BlockCirclesThread, this, i);
			else mBlockCirclesWorkers[i].first.thread = std::thread(&Circles::BlockCirclesThread, this, i);
		}
	}
	
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	std::uniform_int_distribution<> velDistr(MIN_VEL, MAX_VEL); // define the range
	std::uniform_int_distribution<> posDistr(MIN_POS, MAX_POS); 
	std::uniform_int_distribution<> radDistr(MIN_RAD, MAX_RAD); 
	std::vector<Float3> usedPositions;
	
	if (SPHERES) 
	{
		mMovingSpheres = new Sphere[NUM_CIRCLES / 2];
		mStillSpheres = new Sphere[NUM_CIRCLES / 2];
	}
	else
	{
		mMovingCircles = new Circle[NUM_CIRCLES / 2];
		mStillCircles = new Circle[NUM_CIRCLES / 2];
	}

	Float3 position;
	float xrand;
	float yrand;
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		if(SPHERES) mMovingSpheres[i].mName = "Moving Sphere " + std::to_string(i);
		else mMovingCircles[i].mName = "Moving Circle " + std::to_string(i);

		bool posChecked = false;
		bool isSame = false;

		position = Float3{ 0,0,0 };
		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		float zrand = posDistr(gen);
		if (SPHERES) position = Float3{ xrand, yrand, zrand };
		else position = Float3{ xrand, yrand, 0 };
		
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

		if (SPHERES) mMovingSpheres[i].mPosition = position;
		else mMovingCircles[i].mPosition = position;		
		
		bool velicityZero = true;
		auto velocity = Float3{ 0,0,0 };
		while (velicityZero)
		{
			// Set random value for velocity
			xrand = velDistr(gen);
			yrand = velDistr(gen);
			zrand = velDistr(gen);

			if (SPHERES) velocity = Float3{ xrand,yrand,zrand };
			else velocity = Float3{ xrand,yrand,0 };

			if (velocity != Float3{ 0,0,0 }) velicityZero = false;
		}
		if (SPHERES) mMovingSpheres[i].mVelocity = velocity;
		else  mMovingCircles[i].mVelocity = velocity;

		if (SPHERES)
		{
			if (RANDOM_RADIUS) mMovingSpheres[i].mRadius = radDistr(gen);
			else mMovingSpheres[i].mRadius = 10;

			mMovingSpheres[i].mColour = { 1, 0, 1 };
			mMovingSpheres[i].mHealth = 100;
		}
		else
		{
			if (RANDOM_RADIUS) mMovingCircles[i].mRadius = radDistr(gen);
			else mMovingCircles[i].mRadius = 10;

			mMovingCircles[i].mColour = { 1, 0, 1 };
			mMovingCircles[i].mHealth = 100;
		}
		//usedPositions.push_back(position);
	}

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		if (SPHERES) mStillSpheres[i].mName = "Still Sphere " + std::to_string(i);
		else mStillCircles[i].mName = "Still Circle " + std::to_string(i);

		bool posChecked = false;
		bool isSame = false;
		
		position = Float3{ 0,0,0 };

		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		float zrand = posDistr(gen);
		if (SPHERES) position = Float3{ xrand, yrand, zrand };
		else position = Float3{ xrand, yrand, 0 };

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

		if (SPHERES)
		{
			mStillSpheres[i].mPosition = position;
			mStillSpheres[i].mVelocity = { 0,0,0 };

			if (RANDOM_RADIUS) mStillSpheres[i].mRadius = radDistr(gen);
			else mStillSpheres[i].mRadius = 10;

			mStillSpheres[i].mColour = { 1, 0.8, 0 };

			mStillSpheres[i].mHealth = 100;
		}
		else
		{
			mStillCircles[i].mPosition = position;
			mStillCircles[i].mVelocity = { 0,0,0 };

			if (RANDOM_RADIUS) mStillCircles[i].mRadius = radDistr(gen);
			else mStillCircles[i].mRadius = 10;

			mStillCircles[i].mColour = { 1, 0.8, 0 };

			mStillCircles[i].mHealth = 100;
		}

		//usedPositions.push_back(position);
	}

	if(SPHERES) SortCirclesByX(mStillSpheres, NUM_CIRCLES / 2);
	else SortCirclesByX(mStillCircles, NUM_CIRCLES / 2);

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
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete[] mMovingSpheres;
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete[] mStillSpheres;
}

template<class T>
void Circles::BlockCirclesLS(T* movingCircles, uint32_t numMovingCircles, T* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions)
{
	T* movingEnd = movingCircles + numMovingCircles - 1;

	const T* stillEnd = stillCircles + numStillCircles - 1;
	while (movingCircles != movingEnd)
	{
		auto x1 = movingCircles->mPosition.x;
		auto y1 = movingCircles->mPosition.y;
		auto z1 = movingCircles->mPosition.z;

		if (CIRCLE_DEATH) 
		{
			if (movingCircles->mHealth <= 0) { movingCircles++; continue; }
		} 

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
		T* mid;
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
				if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + blocker->mRadius) * (movingCircles->mRadius + blocker->mRadius);

				if (CIRCLE_DEATH)
				{
					if (blocker->mHealth <= 0) { blocker++; continue; }
				}

				auto x2 = blocker->mPosition.x;
				auto y2 = blocker->mPosition.y;
				auto z2 = 0.0f;
				if (SPHERES) z2 = blocker->mPosition.z;

				auto distanceBetweenCirclesSquared = 0.0f;
				if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));					
				else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

				if (distanceBetweenCirclesSquared <= minColDistance)
				{
					if(movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
					if(blocker->mHealth - 20 >= 0) blocker->mHealth -= 20;

					if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, blocker->mName, movingCircles->mHealth, blocker->mHealth, mTimer->GetTime()});

					// v - 2 * (v . n) * n
					movingCircles->Reflect(blocker);
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

				if (CIRCLE_DEATH)
				{
					if (blocker->mHealth <= 0) { blocker--; continue; }
				}

				auto x2 = blocker->mPosition.x;
				auto y2 = blocker->mPosition.y;
				auto z2 = 0.0f;
				if (SPHERES) z2 = blocker->mPosition.z;

				auto distanceBetweenCirclesSquared = 0.0f;
				if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));
				else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
				if (distanceBetweenCirclesSquared <= minColDistance)
				{
					if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
					if (blocker->mHealth - 20 >= 0) blocker->mHealth -= 20;

					if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, blocker->mName, movingCircles->mHealth, blocker->mHealth, mTimer->GetTime() });

					// v - 2 * (v . n) * n
					movingCircles->Reflect(blocker);
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
		bool wallFrontCollision = false;
		bool wallBackCollision = false;

		if		(x1 >= maxPos) { wallRightCollision = true; wallCollision = true; }
		else if (y1 >= maxPos) { wallUpCollision = true; wallCollision = true; }
		else if (x1 <= minPos) { wallLeftCollision = true; wallCollision = true; }
		else if (y1 <= minPos) { wallDownCollision = true; wallCollision = true; }
		else if (z1 >= maxPos) { wallBackCollision = true; wallCollision = true; }
		else if (z1 <= minPos) { wallFrontCollision = true; wallCollision = true; }

		if (wallCollision)
		{
			int x2 = 0;
			int y2 = 0;
			int z2 = 0;
			if		(wallLeftCollision) { x2 = minPos;  y2 = y1; z2 = z1; }
			else if (wallRightCollision) { x2 = maxPos; y2 = y1; z2 = z1;}
			else if (wallUpCollision) { x2 = x1; y2 = maxPos; z2 = z1;}
			else if (wallDownCollision) { x2 = x1; y2 = minPos; z2 = z1;}
			else if (wallFrontCollision) { x2 = x1; y2 = y1; z2 = minPos; }
			else if (wallBackCollision) { x2 = x1; y2 = y1; z2 = maxPos; }

			// Calculate the normal vector pointing from c1 to c2
			auto distance = DistanceWall(x1, y1, z1, x2, y2, z2);
			float nx = (x2 - x1) / distance;
			float ny = (y2 - y1) / distance;
			float nz = (z2 - z1) / distance; 
			// v - 2 * (v . n) * n
			ReflectWall(movingCircles->mVelocity.x, movingCircles->mVelocity.y, movingCircles->mVelocity.z, nx, ny, nz);
			movingCircles->mPosition += Float3{ nx,ny,nz };
			++movingCircles;
			continue;
		}

		if (!MOVING_MOVING)
		{
			++movingCircles;
			continue;
		}







		++movingCircles;
	}
}

template<class T>
void Circles::BlockCircles(T* movingCircles, uint32_t numMovingCircles, T* firstMovingCircle, T* stillCircles, uint32_t numStillCircles, std::vector<Collision>& collisions)
{
	T* movingEnd = movingCircles + numMovingCircles - 1;
	T* movingStart = firstMovingCircle;

	const T* stillEnd = stillCircles + numStillCircles - 1;
	while (movingCircles != movingEnd)
	{
		auto x1 = movingCircles->mPosition.x;
		auto y1 = movingCircles->mPosition.y;
		auto z1 = movingCircles->mPosition.z;
		if (CIRCLE_DEATH)
		{
			if (movingCircles->mHealth <= 0)
			{
				movingCircles++;
				continue;
			}
		}

		auto still = stillCircles;
		bool col = false;
		while (still != stillEnd)
		{
			auto minColDistance = 400;
			if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + still->mRadius) * (movingCircles->mRadius + still->mRadius);
			
			if (CIRCLE_DEATH)
			{
				if (still->mHealth <= 0)
				{
					still++;
					continue;
				}
			}
			
			auto x2 = still->mPosition.x;
			auto y2 = still->mPosition.y;
			auto z2 = still->mPosition.z;
			auto distanceBetweenCirclesSquared = 0.0f;
			if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));
			else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

			// Check for collision
			if (distanceBetweenCirclesSquared <= minColDistance)
			{
				// collision detected				
				if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
				if (still->mHealth - 20 >= 0) still->mHealth -= 20;

				if(OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, still->mName, movingCircles->mHealth, still->mHealth, mTimer->GetTime() });

				// v - 2 * (v . n) * n			
				movingCircles->Reflect(still);

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
		bool wallFrontCollision = false;
		bool wallBackCollision = false;

		if (x1 >= maxPos) { wallRightCollision = true; wallCollision = true; }
		else if (y1 >= maxPos) { wallUpCollision = true; wallCollision = true; }
		else if (x1 <= minPos) { wallLeftCollision = true; wallCollision = true; }
		else if (y1 <= minPos) { wallDownCollision = true; wallCollision = true; }
		else if (z1 >= maxPos) { wallBackCollision = true; wallCollision = true; }
		else if (z1 <= minPos) { wallFrontCollision = true; wallCollision = true; }

		if (wallCollision)
		{
			int x2 = 0;
			int y2 = 0;
			int z2 = 0;
			if (wallLeftCollision) { x2 = minPos;  y2 = y1; z2 = z1; }
			else if (wallRightCollision) { x2 = maxPos; y2 = y1; z2 = z1; }
			else if (wallUpCollision) { x2 = x1; y2 = maxPos; z2 = z1; }
			else if (wallDownCollision) { x2 = x1; y2 = minPos; z2 = z1; }
			else if (wallFrontCollision) { x2 = x1; y2 = y1; z2 = minPos; }
			else if (wallBackCollision) { x2 = x1; y2 = y1; z2 = maxPos; }

			// Calculate the normal vector pointing from c1 to c2
			auto distance = DistanceWall(x1, y1, z1, x2, y2, z2);
			float nx = (x2 - x1) / distance;
			float ny = (y2 - y1) / distance;
			float nz = (z2 - z1) / distance;
			// v - 2 * (v . n) * n
			ReflectWall(movingCircles->mVelocity.x, movingCircles->mVelocity.y, movingCircles->mVelocity.z, nx, ny, nz);
			movingCircles->mPosition += Float3{ nx,ny,nz };
			++movingCircles;
			continue;
		}

		if (!MOVING_MOVING)
		{
			++movingCircles;
			continue;
		}

		auto otherMoving = movingStart;
		col = false;
		while (otherMoving != movingEnd)
		{
			if (otherMoving == movingCircles)
			{
				otherMoving++;
				continue;
			}
			auto minColDistance = 400;
			if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + otherMoving->mRadius) * (movingCircles->mRadius + otherMoving->mRadius);

			if (CIRCLE_DEATH)
			{
				if (otherMoving->mHealth <= 0)
				{
					otherMoving++;
					continue;
				}
			}

			auto x2 = otherMoving->mPosition.x;
			auto y2 = otherMoving->mPosition.y;
			auto z2 = otherMoving->mPosition.z;
			auto distanceBetweenCirclesSquared = 0.0f;
			if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));
			else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

			// Check for collision
			if (distanceBetweenCirclesSquared <= minColDistance)
			{
				// collision detected				
				if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
				if (otherMoving->mHealth - 20 >= 0) otherMoving->mHealth -= 20;

				if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, otherMoving->mName, movingCircles->mHealth, otherMoving->mHealth, mTimer->GetTime() });

				// v - 2 * (v . n) * n			
				movingCircles->Reflect(otherMoving);
				col = true;
				break;
			}
			++otherMoving;
		}
		++movingCircles;
	}
}

void Circles::BlockCirclesThread(uint32_t thread)
{
	if (SPHERES)
	{
		WorkerThread& worker = mBlockSpheresWorkers[thread].first;
		BlockCirclesWork<Sphere>& work = mBlockSpheresWorkers[thread].second;

		while (true)
		{
			{
				std::unique_lock<std::mutex> l(worker.lock);
				worker.workReady.wait(l, [&]() { return !work.complete; });
			}
			if(LINE_SWEEP) BlockCirclesLS(work.movingCircles, work.numMovingCircles, work.stillCircles, work.numStillCircles, work.collisions);
			else BlockCircles(work.movingCircles, work.numMovingCircles, work.firstMovingCircle, work.stillCircles, work.numStillCircles, work.collisions);
			{
				std::unique_lock<std::mutex> l(worker.lock);
				work.complete = true;
			}
			worker.workReady.notify_one();
		}
	}
	else
	{
		WorkerThread& worker = mBlockCirclesWorkers[thread].first;
		BlockCirclesWork<Circle>& work = mBlockCirclesWorkers[thread].second;

		while (true)
		{
			{
				std::unique_lock<std::mutex> l(worker.lock);
				worker.workReady.wait(l, [&]() { return !work.complete; });
			}
			if (LINE_SWEEP) BlockCirclesLS(work.movingCircles, work.numMovingCircles, work.stillCircles, work.numStillCircles, work.collisions);
			else BlockCircles(work.movingCircles, work.numMovingCircles, work.firstMovingCircle, work.stillCircles, work.numStillCircles, work.collisions);
			{
				std::unique_lock<std::mutex> l(worker.lock);
				work.complete = true;
			}
			worker.workReady.notify_one();
		}
	}
}

template<class T>
void Circles::SortCirclesByX(T* circles, int numCircles)
{
	if (numCircles <= 1)
		return;

	int pivotIndex = numCircles / 2;
	float pivotX = circles[pivotIndex].mPosition.x;

	T* left = circles;
	T* right = circles + numCircles - 1;

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

		T temp = *left;
		*left = *right;
		*right = temp;
		left++;
		right--;
	}

	SortCirclesByX(circles, right - circles + 1);
	SortCirclesByX(left, circles + numCircles - left);
}