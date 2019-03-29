#pragma once

// Include directives
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <string>
#include <vector>

// Using the Urho3D namespace
namespace Urho3D
{
	class Node;
	class Scene;
	class RigidBody;
	class CollisionShape;
	class ResourceCache;
}

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

// Boid class
class Boid
{
	friend class Grid;

	// Defines the ranges and scaling factors for each force on the boid
	// - The range at which the Cohension, Seperation and Alignment forces will take effect on the boid
	static float CohesionForce_Range;
	static float SeperationForce_Range;
	static float AlignmentForce_Range;

	// - The scaling factors of the Cohension, Seperation and Alignment forces
	// - Tha maximum velocity
	static float CohesionForce_Factor;
	static float SeperationForce_Factor;
	static float AlignmentForce_Factor;
	static float CohesionForce_VMax;

	// The range at which a boid copys another boids force
	static float Copy_Range;

public:
	// Constructor
	Boid() : 
		pNode(nullptr),
		pRigidBody(nullptr),
		pCollisionShape(nullptr), 
		pObject(nullptr), 
		numberToCompute_(10) 
	{}

	// Destructor
	~Boid() {}

	// Initialisation function
	void Initialise(ResourceCache* cache, Scene* scene, Vector3 starPos, bool copy, bool limit);

	// Update - called each frame by the game engine
	void Update(float timeStep);

	// Called each frame to calculate the force acting on the boid from its neighbours
	void ComputeForce(Boid *pBoid);

	// MOVED CALCULATIONS TO COMPUTE FORE TO REDUCE LOOPS

	//// Steering forces applied to the boid
	//Vector3 AglinmentForce(Boid *pBoid);
	//Vector3 CohesionForce(Boid *pBoid);
	//Vector3 SeperationForce(Boid *pBoid);

	// Further steering forces applied to the boid
	Vector3 TargetPosition(Vector3 targetPosition, float forceStrength);
	
	// Set boid targets
	void SetTargetVectors(Node* target, float forceStrength);

	// Stores the combined forces caused by other boids,
	// which we will calculate and apply on each update
	Vector3 force_;

	// Node object pointer
	Node* pNode;

	// RigidBody object pointer
	RigidBody* pRigidBody;

	// CollisionShape pointer
	CollisionShape* pCollisionShape;

	// StaticModel pointer
	StaticModel* pObject;

	// Set the total number of boids in the set
	void SetNumberOfBoids(int numBoids);

private:
	// Number of boids in the set
	int numBoids_;
	int numberToCompute_;

	// Target vectors for the boids
	Node* target_;
	float forceStrength_ = 2.0f;

	// Speeds / game world size
	float minSpeed_ = 5.0f;
	float maxSpeed_ = 10.0f;
	float worldSize_ = 250.0f;

	// Flags
	bool copyRange_ = false;
	bool forceCopied_ = false;
	bool limitNeighbours_ = false;
};