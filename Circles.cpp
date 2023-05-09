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
		mMovingCircles[i].mPosition += mMovingCircles[i].mVelocity * frameTime;
	}

	if (THREADED)
	{
		auto movingCircles = &mMovingCircles[0];
		// Distribute the work to the worker threads
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			// Prepare a section of work (basically the parameters to the collision detection function)
			auto& work = mBlockCirclesWorkers[i].second;
			work.movingCircles = movingCircles;
			work.numMovingCircles = (NUM_CIRCLES / 2) / (mNumWorkers + 1); // Add one because this main thread will also do some work
			work.stillCircles = &mStillCircles[0];
			work.numStillCircles = NUM_CIRCLES / 2;
			work.startIndex = ((NUM_CIRCLES / 2) / (mNumWorkers + 1)) * i;
			work.collisions.clear();

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
			movingCircles += work.numMovingCircles;
		}

		// This main thread will also do one section of the work
		uint32_t numRemainingParticles = (NUM_CIRCLES / 2) - static_cast<uint32_t>(movingCircles - &mMovingCircles[0]);
		BlockCircles(movingCircles, numRemainingParticles, &mStillCircles[0], NUM_CIRCLES / 2, ((NUM_CIRCLES / 2) / (mNumWorkers + 1)) * mNumWorkers, mCollisions);

		// Wait for all the workers to finish
		for (uint32_t i = 0; i < mNumWorkers; ++i)
		{
			auto& workerThread = mBlockCirclesWorkers[i].first;
			auto& work = mBlockCirclesWorkers[i].second;

			// Wait for a signal via a condition variable indicating that the worker is complete
			// See comments in BlockSpritesThread regarding the mutex and the wait method
			std::unique_lock<std::mutex> l(workerThread.lock);
			workerThread.workReady.wait(l, [&]() { return work.complete; });

			
			{
				for (auto& collision : work.collisions)
				{
					mCollisions.push_back(collision);
				}
			}					
		}	

		// Continues here when *all* workers are complete

		//***************************************************
	}
	else
	{
		BlockCircles(&mMovingCircles[0], NUM_CIRCLES / 2, &mStillCircles[0], NUM_CIRCLES / 2, 0, mCollisions);
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
	if (THREADED)
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
	
	auto movingCircles = new Circle[NUM_CIRCLES / 2];
	auto stillCircles = new Circle[NUM_CIRCLES / 2];
	mMovingCircles = new Circle[NUM_CIRCLES / 2];
	mStillCircles = new Circle[NUM_CIRCLES / 2];
	Float3 position;
	float xrand;
	float yrand;
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mMovingCircles[i].mIndex = i;
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

		//circle->mRadius = 10;

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
		//circle->mHealth = 100;

		usedPositions.push_back(position);
	}

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mStillCircles[i]. mIndex = i;

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
		//circle->mRadius = 10;

		//circle->mHealth = 100;

		usedPositions.push_back(position);
	}
	mTimer = new Timer();
	mTimer->Start();
}

void Circles::OutputFrame()
{
	std::cout << std::endl << "FrameTime: " << mTimer->GetLapTime() << std::endl;

	if (!OUTPUT_COLLISIONS) return;

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
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete[] mMovingCircles;
	for (int i = 0; i < NUM_CIRCLES / 2; i++) delete[] mStillCircles;
}

