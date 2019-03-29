#include "MissileSet.h"

// Initialisation function
void MissileSet::Initialise(ResourceCache* cache, Scene* scene, Node* node)
{
	// Loop to call the Initialise function for each missile in the array
	for (int i = 0; i < NUMBER_OF_MISSILES; i++)
	{
		missileList[i].Initialise(cache, scene, node);
	}
}


// Update - called each frame by the game engine
void MissileSet::Update()
{
	// Loop through the missiles
	for (auto& missile : missileList)
	{
		// If it is active update it
		if (missile.IsActive())
			missile.Update();

		// Else set as inactive
		else
			missile.Inactive();
	}
}

// Update - called each frame by the game engine
void MissileSet::Update(float timeStep)
{
	// Loop through the missiles
	for (auto& missile : missileList)
	{
		// If it is active update it
		if (missile.IsActive())
			missile.Update(timeStep);

		// Else set as inactive
		else
			missile.Inactive();
	}
}


// Shoot function
void MissileSet::Shoot(Vector3 direction, RigidBody* target)
{
	// Initialise index
	int i = 0;

	// Set not found as true
	bool notFound = true;

	// Loop through missile while notfound
	while ((i < NUMBER_OF_MISSILES) && (notFound))
	{
		// If missile has zero velocity
		if (!missileList[i].IsActive())
		{
			// Set missile active
			if (target != nullptr) missileList[i].SetActive(true, direction, target);
			else missileList[i].SetActive(true, direction);

			// Set notfound as false
			notFound = false;
		}

		// Increase the index
		i++;
	}
}