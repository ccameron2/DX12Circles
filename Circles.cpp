#include "Circles.h"
#include <random>
#include <iostream>

// Calculate the distance between two 3D points for wall collision
float DistanceWall(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
}

// Calculate the dot product of two 3D points for wall collision
float DotProduct(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return x1 * x2 + y1 * y2 + z1 * z2;
}

// Reflect a vector around another vector for wall collision
void ReflectWall(float& x, float& y, float& z, float nx, float ny, float nz)
{
	float dot = DotProduct(x, y, z, nx, ny, nz);
	x = x - 2 * dot * nx;
	y = y - 2 * dot * ny;
	z = z - 2 * dot * nz;
}

// Update circles for this frame
void Circles::UpdateCircles(float frameTime)
{
	// If frame time is being used, multiply by 100 to prevent very slow circles
	if (frameTime != 1) frameTime *= 100;

	//  For each circle, add velocity to position
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{		
		if(SPHERES) mMovingSpheres[i].mPosition += mMovingSpheres[i].mVelocity * frameTime;
		else mMovingCircles[i].mPosition += mMovingCircles[i].mVelocity * frameTime;
	}

	// If line sweep and moving-moving are enabled
	if (LINE_SWEEP && MOVING_MOVING)
	{
		// Sort the circles
		if(SPHERES) SortCirclesByX(mMovingSpheres, NUM_CIRCLES / 2);
		else SortCirclesByX(mMovingCircles, NUM_CIRCLES / 2);
	}

	// If threading enabled
	if (THREADED)
	{
		// Get array of circles
		auto movingCircles = &mMovingCircles[0];
		auto movingSpheres = &mMovingSpheres[0];

		// If spheres are enabled
		if (SPHERES)
		{
			// For each worker
			for (uint32_t i = 0; i < mNumWorkers; ++i)
			{
				// Set up data
				auto& work = mBlockSpheresWorkers[i].second;
				work.movingCircles = movingSpheres;
				work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1);
				work.firstMovingCircle = &mMovingSpheres[0];
				work.stillCircles = &mStillSpheres[0];
				work.numStillCircles = NUM_CIRCLES / 2;
				work.collisions.clear();

				// Mutex work complete
				auto& workerThread = mBlockSpheresWorkers[i].first;
				{
					std::unique_lock<std::mutex> l(workerThread.lock);
					work.complete = false;
				}

				// Notify worker
				workerThread.workReady.notify_one();

				// Increment through moving circles list
				movingSpheres += work.numMovingCircles;
			}

			// Set main thread up to also do work
			uint32_t numRemainingParticles = (NUM_CIRCLES / 2) - static_cast<uint32_t>(movingSpheres - &mMovingSpheres[0]);
			if (LINE_SWEEP) BlockCirclesLS(movingSpheres, numRemainingParticles, &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(movingSpheres, numRemainingParticles, &mMovingSpheres[0], &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);

			// For each worker
			for (uint32_t i = 0; i < mNumWorkers; ++i)
			{
				// Get worker and work
				auto& workerThread = mBlockSpheresWorkers[i].first;
				auto& work = mBlockSpheresWorkers[i].second;

				// Mutex work complete
				std::unique_lock<std::mutex> l(workerThread.lock);
				workerThread.workReady.wait(l, [&]() { return work.complete; });

				// If output collisions enabled
				if(OUTPUT_COLLISIONS)
				{
					// Collect collisions from threads
					for (auto& collision : work.collisions)
					{
						mCollisions.push_back(collision);
					}
				}
			}
		}
		else
		{
			// For each worker
			for (uint32_t i = 0; i < mNumWorkers; ++i)
			{
				// Set up data
				auto& work = mBlockCirclesWorkers[i].second;
				work.movingCircles = movingCircles;
				work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1);
				work.firstMovingCircle = &mMovingCircles[0];
				work.stillCircles = &mStillCircles[0];
				work.numStillCircles = NUM_CIRCLES / 2;
				work.collisions.clear();

				// Mutex work complete
				auto& workerThread = mBlockCirclesWorkers[i].first;
				{
					std::unique_lock<std::mutex> l(workerThread.lock);
					work.complete = false;
				}

				// Notify worker
				workerThread.workReady.notify_one();

				// Move through circle array
				movingCircles += work.numMovingCircles;
			}

			// Set up the main thread to also do collision
			uint32_t numRemainingParticles = (NUM_CIRCLES / 2) - static_cast<uint32_t>(movingCircles - &mMovingCircles[0]);
			if(LINE_SWEEP) BlockCirclesLS(movingCircles, numRemainingParticles, &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(movingCircles, numRemainingParticles, &mMovingCircles[0], &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
			
			// For each worker
			for (uint32_t i = 0; i < mNumWorkers; ++i)
			{
				// Get worker and work
				auto& workerThread = mBlockCirclesWorkers[i].first;
				auto& work = mBlockCirclesWorkers[i].second;

				// Mutex thread complete
				std::unique_lock<std::mutex> l(workerThread.lock);
				workerThread.workReady.wait(l, [&]() { return work.complete; });
				// If output collisions enabled
				if (OUTPUT_COLLISIONS)				
				{
					// Collect collisions from threads
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
		// If spheres enabled
		if (SPHERES) 
		{
			// Run either line sweep or brute force on all spheres on main thread
			if(LINE_SWEEP) BlockCirclesLS(&mMovingSpheres[0], NUM_CIRCLES / 2, &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(&mMovingSpheres[0], NUM_CIRCLES / 2, &mMovingSpheres[0], &mStillSpheres[0], NUM_CIRCLES / 2, mCollisions);
		} 
		else
		{
			// Run either line sweep or brute force on all circles on main thread
			if (LINE_SWEEP) BlockCirclesLS(&mMovingCircles[0], NUM_CIRCLES / 2, &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
			else BlockCircles(&mMovingCircles[0], NUM_CIRCLES / 2, &mMovingCircles[0], &mStillCircles[0], NUM_CIRCLES / 2, mCollisions);
		}
	}
}

Circles::~Circles()
{
	// If spheres are enabled 
	if (SPHERES)
	{
		// Detach sphere workers
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			mBlockSpheresWorkers[i].first.thread.detach();
		}
	}
	else
	{
		// Detach circle workers
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			mBlockCirclesWorkers[i].first.thread.detach();
		}
	}
	
	// Delete circles
	ClearMemory();
}

void Circles::InitCircles()
{
	// If threaded enabled
	if (THREADED)
	{
		// Get number of threads available
		mNumWorkers = std::thread::hardware_concurrency();
		if (mNumWorkers == 0)  mNumWorkers = 8; // Set to default value if no result
		--mNumWorkers;

		// For each worker, start waiting for notify signal
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			// Start the spheres worker or the circles worker
			if(SPHERES) mBlockSpheresWorkers[i].first.thread = std::thread(&Circles::BlockCirclesThread, this, i);
			else mBlockCirclesWorkers[i].first.thread = std::thread(&Circles::BlockCirclesThread, this, i);
		}
	}
	
	// Set up random generators
	std::random_device rd; // Obtain a random number from hardware
	std::mt19937 gen(rd()); // Seed the generator
	std::uniform_int_distribution<> velDistr(MIN_VEL, MAX_VEL); // Define the range
	std::uniform_int_distribution<> posDistr(MIN_POS, MAX_POS); 
	std::uniform_int_distribution<> radDistr(MIN_RAD, MAX_RAD); 
	
	// Create new spheres or circles
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

	// Set each moving circle's data
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		// Set name depending on if spheres are enabled
		if(SPHERES) mMovingSpheres[i].mName = "Moving Sphere " + std::to_string(i);
		else mMovingCircles[i].mName = "Moving Circle " + std::to_string(i);

		bool posChecked = false;
		bool isSame = false;

		// Get random position
		position = Float3{ 0,0,0 };
		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		float zrand = posDistr(gen);

		// Dont use Z coordinate when spheres disabled
		if (SPHERES) position = Float3{ xrand, yrand, zrand };
		else position = Float3{ xrand, yrand, 0 };

		// Set position
		if (SPHERES) mMovingSpheres[i].mPosition = position;
		else mMovingCircles[i].mPosition = position;		
		
		// Get random value for velocity that is not 0;
		bool velicityZero = true;
		auto velocity = Float3{ 0,0,0 };
		while (velicityZero)
		{
			// Set random value for velocity
			xrand = velDistr(gen);
			yrand = velDistr(gen);
			zrand = velDistr(gen);

			// Dont use z value if spheres disabled
			if (SPHERES) velocity = Float3{ xrand,yrand,zrand };
			else velocity = Float3{ xrand,yrand,0 };

			if (velocity != Float3{ 0,0,0 }) velicityZero = false;
		}

		// Set velocity
		if (SPHERES) mMovingSpheres[i].mVelocity = velocity;
		else  mMovingCircles[i].mVelocity = velocity;

		// Set remaining data for circles or spheres
		if (SPHERES)
		{
			// Set sphere radius to random or fixed value
			if (RANDOM_RADIUS) mMovingSpheres[i].mRadius = radDistr(gen);
			else mMovingSpheres[i].mRadius = 10;

			// Set colour and health
			mMovingSpheres[i].mColour = { 1, 0, 1 };
			mMovingSpheres[i].mHealth = 100;
		}
		else
		{
			// Set circle radius to random or fixed value
			if (RANDOM_RADIUS) mMovingCircles[i].mRadius = radDistr(gen);
			else mMovingCircles[i].mRadius = 10;
			
			// Set colour and health
			mMovingCircles[i].mColour = { 1, 0, 1 };
			mMovingCircles[i].mHealth = 100;
		}
	}

	// Set data for each still circle
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		// Set name of sphere or circle
		if (SPHERES) mStillSpheres[i].mName = "Still Sphere " + std::to_string(i);
		else mStillCircles[i].mName = "Still Circle " + std::to_string(i);

		bool posChecked = false;
		bool isSame = false;
		position = Float3{ 0,0,0 };

		// Get random value for position
		float xrand = posDistr(gen);
		float yrand = posDistr(gen);
		float zrand = posDistr(gen);

		// Use z value if spheres enabled
		if (SPHERES) position = Float3{ xrand, yrand, zrand };
		else position = Float3{ xrand, yrand, 0 };

		// Set remaining data
		if (SPHERES)
		{
			// Set position and velocity
			mStillSpheres[i].mPosition = position;
			mStillSpheres[i].mVelocity = { 0,0,0 };

			// Set radius to random or to fixed value
			if (RANDOM_RADIUS) mStillSpheres[i].mRadius = radDistr(gen);
			else mStillSpheres[i].mRadius = 10;

			// Set colour and health
			mStillSpheres[i].mColour = { 1, 0.8, 0 };
			mStillSpheres[i].mHealth = 100;
		}
		else
		{
			// Set position and velocity 
			mStillCircles[i].mPosition = position;
			mStillCircles[i].mVelocity = { 0,0,0 };

			// Set radius to random or fixed value
			if (RANDOM_RADIUS) mStillCircles[i].mRadius = radDistr(gen);
			else mStillCircles[i].mRadius = 10;

			// Set colour and health
			mStillCircles[i].mColour = { 1, 0.8, 0 };
			mStillCircles[i].mHealth = 100;
		}
	}

	// Sort still circles or spheres by x coordinate
	if(SPHERES) SortCirclesByX(mStillSpheres, NUM_CIRCLES / 2);
	else SortCirclesByX(mStillCircles, NUM_CIRCLES / 2);

	// Start new timer
	mTimer = new Timer();
	mTimer->Start();
}

void Circles::OutputFrame()
{
	// Output frame-time
	std::cout << std::endl << "FrameTime: " << mTimer->GetLapTime() << std::endl;

	if (!OUTPUT_COLLISIONS) return;

	// Output each collision
	for (auto& collision : mCollisions)
	{
		auto name1 = collision.Circle1Name;
		auto name2 = collision.Circle2Name;
		auto health1 = collision.Circle1Health;
		auto health2 = collision.Circle2Health;
		auto time = collision.TimeOfCollision;

		std::cout << name1 << " with " << health1 << " health remaining, collided with " << name2 << " with " << health2 << " health remaining, at " << time << "." << std::endl;
	}
	// Clear collisions for next frame
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
	// Get end of moving circles to check
	T* movingEnd = movingCircles + numMovingCircles - 1;

	// Get end of still circles
	const T* stillEnd = stillCircles + numStillCircles - 1;

	// While not at end of moving circles
	while (movingCircles != movingEnd)
	{
		// Extract position
		auto x1 = movingCircles->mPosition.x;
		auto y1 = movingCircles->mPosition.y;
		auto z1 = movingCircles->mPosition.z;

		// If circle death enabled, check health
		if (CIRCLE_DEATH) 
		{
			// Dont contribute to collision if dead
			if (movingCircles->mHealth <= 0) { movingCircles++; continue; }
		} 

		// Get first still circle
		auto still = stillCircles;
		bool col = false;

		// Left side of line sweep
		int mLSweepSide = 15;
		// Right side of line sweep
		int mRSweepSide = 15;

		// If random radius enabled
		if (RANDOM_RADIUS)
		{
			// Set sides to use circle's radius instead
			mLSweepSide = movingCircles->mRadius + 5;
			mRSweepSide = movingCircles->mRadius + 5;
		}


		auto start = still;
		auto end = stillEnd;
		T* mid;
		bool found = false;
		// Search for still circles with binary search to find overlapping x-range
		do
		{
			mid = start + (end - start) / 2;
			if (movingCircles->mPosition.x + mRSweepSide <= mid->mPosition.x)  end = mid;
			else if (movingCircles->mPosition.x - mLSweepSide >= mid->mPosition.x)  start = mid;
			else found = true;
		} while (!found && end - start > 1);

		// If overlap found
		if (found)
		{
			// Search from circle found rightwards 
			auto blocker = mid;
			while (movingCircles->mPosition.x + mRSweepSide > blocker->mPosition.x && blocker != stillEnd)
			{
				// Either use static variable or calculate using circle radii
				auto minColDistance = 400;
				if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + blocker->mRadius) * (movingCircles->mRadius + blocker->mRadius);

				// If circle death enabled
				if (CIRCLE_DEATH)
				{
					// Check circle health and dont contribute to collisions if dead
					if (blocker->mHealth <= 0) { blocker++; continue; }
				}

				// Extract positions
				auto x2 = blocker->mPosition.x;
				auto y2 = blocker->mPosition.y;
				// Dont extract z if spheres not enabled
				auto z2 = 0.0f;
				if (SPHERES) z2 = blocker->mPosition.z;

				// Calculate distance between circles or spheres
				auto distanceBetweenCirclesSquared = 0.0f;
				if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));					
				else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

				// If distance is less than collision distance
				if (distanceBetweenCirclesSquared <= minColDistance)
				{
					// Reduce health of both circles
					if(movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
					if(blocker->mHealth - 20 >= 0) blocker->mHealth -= 20;

					// Record collision if enabled
					if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, blocker->mName, movingCircles->mHealth, blocker->mHealth, mTimer->GetTime()});

					// Reflect moving circle around blocker
					// v - 2 * (v . n) * n
					movingCircles->Reflect(blocker);

					// Set collision to true
					col = true;
					break;
				}
				// Move to next circle
				++blocker;
			}

			// If a collision happened
			if (col)
			{
				// Move to next moving circle
				++movingCircles;
				continue;
			}

			// Search from circle found leftwards
			blocker = mid;
			while (blocker-- != stillCircles && movingCircles->mPosition.x - mLSweepSide < blocker->mPosition.x)
			{
				// Either use static variable or calculate using circle radii
				auto minColDistance = 400;
				if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + blocker->mRadius) * (movingCircles->mRadius + blocker->mRadius);
				
				// If circle death enabled
				if (CIRCLE_DEATH)
				{
					// Check circle health and dont contribute to collisions if dead
					if (blocker->mHealth <= 0) { blocker--; continue; }
				}

				// Extract positions
				auto x2 = blocker->mPosition.x;
				auto y2 = blocker->mPosition.y;

				// Dont extract z if spheres not enabled
				auto z2 = 0.0f;
				if (SPHERES) z2 = blocker->mPosition.z;

				// Calculate distance between circles or spheres
				auto distanceBetweenCirclesSquared = 0.0f;
				if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));
				else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

				// If distance is less than collision distance
				if (distanceBetweenCirclesSquared <= minColDistance)
				{					
					// Reduce health of both circles
					if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
					if (blocker->mHealth - 20 >= 0) blocker->mHealth -= 20;

					// Record collision if enabled
					if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, blocker->mName, movingCircles->mHealth, blocker->mHealth, mTimer->GetTime() });
					
					// Reflect moving circle around blocker
					// v - 2 * (v . n) * n
					movingCircles->Reflect(blocker);

					// Set collision to true
					col = true;
					break;
				}
			}
			// If a collision happened
			if (col)
			{
				// Move to next moving circle
				++movingCircles;
				continue;
			}
		}

		// If walls not enabled move on early
		if (!WALLS) 
		{
			++movingCircles;
			continue;
		} 

		// Get max and min pos from constants
		int maxPos = MAX_POS + WALL_DISTANCE_FROM_EDGE;
		int minPos = MIN_POS - WALL_DISTANCE_FROM_EDGE;

		// Wall collision bools
		bool wallCollision = false;
		bool wallLeftCollision = false;
		bool wallRightCollision = false;
		bool wallUpCollision = false;
		bool wallDownCollision = false;
		bool wallFrontCollision = false;
		bool wallBackCollision = false;

		// Check circle position against walls
		if		(x1 >= maxPos) { wallRightCollision = true; wallCollision = true; }
		else if (y1 >= maxPos) { wallUpCollision = true; wallCollision = true; }
		else if (x1 <= minPos) { wallLeftCollision = true; wallCollision = true; }
		else if (y1 <= minPos) { wallDownCollision = true; wallCollision = true; }
		else if (z1 >= maxPos) { wallBackCollision = true; wallCollision = true; }
		else if (z1 <= minPos) { wallFrontCollision = true; wallCollision = true; }

		// If wall collision
		if (wallCollision)
		{
			int x2 = 0;
			int y2 = 0;
			int z2 = 0;

			// Get position of the part of wall hit
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

			// Reflect from point on the wall
			// v - 2 * (v . n) * n
			ReflectWall(movingCircles->mVelocity.x, movingCircles->mVelocity.y, movingCircles->mVelocity.z, nx, ny, nz);

			// Adjust position along normal
			movingCircles->mPosition += Float3{ nx,ny,nz };

			// Move to next moving circle
			++movingCircles;
			continue;
		}

		// Move to next moving circle if Moving-Moving disabled
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
	// Get first moving circle to check
	T* movingEnd = movingCircles + numMovingCircles - 1;
	
	// Get start and end of moving circles list
	T* movingStart = firstMovingCircle;
	T* endOfMoving = movingStart + (NUM_CIRCLES / 2) - 1;

	// Get the end of the still circles list
	const T* stillEnd = stillCircles + numStillCircles - 1;

	// While not at the end of circles to check
	while (movingCircles != movingEnd)
	{
		// Extract positions
		auto x1 = movingCircles->mPosition.x;
		auto y1 = movingCircles->mPosition.y;
		auto z1 = movingCircles->mPosition.z;

		// If circle death enabled
		if (CIRCLE_DEATH)
		{
			// Check health
			if (movingCircles->mHealth <= 0)
			{
				// Move onto next circle if dead
				movingCircles++;
				continue;
			}
		}

		auto still = stillCircles;
		bool col = false;

		// While not at the end of still circles
		while (still != stillEnd)
		{
			// Precalculate or dynamically calculate min collision distance between still and moving circle
			auto minColDistance = 400;
			if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + still->mRadius) * (movingCircles->mRadius + still->mRadius);
			
			// If death enabled
			if (CIRCLE_DEATH)
			{
				// Check health
				if (still->mHealth <= 0)
				{
					// Move onto next still circle
					still++;
					continue;
				}
			}

			// Extract positions
			auto x2 = still->mPosition.x;
			auto y2 = still->mPosition.y;
			auto z2 = still->mPosition.z;

			// Calculate distance between circles or spheres
			auto distanceBetweenCirclesSquared = 0.0f;
			if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));
			else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

			// Check for collision
			if (distanceBetweenCirclesSquared <= minColDistance)
			{
				// Reduce health of both circles
				if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
				if (still->mHealth - 20 >= 0) still->mHealth -= 20;

				// Record collision if enabled
				if(OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, still->mName, movingCircles->mHealth, still->mHealth, mTimer->GetTime() });
				
				// Reflect moving circle around blocker
				// v - 2 * (v . n) * n			
				movingCircles->Reflect(still);

				// Set collision to true
				col = true;
				break;
			}
			++still;
		}

		// If a collision happened
		if (col)
		{				
			// Move to next moving circle
			++movingCircles;
			continue;
		}

		// If walls not enabled move on early
		if (!WALLS)
		{
			++movingCircles;
			continue;
		}

		// Get max and min pos from constants
		int maxPos = MAX_POS + WALL_DISTANCE_FROM_EDGE;
		int minPos = MIN_POS - WALL_DISTANCE_FROM_EDGE;

		// Wall collision bools
		bool wallCollision = false;
		bool wallLeftCollision = false;
		bool wallRightCollision = false;
		bool wallUpCollision = false;
		bool wallDownCollision = false;
		bool wallFrontCollision = false;
		bool wallBackCollision = false;

		// Check circle position against walls
		if (x1 >= maxPos) { wallRightCollision = true; wallCollision = true; }
		else if (y1 >= maxPos) { wallUpCollision = true; wallCollision = true; }
		else if (x1 <= minPos) { wallLeftCollision = true; wallCollision = true; }
		else if (y1 <= minPos) { wallDownCollision = true; wallCollision = true; }
		else if (z1 >= maxPos) { wallBackCollision = true; wallCollision = true; }
		else if (z1 <= minPos) { wallFrontCollision = true; wallCollision = true; }

		// If wall collision
		if (wallCollision)
		{
			int x2 = 0;
			int y2 = 0;
			int z2 = 0;

			// Get position of the part of wall hit
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

			// Reflect from position on wall hit
			// v - 2 * (v . n) * n
			ReflectWall(movingCircles->mVelocity.x, movingCircles->mVelocity.y, movingCircles->mVelocity.z, nx, ny, nz);

			// Adjust position along normal
			movingCircles->mPosition += Float3{ nx,ny,nz };

			// Move to next moving circle
			++movingCircles;
			continue;
		}

		// If moving-moving disabled
		if (!MOVING_MOVING)
		{
			// Move to next moving circle
			++movingCircles;
			continue;
		}

		// Get start of moving list
		auto otherMoving = movingStart;
		col = false;

		// While not at the end of moving list
		while (otherMoving != endOfMoving)
		{
			// If checking this circle
			if (otherMoving == movingCircles)
			{
				// Move on
				otherMoving++;
				continue;
			}

			// Calculate or use pre calculated min distance for collision
			auto minColDistance = 400;
			if (RANDOM_RADIUS) minColDistance = (movingCircles->mRadius + otherMoving->mRadius) * (movingCircles->mRadius + otherMoving->mRadius);

			// If circle death enabled
			if (CIRCLE_DEATH)
			{
				// Check health
				if (otherMoving->mHealth <= 0)
				{
					// Dont contribute to collision
					otherMoving++;
					continue;
				}
			}

			// Extract positions
			auto x2 = otherMoving->mPosition.x;
			auto y2 = otherMoving->mPosition.y;
			auto z2 = otherMoving->mPosition.z;

			// Calculate distance between circles
			auto distanceBetweenCirclesSquared = 0.0f;
			if (SPHERES) distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));
			else distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

			// Check for collision
			if (distanceBetweenCirclesSquared <= minColDistance)
			{
				// Reduce circle healths			
				if (movingCircles->mHealth - 20 >= 0) movingCircles->mHealth -= 20;
				if (otherMoving->mHealth - 20 >= 0) otherMoving->mHealth -= 20;

				// Record collision
				if (OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingCircles->mName, otherMoving->mName, movingCircles->mHealth, otherMoving->mHealth, mTimer->GetTime() });

				// Reflect around moving circle
				// v - 2 * (v . n) * n			
				movingCircles->Reflect(otherMoving);
				col = true;
				break;
			}
			// Move to next other moving circle
			++otherMoving;
		}
		// Move to next moving circle
		++movingCircles;
	}
}