void Circles::BlockCircles(Circle* movingCircles, uint32_t numMovingCircles, const Circle* stillCircles, uint32_t numStillCircles, uint32_t startIndex, std::vector<Collision>& collisions)
{
	Circle* nextCircle = movingCircles + 1;
	Circle* movingEnd = movingCircles + numMovingCircles - 1;

	const Circle* stillEnd = stillCircles + numStillCircles - 1;
	int movingIndex = startIndex;
	while (movingCircles != movingEnd)
	{
		auto x1 = movingCircles->mPosition.x;
		auto y1 = movingCircles->mPosition.y;

		auto still = stillCircles;
		int stillIndex = 0;
		bool wallCol = false;

		//auto s = still;
		//auto e = stillEnd;
		//   Float3* m;
		//bool found = false;
		//do
		//{
		//	m = s + (e - s) / 2;
		//	if (movingPositions->x <= m->x)  e = m;
		//	else if (movingPositions->x >= m->x)  s = m;
		//	else found = true;
		//} while (!found && e - s > 1);
		//// If no overlapping x-range found then no collision
		//if (found)
		//{
		//	// Search from the blocker found in the strip, in a rightwards direction, until outside strip or end of list
		//	auto blocker = m;
		//	while (movingPositions->x > blocker->x && blocker != stillEnd)
		//	{
		//		float bx = blocker->x;
		//		float by = blocker->y;
		//		float overlapRight = movingPositions->x - bx;
		//		float overlapLeft = bx - movingPositions->x;
		//		float overlapBottom = movingPositions->y - by;
		//		float overlapTop = by - movingPositions->y;
		//		float o1 = overlapLeft * overlapRight;
		//		float o2 = overlapTop * overlapBottom;
		//		if (o1 > 0 && o2 > 0)
		//		{
		//			auto distance = Distance(x1, y1, bx, by);
		//			float nx = (bx - x1) / distance;
		//			float ny = (by - y1) / distance;
		//			Reflect(mVelocities[movingIndex].x, mVelocities[movingIndex].y, nx, ny);
		//		}
		//		++blocker;
		//	}
		//	// Search from the blocker found in the strip, in a lefttwards direction, until outside strip or end of list
		//	blocker = m;
		//	while (blocker-- != stillPositions && movingPositions->x < blocker->x)  // NOTE: using side effect of --
		//	{
		//		float bx = blocker->x;
		//		float by = blocker->y;
		//		float overlapRight = movingPositions->x - bx;
		//		float overlapLeft = bx - movingPositions->x;
		//		float overlapBottom = movingPositions->y - by;
		//		float overlapTop = by - movingPositions->y;
		//		float o1 = overlapLeft * overlapRight;
		//		float o2 = overlapTop * overlapBottom;
		//		if (o1 > 0 && o2 > 0)
		//		{
		//			auto distance = Distance(x1, y1, bx, by);
		//			float nx = (bx - x1) / distance;
		//			float ny = (by - y1) / distance;
		//			Reflect(mVelocities[movingIndex].x, mVelocities[movingIndex].y, nx, ny);
		//		}
		//	}
		//}

		while (still != stillEnd)
		{
			auto x2 = still->mPosition.x;
			auto y2 = still->mPosition.y;
			auto distanceBetweenCirclesSquared = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));


			// Check for collision
			if (distanceBetweenCirclesSquared <= 400)
			{
				// collision detected
				
				//mHealths[i] -= 20;
				//mHealths[j] -= 20;

				if(OUTPUT_COLLISIONS) collisions.push_back(Collision{ movingIndex, stillIndex });

				// Calculate the normal vector pointing from c1 to c2
				auto distance = Distance(x1, y1, x2, y2);
				float nx = (x2 - x1) / distance;
				float ny = (y2 - y1) / distance;

				// v - 2 * (v . n) * n
				Reflect(movingCircles->mVelocity.x, movingCircles->mVelocity.y, nx, ny);
				wallCol = true;
				break;
			}
			stillIndex++;
			++still;
		}		
		if (wallCol)
		{
			movingIndex++;
			++movingCircles;
			continue;
		}
		int maxPos = MAX_POS + 100;
		int minPos = MIN_POS - 100;

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
			movingIndex++;
			++movingCircles;
			continue;
		}
		movingIndex++;
		++movingCircles;
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
		BlockCircles(work.movingCircles, work.numMovingCircles, work.stillCircles, work.numStillCircles,work.startIndex, work.collisions);
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
