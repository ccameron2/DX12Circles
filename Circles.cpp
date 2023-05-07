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
	}

	if (mThreaded)
	{
		auto movingPositions = &mMovingPositions[0];
		// Distribute the work to the worker threads
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			// Prepare a section of work (basically the parameters to the collision detection function)
			auto& work = mBlockCirclesWorkers[i].second;
			work.movingPositions = movingPositions;
			work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1); // Add one because this main thread will also do some work
			work.stillPositions = &mStillPositions[0];
			work.numStillCircles = NUM_CIRCLES / 2;
			work.startIndex = ((NUM_CIRCLES / 2) / (mNumWorkers + 1)) * i;
			// Flag the work as not yet complete
			auto& workerThread = mBlockCirclesWorkers[i].first;
			{
				// Guard every access to shared variable "work.complete" with a mutex (see BlockSpritesThread comments)
				std::unique_lock<std::mutex> l(workerThread.lock);
				work.complete = false;
			}

			// Notify the worker thread via a condition variable - this will wake the worker thread up
			workerThread.workReady.notify_one();

			// Move to next section of work
			movingPositions += work.numMovingCircles;
		}

		// This main thread will also do one section of the work
		uint32_t numRemainingParticles = (NUM_CIRCLES / 2) - static_cast<uint32_t>(movingPositions - &mMovingPositions[0]);
		BlockCircles(movingPositions, numRemainingParticles, &mStillPositions[0], NUM_CIRCLES / 2, ((NUM_CIRCLES / 2) / (mNumWorkers + 1)) * mNumWorkers);

		// Wait for all the workers to finish
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			auto& workerThread = mBlockCirclesWorkers[i].first;
			auto& work = mBlockCirclesWorkers[i].second;

			// Wait for a signal via a condition variable indicating that the worker is complete
			// See comments in BlockSpritesThread regarding the mutex and the wait method
			std::unique_lock<std::mutex> l(workerThread.lock);
			workerThread.workReady.wait(l, [&]() { return work.complete; });
		}

		// Continues here when *all* workers are complete

		//***************************************************
	}
	else
	{
		BlockCircles(&mMovingPositions[0], NUM_CIRCLES / 2, &mStillPositions[0], NUM_CIRCLES / 2, 0);
	}
}
void Circles::UpdateCircles(float frameTime)
{
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mMovingPositions[i] += mVelocities[i] * frameTime;
	}

	if (mThreaded)
	{
		auto movingPositions = &mMovingPositions[0];
		// Distribute the work to the worker threads
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			// Prepare a section of work (basically the parameters to the collision detection function)
			auto& work = mBlockCirclesWorkers[i].second;
			work.movingPositions = movingPositions;
			work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1); // Add one because this main thread will also do some work
			work.stillPositions = &mStillPositions[0];
			work.numStillCircles = NUM_CIRCLES / 2;
			work.startIndex = ((NUM_CIRCLES / 2) / (mNumWorkers + 1)) * i;
			// Flag the work as not yet complete
			auto& workerThread = mBlockCirclesWorkers[i].first;
			{
				// Guard every access to shared variable "work.complete" with a mutex (see BlockSpritesThread comments)
				std::unique_lock<std::mutex> l(workerThread.lock);
				work.complete = false;
			}

			// Notify the worker thread via a condition variable - this will wake the worker thread up
			workerThread.workReady.notify_one();

			// Move to next section of work
			movingPositions += work.numMovingCircles;
		}

		// This main thread will also do one section of the work
		uint32_t numRemainingParticles = (NUM_CIRCLES / 2) - static_cast<uint32_t>(movingPositions - &mMovingPositions[0]);
		BlockCircles(movingPositions, numRemainingParticles, &mStillPositions[0], NUM_CIRCLES / 2, ((NUM_CIRCLES / 2) / (mNumWorkers + 1)) * mNumWorkers);

		// Wait for all the workers to finish
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			auto& workerThread = mBlockCirclesWorkers[i].first;
			auto& work = mBlockCirclesWorkers[i].second;

			// Wait for a signal via a condition variable indicating that the worker is complete
			// See comments in BlockSpritesThread regarding the mutex and the wait method
			std::unique_lock<std::mutex> l(workerThread.lock);
			workerThread.workReady.wait(l, [&]() { return work.complete; });
		}

		// Continues here when *all* workers are complete

		//***************************************************
	}
	else
	{
		BlockCircles(&mMovingPositions[0], NUM_CIRCLES / 2, &mStillPositions[0], NUM_CIRCLES / 2, 0);
	}
}

Circles::~Circles()
{
	//*********************************************************
	// Running threads must be joined to the main thread or detached before their destruction. If not
	// the entire program immediately terminates (in fact the program is quiting here anyway, but if 
	// std::terminate is called we probably won't get proper destruction). The worker threads never
	// naturally exit so we can't use join. So in this case detach them prior to their destruction.
	for (uint32_t i = 0; i < mNumWorkers; ++i)
	{
		mBlockCirclesWorkers[i].first.thread.detach();
	}
}

