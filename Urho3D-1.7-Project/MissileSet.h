#pragma once

// Include directives
#include "Missile.h"

// Global static integer which defines the number of missiles in a set
static const int NUMBER_OF_MISSILES = 30;

// MissileSet class
class MissileSet
{
public:
	// Array of boids
	Missile missileList[NUMBER_OF_MISSILES];

	// Constructor
	MissileSet() {};

	// Initialisation function
	void Initialise(ResourceCache* cache, Scene* scene, Node* node);

	// Shoot the missile(s)
	void Shoot(Vector3 direction, RigidBody* target);

	// Update - called each frame by the game engine
	void Update();

	// Update - called each frame by the game engine
	void Update(float timeStep);
};