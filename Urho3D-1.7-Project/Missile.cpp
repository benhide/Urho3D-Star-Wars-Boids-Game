#include "Missile.h"


// Initialisation function
void Missile::Initialise(ResourceCache* cache, Scene* scene, Node* node)
{
	// Create the node for the missile
	pNodeMissile = scene->CreateChild("Missile");
	pParentNode = node;

	// Parent node rotation
	Quaternion dir(pParentNode->GetRotation());

	// Set the position
	Vector3 targetPos = pParentNode->GetPosition() + dir * offset_;

	// Set missile rotation and scale
	pNodeMissile->SetRotation(Quaternion::IDENTITY);
	pNodeMissile->SetScale(0.25f);
	pNodeMissile->SetPosition(pParentNode->GetPosition());
	lastPosition_ = pNodeMissile->GetPosition();

	// Create missile object
	pObject = pNodeMissile->CreateComponent<StaticModel>();

	// Disable missile
	pObject->SetEnabled(false);

	// Set the missile model, material and shadows
	pObject->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
	pObject->SetMaterial(cache->GetResource<Material>("Materials/Editor/BlueUnlit.xml"));
	pObject->SetCastShadows(true);

	// Give the missile a rigidbody
	pRigidBody = pNodeMissile->CreateComponent<RigidBody>();

	// Set the missile collision layer and shape
	pCollisionShape = pNodeMissile->CreateComponent<CollisionShape>();
	pCollisionShape->SetSphere(1.0f);
	pRigidBody->SetLinearVelocity(Vector3::ZERO);

	// Turn off gravity and set the start position of rigidbody
	pRigidBody->SetMass(1.0f);
	pRigidBody->SetUseGravity(false);

	// Particle emitter
	pParticleEffect_ = cache->GetResource<ParticleEffect>("Particle/Fire.xml");
	if (!pParticleEffect_)
		return;

	// Initialise particle emitter
	pNodeParticle = pNodeMissile->CreateChild("ParticleEmitter");
	pEmitter_ = pNodeParticle->CreateComponent<ParticleEmitter>();
	pEmitter_->SetEffect(pParticleEffect_);

	// Initialise the ribbon trail
	pTrail_ = pNodeMissile->CreateComponent<RibbonTrail>();
	pTrail_->SetMaterial(cache->GetResource<Material>("Materials/RibbonTrail.xml"));
	pTrail_->SetStartColor(Color(0.1f, 0.5f, 1.0f, 1.0f));
	pTrail_->SetEndColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	pTrail_->SetWidth(0.15f);
	pTrail_->SetTailColumn(2);

	// Set the nuumber of particles, scale and life (time)
	pEmitter_->SetNumParticles(numbOfParticles_);
	pNodeParticle->SetScale(2.5f);
	life_ = lifeTime_;
}


// Update - called each frame by the game engine
void Missile::Update()
{
	// If the missile is active and not in flight
	if (isActive_ && !inFlight_)
	{
		// Set the missile active
		pObject->SetEnabled(true);

		// Set emitting
		pEmitter_->SetEnabled(true);
		pEmitter_->SetEmitting(true);
		pEmitter_->SetNumParticles(numbOfParticles_);
		pTrail_->SetEnabled(true);
		pTrail_->SetEmitting(true);
		pTrail_->SetLifetime(0.5f);

		// Missile in flight (will need resetting)
		inFlight_ = true;
		isReset_ = false;

		// Set the rigid body enabled
		pRigidBody->SetEnabled(true);
	}

	// If the missile is active and in flight
	else if (isActive_ && inFlight_)
	{
		// Reduce the life time
		life_ -= 0.1f;

		// Set the velocity
		pRigidBody->SetLinearVelocity(direction_.Normalized() * 100.0f);

		// If the life time reaches zero
		if (life_ <= 0.0f)
		{
			// Reset the missile
			pObject->SetEnabled(false);
			isActive_ = false;
			inFlight_ = false;
			life_ = lifeTime_;
		}
	}
}


// Update - called each frame by the game engine
void Missile::Update(float timeStep)
{
	// If the missile is active and not in flight
	if (isActive_ && !inFlight_)
	{
		// Set the missile active
		pObject->SetEnabled(true);

		// Set emitting
		pEmitter_->SetEnabled(true);
		pEmitter_->SetEmitting(true);
		pEmitter_->SetNumParticles(numbOfParticles_);
		pTrail_->SetEnabled(true);
		pTrail_->SetEmitting(true);
		pTrail_->SetLifetime(0.5f);

		// Missile in flight (will need resetting)
		inFlight_ = true;
		isReset_ = false;

		// Set the rigid body enabled
		pRigidBody->SetEnabled(true);
	}

	// If the missile is active and in flight
	else if (isActive_ && inFlight_)
	{
		// Reduce the life time
		life_ -= timeStep;

		// Target destroyed
		if (!target_->GetNode()->IsEnabled())
			life_ = 0.0f;

		// Apply the force
		pNodeMissile->Translate(Vector3::FORWARD * MISSILE_SPEED);
		Quaternion finalRotation = Quaternion::IDENTITY;
		finalRotation.FromLookRotation(target_->GetPosition() - pNodeMissile->GetPosition(), Vector3::UP);
		pNodeMissile->SetRotation(finalRotation);

		// If the life time reaches zero
		if (life_ <= 0.0f)
		{
			// Reset the missile
			pObject->SetEnabled(false);
			isActive_ = false;
			inFlight_ = false;
			life_ = lifeTime_;
		}
	}
}


// Activate the missile
void Missile::SetActive(bool isActive, Vector3 direction, RigidBody* target)
{
	isActive_ = isActive;
	direction_ = direction;
	target_ = target;
}


// Activate the missile
void Missile::SetActive(bool isActive, Vector3 direction)
{
	isActive_ = isActive;
	direction_ = direction;
}


// Disable missile effects and reset position and velocity
void Missile::Inactive()
{
	// Missile inactive
	if (!isActive_ && !inFlight_ && !isReset_)
	{
		// Stop emitting
		pEmitter_->RemoveAllParticles();
		pEmitter_->SetEmitting(false);
		pEmitter_->SetEnabled(false); 
		pTrail_->SetEmitting(false);
		isReset_ = true;

		// Set the rigid body disabled
		pRigidBody->SetEnabled(false);
	}

	// Parent node rotation
	Quaternion dir(pParentNode->GetRotation());

	// Set the position
	Vector3 targetPos = pParentNode->GetPosition() + dir * offset_;

	// Reset position - missile reset
	pRigidBody->SetLinearVelocity(Vector3::ZERO);
	pNodeMissile->SetPosition(targetPos);
}


// Disable missile
void Missile::DisableMissile()
{
	// Stop emitting
	pEmitter_->RemoveAllParticles();
	pEmitter_->SetEmitting(false);
	pEmitter_->SetEnabled(false);
	pTrail_->SetEmitting(false);
	isReset_ = true;

	// Reset the missile
	pObject->SetEnabled(false);
	isActive_ = false;
	inFlight_ = false;
	life_ = lifeTime_;
	pEmitter_->SetNumParticles(0);

	// Set the rigid body disabled
	pRigidBody->SetEnabled(false);

	// Parent node rotation
	Quaternion dir(pParentNode->GetRotation());

	// Set the position
	Vector3 targetPos = pParentNode->GetPosition() + dir * offset_;

	// Reset position - missile reset
	pRigidBody->SetLinearVelocity(Vector3::ZERO);
	pNodeMissile->SetPosition(targetPos);
}


// Is the missile active
bool Missile::IsActive()
{
	return isActive_;
}