void Circles::InitCircles()
{
	if (mThreaded)
	{
		//*********************************************************
		// Start worker threads
		mNumWorkers = std::thread::hardware_concurrency(); // Gives a hint about level of thread concurrency supported by system (0 means no hint given)
		if (mNumWorkers == 0)  mNumWorkers = 8;
		--mNumWorkers; // Decrease by one because this main thread is already running
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			// Start each worker thread running the BlockSpritesThread method. Note the way to construct std::thread to run a member function
			mBlockCirclesWorkers[i].first.thread = std::thread(&Circles::BlockCirclesThread, this, i);
		}
	}
	
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

		position = Float3{ 0,0,0 };

		while (!posChecked)
		{
			// Generate a random position
			float xrand = posDistr(gen);
			float yrand = posDistr(gen);
			position = Float3{ xrand, yrand, 0 };

			// Check if the position has already been used
			auto it = std::find_if(usedPositions.begin(), usedPositions.end(),
				[position](const Float3& p) { return p.x == position.x && p.y == position.y; });
			if (it == usedPositions.end())
			{
				// Position is unique, so use it
				usedPositions.push_back(position);
				posChecked = true;
			}
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
		
		position = Float3{ 0,0,0 };
		posChecked = false;

		position = Float3{ 0,0,0 };

		while (!posChecked)
		{
			// Generate a random position
			float xrand = posDistr(gen);
			float yrand = posDistr(gen);
			position = Float3{ xrand, yrand, 0 };

			// Check if the position has already been used
			auto it = std::find_if(usedPositions.begin(), usedPositions.end(),
				[position](const Float3& p) { return p.x == position.x && p.y == position.y; });
			if (it == usedPositions.end())
			{
				// Position is unique, so use it
				usedPositions.push_back(position);
				posChecked = true;
			}
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

void Circles::BlockCircles(Float3* movingPositions, uint32_t numMovingCircles, Float3* stillPositions, uint32_t numStillCircles, uint32_t startIndex)
{
	auto movingEnd = movingPositions + numMovingCircles - 1;
	auto stillEnd = stillPositions + numStillCircles - 1;
	int movingIndex = startIndex;
	while (movingPositions != movingEnd)
	{
		auto x1 = movingPositions->x;
		auto y1 = movingPositions->y;

		auto still = stillPositions;
		int stillIndex = 0;

		int maxPos = MAX_POS + 100;
		int minPos = MIN_POS - 100;

		bool wallCollision = false;
		bool wallLeftCollision = false;
		bool wallRightCollision = false;
		bool wallUpCollision = false;
		bool wallDownCollision = false;

		if		(x1 >= maxPos){wallRightCollision = true; wallCollision = true;}
		else if (y1 >= maxPos){wallUpCollision = true;	  wallCollision = true;}
		else if (x1 <= minPos){wallLeftCollision = true;  wallCollision = true;}
		else if (y1 <= minPos){wallDownCollision = true;  wallCollision = true; }

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
			Reflect(mVelocities[movingIndex].x, mVelocities[movingIndex].y, nx, ny);

		}


		while (still != stillEnd)
		{
			auto x2 = still->x;
			auto y2 = still->y;
			auto distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));

			// Check for collision
			if (distanceBetweenCirclesSquared <= 400)
			{
				// collision detected
				//mHealths[i] -= 20;
				//mHealths[j] -= 20;

				//mCollisions.push_back(Collision{ movingIndex, stillIndex });

				// Calculate the normal vector pointing from c1 to c2
				auto distance = Distance(x1, y1, x2, y2);
				float nx = (x2 - x1) / distance;
				float ny = (y2 - y1) / distance;

				// v - 2 * (v . n) * n
				Reflect(mVelocities[movingIndex].x, mVelocities[movingIndex].y, nx, ny);
			}
			stillIndex++;
			still++;
		}		
		movingIndex++;
		movingPositions++;
	}
}

void Circles::BlockCirclesThread(uint32_t thread)
{
	auto& worker = mBlockCirclesWorkers[thread].first;
	auto& work = mBlockCirclesWorkers[thread].second;
	while (true)
	{
		// In this system we need to communicate between the main thread and these worker threads.
		// We need to indicate when work is ready, then reply when it is complete. We use "condition
		// variables" for this. They are a method of receiving notifications between threads.
		// Condition variables reqiuire a mutex to be held when using them. It protects the shared
		// data that is used in the predicate ("work.complete" here). We don't want the other thread
		// changing work.complete while we are reading it. 
		// In practice, when we call "wait" the mutex is immediately released while waiting for the
		// signal (workReady), it is aquired again when a signal arrives and the predicate is checked
		// (work.complete). After wait finishes the mutex is still held. That is why there are curly
		// brackets here to release the mutex (unique_lock works like unique_ptr, it releases the
		// mutex when it goes out of scope). This system is tricky to fully appreciate at first.
		{
			std::unique_lock<std::mutex> l(worker.lock);
			worker.workReady.wait(l, [&]() { return !work.complete; }); // Wait until a workReady signal arrives, then verify it by testing      
			// that work.complete is false. The test is required because there is
			// the possibility of "spurious wakeups": a false signal. Also in 
			// some situations other threads may have eaten the work already.
		}
		// We have some work so do it...
		BlockCircles(work.movingPositions, work.numMovingCircles, work.stillPositions, work.numStillCircles,work.startIndex);
		{
			// Flag the work is complete
			// We also guard every normal access to shared variable "work.complete" with the same mutex
			std::unique_lock<std::mutex> l(worker.lock);
			work.complete = true;
		}
		// Send a signal back to the main thread to say the work is complete, loop back and wait for more work
		worker.workReady.notify_one();
	}
}
