// Include directives
#include "Boid.h"


// Defines the ranges and scaling factors for each force on the boid
// - The range at which the Cohension, Seperation and Alignment forces will take effect on the boid
float Boid::CohesionForce_Range = 900.0f;
float Boid::SeperationForce_Range = 625.0f;
float Boid::AlignmentForce_Range = 225.0f;


// - The scaling factors of the Cohension, Seperation and Alignment forces
// - Tha maximum velocity
float Boid::CohesionForce_Factor = 15.0f;
float Boid::SeperationForce_Factor = 20.0f;
float Boid::AlignmentForce_Factor = 12.0f;
float Boid::CohesionForce_VMax = 3.0f;

// The range at which a boid copys another boids force
float Boid::Copy_Range = 100.0f;


// Initialisation function
void Boid::Initialise(ResourceCache* cache, Scene* scene, Vector3 starPos, bool copy, bool limit)
{
	// ----------------------------------- INITIALISATION -------------------------------------------
	// Create the node for the boid
	pNode = scene->CreateChild("Boid");

	// Set boid rotation and scale
	pNode->SetRotation(Quaternion::IDENTITY);
	pNode->SetScale(1.0f);

	// Create boid object
	pObject = pNode->CreateComponent<StaticModel>();

	// Set the boid model, material and shadows
	if (Random(1.0f) < 0.5f)
	{
		pObject->SetModel(cache->GetResource<Model>("Models/Tie-Fighter.mdl"));
		pObject->ApplyMaterialList("Models/Tie-Fighter.txt");
		pObject->SetCastShadows(true);
	}
	else
	{
		pObject->SetModel(cache->GetResource<Model>("Models/Tie-Interceptor.mdl"));
		pObject->ApplyMaterialList("Models/Tie-Interceptor.txt");
		pObject->SetCastShadows(true);
	}

	// Give the boid a rigidbody
	pRigidBody = pNode->CreateComponent<RigidBody>();

	// Set the boid collision layer and shape
	pRigidBody->SetCollisionLayer(1);
	//pRigidBody->SetCollisionMask(2);
	pCollisionShape = pNode->CreateComponent<CollisionShape>();
	pCollisionShape->SetBox(Vector3::ONE);

	// Turn off gravity and randomise the start position of rigidbody
	pRigidBody->SetMass(1.0f);
	pRigidBody->SetUseGravity(false);
	pRigidBody->SetPosition(starPos);

	// Set the optimisations
	copyRange_ = copy;
	limitNeighbours_ = limit;
}


