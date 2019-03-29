#pragma once

// Include directives
#include "Boid.h"

// Boid Set class
class BoidSet
{
public:
	// Vector of boids
	std::vector<Boid> boidList;

	// Constructor
	BoidSet() {};

	// Initialisation function
	void Initialise(ResourceCache* cache, Scene* scene, int numbOfBoids, bool copy, bool limit, bool halfUpdate);

	// Update - called each frame by the game engine
	void Update(float timeStep);

	// Set the targets
	void SetTargets(Node* node, float forceStrength);

	// Number of boids
	int numberOfBoids_;

	// Optimisation flag
	bool halfUpdate_ = false;
};