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
#include <Urho3D/Graphics/RibbonTrail.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <string>  


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

// Movement speed of missile
const float MISSILE_SPEED = 0.75f;

// Missile class
class Missile
{
public:
	// Constructor
	Missile() :
		pNodeMissile	(nullptr),
		pRigidBody		(nullptr),
		pCollisionShape	(nullptr),
		pObject			(nullptr),
		lifeTime_		(5.0f),
		isActive_		(false),
		inFlight_		(false),
		isReset_		(false),
		numbOfParticles_(100),
		offset_			(Vector3(0.0f, 0.0f, 2.5f)),
		direction_		(Vector3::ZERO)
	{}

	// Destructor
	~Missile() {}

	// Initialisation function
	void Initialise(ResourceCache* cache, Scene* scene, Node* node);

	// Update - called each frame by the game engine
	void Update();

	// Update - called each frame by the game engine
	void Update(float timeStep);

	// Activate/deactivate the missile
	void SetActive(bool isActive, Vector3 direction, RigidBody* target);

	// Activate/deactivate the missile
	void SetActive(bool isActive, Vector3 direction);

	// Disable missile
	void Inactive();

	// Disable missile
	void DisableMissile();

	// Is the missile active
	bool IsActive();

	// Node, particle node and parent object pointer
	Node* pNodeMissile;
	Node* pNodeParticle;
	Node* pParentNode;

	// RigidBody object pointer
	RigidBody* pRigidBody;

	// CollisionShape pointer
	CollisionShape* pCollisionShape;

	// StaticModel pointer
	StaticModel* pObject;

	// Vectors
	Vector3 lastPosition_;

private:
	// Missile life time
	float lifeTime_;
	float life_;
	
	// Number of particles
	int numbOfParticles_;

	// Bool from in flight missile
	bool isActive_;
	bool inFlight_;
	bool isReset_;

	// Vectors
	Vector3 offset_;
	Vector3 direction_;
	RigidBody* target_;

	// Pointer to particle emitter, effect and trail
	ParticleEmitter* pEmitter_;
	ParticleEffect* pParticleEffect_;
	RibbonTrail* pTrail_;
};