// Called each frame to calculate the force acting on the boid from its neighbours
void Boid::ComputeForce(Boid* pBoid)
{
	// Centre of mass, accumulated total
	Vector3 centreOfMass;

	// Count number of neighbours
	int numbCF = 0;
	int numbAF = 0;
	int numbSF = 0;

	// The total force
	force_ = Vector3(0.0f, 0.0f, 0.0f);

	// Initialise the seperation force
	Vector3 cohesionForce = Vector3(0.0f, 0.0f, 0.0f);

	// Initialise the alignment force
	Vector3 alignmentForce = Vector3(0.0f, 0.0f, 0.0f);

	// Initialise the seperation force
	Vector3 seperationForce = Vector3(0.0f, 0.0f, 0.0f);

	// Initialise the direction vector
	Vector3 direction = Vector3(0.0f, 0.0f, 0.0f);

	// Search neighbourhood
	for (int i = 0; i < numBoids_; i++)
	{
		// Is the current boid in the loop is this boid - continue to the next boid
		if (this == &pBoid[i])
			continue;

		// Calculate the seperation of this boid from current boid (in the loop)
		Vector3 seperation = pRigidBody->GetPosition() - pBoid[i].pRigidBody->GetPosition();

		// Calculate the distance of this boid from current boid (in the loop)
		float distanceOfBoid = seperation.LengthSquared();

		// If distance is less than copy range
		if (copyRange_ && distanceOfBoid < Copy_Range)
		{
			// Copy the force
			force_ = pBoid[i].force_;
			break;
		}

		// The neighbours
		int neighbourCount;

		// Neighbour count
		if (limitNeighbours_) neighbourCount = numberToCompute_;
		else neighbourCount = numBoids_;

		// Only compute for given number of neighbours
		if (numbCF < neighbourCount)
		{
			// If the distance of the boid is less than the cohesion force range
			if (distanceOfBoid < CohesionForce_Range)
			{
				// Boid within range, so boids are neighbours
				// - Add position of boid to centre of mass
				// - Increase neighbour count
				centreOfMass += pBoid[i].pRigidBody->GetPosition();
				numbCF++;
			}
		}

		// Only compute for given number of neighbours
		if (numbAF < neighbourCount)
		{
			// If the distance of the boid is less than the alignment force range
			if (distanceOfBoid < AlignmentForce_Range)
			{
				// Boid within range, so boids are neighbours
				// - Add boid velocity to the direction vector
				// - Increase neighbour count
				direction += pBoid[i].pRigidBody->GetLinearVelocity();
				numbAF++;
			}
		}

		// Only compute for given number of neighbours
		if (numbSF < neighbourCount)
		{
			// If the distance of the boid is less than the seperation force range
			if (distanceOfBoid < SeperationForce_Range)
			{
				// Boid within range, so boids are neighbours
				// - Calculate the seperation force
				// - Increase neighbour count
				seperationForce += (seperation / seperation.Length());
				numbSF++;
			}
		}

		// Break from loop - neighbou count reached
		else break;
	}

	// If the boid has any neighbours
	if (numbAF > 0)
	{
		// Average the perceived direction vector for the number of neighbours
		direction /= numbAF;

		// Set the alignmnet force to be applied to the boid
		alignmentForce += (direction - pRigidBody->GetLinearVelocity()) * AlignmentForce_Factor;
	}

	// If the boid has neighbours
	if (numbSF > 0)
	{
		// Set the seperation force to be applied to the boid
		seperationForce *= SeperationForce_Factor;
	}

	// If the boid has any neighbours
	if (numbCF > 0) 
	{
		// Find perceived average position = centre of mass
		centreOfMass /= numbCF;

		// Calculate the direction from this boid to the centre of mass (unit vector)
		Vector3 dirOfCentre = (centreOfMass - pRigidBody->GetPosition()).Normalized();

		// Calculate the desired velocity
		Vector3 desiredVelocity = dirOfCentre * CohesionForce_VMax;

		// Set the cohesion force to be applied to the boid
		cohesionForce += (desiredVelocity - pRigidBody->GetLinearVelocity()) * CohesionForce_Factor;

		// The total force
		force_ = seperationForce + alignmentForce + cohesionForce + TargetPosition(Vector3::ZERO, forceStrength_);
	}
}


// MOVED CALCULATIONS TO COMPUTE FORE TO REDUCE LOOPS