void Circles::BlockCirclesThread(uint32_t thread)
{
	// Start sphere workers if spheres enabled
	if (SPHERES)
	{
		// Get worker and work
		WorkerThread& worker = mBlockSpheresWorkers[thread].first;
		BlockCirclesWork<Sphere>& work = mBlockSpheresWorkers[thread].second;

		// Loop constantly
		while (true)
		{
			// Mutex work complete
			{
				std::unique_lock<std::mutex> l(worker.lock);
				worker.workReady.wait(l, [&]() { return !work.complete; });
			}

			// Run correct collision function
			if(LINE_SWEEP) BlockCirclesLS(work.movingCircles, work.numMovingCircles, work.stillCircles, work.numStillCircles, work.collisions);
			else BlockCircles(work.movingCircles, work.numMovingCircles, work.firstMovingCircle, work.stillCircles, work.numStillCircles, work.collisions);
			
			// Mutex work complete
			{
				std::unique_lock<std::mutex> l(worker.lock);
				work.complete = true;
			}

			// Notify worker
			worker.workReady.notify_one();
		}
	}
	else // Start circle workers
	{
		WorkerThread& worker = mBlockCirclesWorkers[thread].first;
		BlockCirclesWork<Circle>& work = mBlockCirclesWorkers[thread].second;

		// Loop constantly
		while (true)
		{
			// Mutex work complete
			{
				std::unique_lock<std::mutex> l(worker.lock);
				worker.workReady.wait(l, [&]() { return !work.complete; });
			}
			
			// Run correct collision function
			if (LINE_SWEEP) BlockCirclesLS(work.movingCircles, work.numMovingCircles, work.stillCircles, work.numStillCircles, work.collisions);
			else BlockCircles(work.movingCircles, work.numMovingCircles, work.firstMovingCircle, work.stillCircles, work.numStillCircles, work.collisions);

			// Mutex work complete
			{
				std::unique_lock<std::mutex> l(worker.lock);
				work.complete = true;
			}

			// Notify worker
			worker.workReady.notify_one();
		}
	}
}

// Sort circles by x coordinate
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