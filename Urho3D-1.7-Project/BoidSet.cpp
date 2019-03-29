// Inculude directives
#include "BoidSet.h"


// Initialisation function
void BoidSet::Initialise(ResourceCache* cache, Scene* scene, int numbOfBoids, bool copy, bool limit, bool halfUpdate)
{
	// Set the number of boids
	numberOfBoids_ = numbOfBoids;

	// Loop to call the Initialise function for each Boid in the array
	for (int i = 0; i < numberOfBoids_; i++)
	{
		Vector3 startPos = Vector3(Random(50.0f) - 25.0f, Random(50.0f) - 25.0f, Random(50.0f) - 25.0f);
		boidList.push_back(Boid());
		boidList[i].Initialise(cache, scene, startPos, copy, limit);
		boidList[i].SetNumberOfBoids(numberOfBoids_);
	}

	// Set flags
	halfUpdate_ = halfUpdate;
}


// Update - called each frame by the game engine
void BoidSet::Update(float timeStep)
{
	// Half update optimisation
	static int update = 1;
	int i = 0;
	int end = numberOfBoids_;

	// Update half
	if (halfUpdate_)
	{
		// Update first half
		if (update == 1)
		{
			i = 0;
			end = numberOfBoids_ / 2;
			update = 2;
		}

		// Update second half
		else
		{
			i = numberOfBoids_ / 2;
			end = numberOfBoids_;
			update = 1;
		}
	}

	// Update all
	else
	{
		i = 0;
		end = numberOfBoids_;
	}

	// Loop to call the ComputeForce and Update function for each boid in the array
	for (; i < end; i++)
	{
		// Compute the force applied to each boid
		// Passed the address of the first element in the array
		boidList[i].ComputeForce(&boidList[0]);

		// Update the boid
		boidList[i].Update(timeStep);
	}
}


// Set the targets
void BoidSet::SetTargets(Node* node, float forceStrength)
{
	// Loop through boids
	for (int i = 0; i < numberOfBoids_; i++)
	{
		boidList[i].SetTargetVectors(node, forceStrength);
	}
}