//// Calculate the cohesion force applied to the boid
//Vector3 Boid::CohesionForce(Boid* pBoid)
//{
//	// -------------------------------------- COHESION ----------------------------------------------
//	// Cohesion is the behavior that forces the boids to steer towards the "center of mass"
//	// - The center of mass is calculated as the average position of the boids within a certain range
//	// - The range at which the cohension force takes effect is defined as the "CohesionForce_Range"
//	// - Example:
//	// - (^ = boids)
//	//						^	^	^
//	//					^	^	 <------- ^		= This boid will 'steer' towards center of mass
//	//						^	^
//
//	// Centre of mass, accumulated total
//	Vector3 centreOfMass;
//
//	// Count number of neighbours
//	int numbOfNeighbours = 0;
//
//	// Initialise the seperation force
//	Vector3 cohesionForce =  Vector3(0.0f, 0.0f, 0.0f);
//
//	// Search neighbourhood
//	for (int i = 0; i < numBoids_; i++)
//	{
//		// Is the current boid in the loop is this boid - continue to the next boid
//		if (this == &pBoid[i])
//			continue;
//
//		// The neighbours
//		int neighbourCount;
//
//		// Neighbour count
//		if (limitNeighbours_) neighbourCount = numberToCompute_;
//		else neighbourCount = numberToCompute_;
//
//		// Only compute for given number of neighbours
//		if (numbOfNeighbours < neighbourCount)
//		{
//			// Calculate the seperation of this boid from current boid (in the loop)
//			Vector3 seperation = pRigidBody->GetPosition() - pBoid[i].pRigidBody->GetPosition();
//
//			// Calculate the distance of this boid from current boid (in the loop)
//			float distanceOfBoid = seperation.LengthSquared();
//
//			// If distance is less than copy range
//			if (copyRange_ && distanceOfBoid < Copy_Range)
//			{
//				// Copy the force
//				force_ = pBoid[i].force_;
//				continue;
//			}
//
//			// If the distance of the boid is less than the cohesion force range
//			if (distanceOfBoid < CohesionForce_Range)
//			{
//				// Boid within range, so boids are neighbours
//				// - Add position of boid to centre of mass
//				// - Increase neighbour count
//				centreOfMass += pBoid[i].pRigidBody->GetPosition();
//				numbOfNeighbours++;
//			}
//		}
//	}
//
//	// If the boid has any neighbours - i.e. number of boids within the cohesion force range
//	if (numbOfNeighbours > 0)
//	{
//		// Find perceived average position = centre of mass
//		centreOfMass /= numbOfNeighbours;
//
//		// Calculate the direction from this boid to the centre of mass (unit vector)
//		Vector3 dirOfCentre = (centreOfMass - pRigidBody->GetPosition()).Normalized();
//
//		// Calculate the desired velocity
//		Vector3 desiredVelocity = dirOfCentre * CohesionForce_VMax;
//
//		// Set the cohesion force to be applied to the boid
//		cohesionForce += (desiredVelocity - pRigidBody->GetLinearVelocity()) * CohesionForce_Factor;
//	}
//
//	// Return the calculated cohesion force
//	return cohesionForce;
//	// ----------------------------------------------------------------------------------------------
//}
//
//
//// Calculate the alignment force applied to the boid
//Vector3 Boid::AglinmentForce(Boid* pBoid)
//{
//	// -------------------------------------- ALIGNMENT ---------------------------------------------
//	// Alignment is a behavior that forces a boid to line up with the direction of other nearby boids
//	// - The range at which the aglinment force takes effect is defined as the "AlignmentForce_Factor"
//	// - Example:
//	// - (\ = boids)
//	//					\	\	\
//	//				\	\  <|<	\	\			= This boid with 'align' with neighbouring boids
//	//				\	\	\	\	\
//	//					\	\	\	
//
//	// Initialise the direction vector
//	Vector3 direction = Vector3(0.0f, 0.0f, 0.0f);
//
//	// Count number of neighbours
//	int numbOfNeighbours = 0;
//
//	// Initialise the alignment force
//	Vector3 alignmentForce = Vector3(0.0f, 0.0f, 0.0f);
//
//	// Search neighbourhood
//	for (int i = 0; i < numBoids_; i++)
//	{
//		// Is the current boid in the loop is this boid - continue to the next boid
//		if (this == &pBoid[i])
//			continue;
//
//		// The neighbours
//		int neighbourCount;
//
//		// Neighbour count
//		if (limitNeighbours_) neighbourCount = numberToCompute_;
//		else neighbourCount = numBoids_;
//
//		// Only compute for given number of neighbours
//		if (numbOfNeighbours < neighbourCount)
//		{
//			// Calculate the seperation of this boid from current boid (in the loop)
//			Vector3 seperation = pRigidBody->GetPosition() - pBoid[i].pRigidBody->GetPosition();
//
//			// Calculate the distance of this boid from current boid (in the loop)
//			float distanceOfBoid = seperation.LengthSquared();
//
//			// If distance is less than copy range
//			if (copyRange_ && distanceOfBoid < Copy_Range)
//			{
//				// Copy the force
//				force_ = pBoid[i].force_;
//				continue;
//			}
//
//			// If the distance of the boid is less than the alignment force range
//			if (distanceOfBoid < AlignmentForce_Range)
//			{
//				// Boid within range, so boids are neighbours
//				// - Add boid velocity to the direction vector
//				// - Increase neighbour count
//				direction += pBoid[i].pRigidBody->GetLinearVelocity();
//				numbOfNeighbours++;
//			}
//		}
//	}
//
//	// If the boid has any neighbours - i.e. number of boids within the alignmnet force range
//	if (numbOfNeighbours > 0)
//	{
//		// Average the perceived direction vector for the number of neighbours
//		direction /= numbOfNeighbours;
//
//		// Calculate the desired velocity
//		Vector3 desiredVelocity = direction;
//
//		// Set the alignmnet force to be applied to the boid
//		alignmentForce += (desiredVelocity - pRigidBody->GetLinearVelocity()) * AlignmentForce_Factor;
//	}
//
//	// Return the calculated alignment force
//	return alignmentForce;
//	// ----------------------------------------------------------------------------------------------
//}
//
//
//// Calculate the seperation force applied to the boid
//Vector3 Boid::SeperationForce(Boid* pBoid)
//{
//	// ------------------------------------- SEPERATION ---------------------------------------------
//	// Separation is a behavior that forces a boid to maintain a distance from all of its neighbours
//	// - The range at which the seperation force takes effect is defined as the "SeperationForce_Factor"
//	// - Example:
//	// - (^ = boids)
//	//						^	^	^
//	//					^	^ ^ --->			= This boid will 'steer' away from neighbouring boids
//	//						^	^
//
//	// Count number of neighbours
//	int numbOfNeighbours = 0;
//
//	// Initialise the seperation force
//	Vector3 seperationForce = Vector3(0.0f, 0.0f, 0.0f);
//
//	// Search neighbourhood
//	for (int i = 0; i < numBoids_; i++)
//	{
//		// Is the current boid in the loop is this boid - continue to the next boid
//		if (this == &pBoid[i])
//			continue;
//
//		// The neighbours
//		int neighbourCount;
//
//		// Neighbour count
//		if (limitNeighbours_) neighbourCount = numberToCompute_;
//		else neighbourCount = numberToCompute_;
//
//		// Only compute for given number of neighbours
//		if (numbOfNeighbours < neighbourCount)
//		{
//			// Calculate the seperation of this boid from current boid (in the loop)
//			Vector3 seperation = pRigidBody->GetPosition() - pBoid[i].pRigidBody->GetPosition();
//
//			// Calculate the distance of this boid  from current boid (in the loop)
//			float distanceOfBoid = seperation.LengthSquared();
//
//			// If distance is less than copy range
//			if (copyRange_ && distanceOfBoid < Copy_Range)
//			{
//				// Copy the force
//				force_ = pBoid[i].force_;
//				continue;
//			}
//
//			// If the distance of the boid is less than the seperation force range
//			if (distanceOfBoid < SeperationForce_Range)
//			{
//				// Boid within range, so boids are neighbours
//				// - Calculate the seperation force
//				// - Increase neighbour count
//				Vector3 delta = (pRigidBody->GetPosition() - pBoid[i].pRigidBody->GetPosition());
//				seperationForce += (delta / delta.Length());
//				numbOfNeighbours++;
//			}
//		}
//	}
//
//	// If the boid has neighbours
//	if (numbOfNeighbours > 0)
//	{
//		// Set the seperation force to be applied to the boid
//		seperationForce *= SeperationForce_Factor;
//	}
//
//	// Return the calculated seperation force
//	return seperationForce;
//	// ----------------------------------------------------------------------------------------------
//}


// Update - called each frame by the game engine
void Boid::Update(float timeStep)
{
	// ------------------------------------ UPDATING BOID -------------------------------------------
	// Apply the calculated steering force to the boid
	pRigidBody->ApplyForce(force_);

	// Get the velocity of the boid
	Vector3 velocity = pRigidBody->GetLinearVelocity();
	
	// Get the direction of the boid
	float speed = velocity.Length();
	
	// Set the velocity of the boid - unit vector of the velocity
	// multiplied by the direction vector magnitude
	pRigidBody->SetLinearVelocity(velocity.Normalized() * speed);
	
	// Clamp direction value (velocity magnitude) between 10 and 50
	// If the direction vector is less than 10
	if (speed < minSpeed_)
	{
		// Set direction value (velocity magnitude) to 10 and set boid velocity
		speed = minSpeed_;
		pRigidBody->SetLinearVelocity(velocity.Normalized() * speed);
	}
	
	// If the direction vector is more than 50
	else if (speed > maxSpeed_)
	{
		// Set direction value (velocity magnitude) to 50 and set boid velocity
		speed = maxSpeed_;
		pRigidBody->SetLinearVelocity(velocity.Normalized() * speed);
	}

	// Get the normalised velocity
	Vector3 normalizedVelocity = pRigidBody->GetLinearVelocity().Normalized();

	// Set the boids rotation
	Quaternion finalRotation = Quaternion::IDENTITY;
	finalRotation.FromLookRotation(normalizedVelocity, Vector3::UP);
	pRigidBody->SetRotation(finalRotation);

	// Get the position of the boid
	Vector3 position = pRigidBody->GetPosition();
	
	// If the x component of the boids position is less than 10
	if (position.x_ < -worldSize_)
	{
		// Set the x component of the boids position to 150 - update the position
		position.x_ = -worldSize_;
		pRigidBody->SetPosition(position);
	}
	// If the x component of the boids position is more than 150
	else if (position.x_ > worldSize_)
	{
		// Set the x component of the boids position to 150 - update the position
		position.x_ = worldSize_;
		pRigidBody->SetPosition(position);
	}
	
	// If the y component of the boids position is less than -150
	if (position.y_ < -worldSize_)
	{
		// Set the y component of the boids position to -150 - update the position
		position.y_ = -worldSize_;
		pRigidBody->SetPosition(position);
	}
	
	// If the y component of the boids position is more than 150
	else if (position.y_ > worldSize_)
	{
		// Set the y component of the boids position to 150 - update the position
		position.y_ = worldSize_;
		pRigidBody->SetPosition(position);
	}
	
	// If the z component of the boids position is less than -150
	if (position.z_ < -worldSize_)
	{
		// Set the z component of the boids position to -150 - update the position
		position.z_ = -worldSize_;
		pRigidBody->SetPosition(position);
	}
	// If the z component of the boids position is more than 150
	else if (position.z_ > worldSize_)
	{
		// Set the z component of the boids position to 150 - update the position
		position.z_ = worldSize_;
		pRigidBody->SetPosition(position);
	}

	//// Move in the grid
	//grid_->Move(this, pRigidBody->GetPosition().x_, pRigidBody->GetPosition().y_);
}


// Set the total number of boids in the set
void Boid::SetNumberOfBoids(int numBoids)
{
	numBoids_ = numBoids;
}


// Calculate the "target position" force applied to the boid
Vector3 Boid::TargetPosition(Vector3 targetPosition, float forceStrength)
{
	//// If target is greater than attack distance
	//float distance = (targetPosition - pRigidBody->GetPosition()).Length();
	//if (distance > 20.0f) 
	//	return (targetPosition - pRigidBody->GetPosition()) / forceStrength;
	//else 
	//	return ((targetPosition - pRigidBody->GetPosition()) / forceStrength) * -1;
	return (targetPosition - pRigidBody->GetPosition()) / forceStrength;
}


// Set the target vectors and force strength
void Boid::SetTargetVectors(Node* target, float forceStrength)
{
	// Target vectors for the boids
	target_ = target;
	forceStrength_ = forceStrength;
}