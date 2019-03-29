#pragma once

// Include directives
#include "Sample.h"
#include "BoidSet.h"
#include "MissileSet.h"


// Using the Urho3D namespace
namespace Urho3D
{
	class Node;
	class Scene;
	class Connection;
	class Window;
}

// Camera controls
const float CAMERA_DISTANCE = 3.0f;
const float CAMERA_HEIGHT = -0.5f;
const float CAMERA_X_ROTATION = 3.0f;
const float CAMERA_SPEED = 20.0f;
const float MOUSE_SENSITIVITY = 0.1f;


// Boid Demo class
class MainGame : public Sample
{
	// Enable type information
	URHO3D_OBJECT(MainGame, Sample);

public:
	// Constructor
	MainGame(Context* context);

	// Destructor
	~MainGame();

	// Setup after engine initialization and before running the main loop
	virtual void Start();

	// Connect
	void HandleConnect(StringHash eventType, VariantMap& eventData);

	// Disconnect
	void HandleDisconnect(StringHash eventType, VariantMap& eventData);

private:
	// Subscribe to necessary events
	void SubscribeToEvents();

	// Create scene
	void CreateScene();

	// Initialise the audio
	void InitAudio();

	// Initialse the UI
	void InitUI();

	// Initialise the boids
	void InitBoids();

	// Initialise the environment objects
	void InitEnvironmentObjects();

	// Create controllable player
	Node* CreatePlayer();

	// Create star destroyer
	void CreateStarDestroyer(Vector3 position, Quaternion rotation);

	// Construct an instruction text to the UI
	void CreateInstructions();

	// Creates the menu
	void CreateMainMenu();

	// Creates a button
	Button* CreateButton(const String& text, int height, Urho3D::Window* window);

	// Creates a line edit
	LineEdit* CreateLineEdit(const String& text, int height, Urho3D::Window* window);

	// Handle quit
	void HandleQuit(StringHash eventType, VariantMap& eventData);

	// Handle start
	void HandleStart(StringHash eventType, VariantMap& eventData);

	// Handle application update. Set controls to player
	void HandleUpdate(StringHash eventType, VariantMap& eventData);

	// Update the player
	void PlayerUpdate(float timeStep);

	// Update the boids
	void BoidsUpdate(float timeStep);

	// Set the target 
	void SetBoidTargets(Node* node);

	// Handle application post-update. Update camera position after player has moved
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

	// Move the camera
	void MoveCamera(float timeStep);

	// Handle collisions - single player
	void HandleCollisionsSingle();

	// Handle collisions - server
	void HandleCollisionsServer(Connection* connection);

	// A client connecting to the server.
	void HandleClientConnected(StringHash eventType, VariantMap& eventData);

	// A client disconnecting from the server.
	void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);

	// Handle start server
	void HandleStartServer(StringHash eventType, VariantMap& eventData);

	// Controls from client
	Controls FromClientToServerControls();

	// Process the cliens controls
	void ProcessClientControls();

	// Physics pre-step
	void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);

	// Client finished loading
	void HandleClientFinishedLoading(StringHash eventType, VariantMap& eventData);

	// Handle remote event from server to Client to share controlled object node ID.
	void HandleServerToClientObjectID(StringHash eventType, VariantMap& eventData);

	// Handle remote event, client tells server that client is ready to start game
	void HandleClientToServerReadyToStart(StringHash eventType, VariantMap& eventData);

	// Client started the game
	void HandleClientStartGame(StringHash eventType, VariantMap & eventData);

	// Pointers
	SharedPtr<Window> window_;
	SharedPtr<UIElement> uiRoot_;
	DebugRenderer* debug_ = nullptr;

	// Client: ID of own object
	unsigned clientObjectID_ = 0;

	// Server Client/Object HashMap
	HashMap<Connection*, WeakPtr<Node>> serverObjects_;
	HashMap<Connection*, MissileSet*> serverMissileSets_;

	// Flags
	bool firstPerson_;
	bool drawDebug_;
	bool menuVisible_;

	// Optimisation flags
	bool useGroups_;
	bool copy_;
	bool limit_;
	bool updateHalf_;

	// Buttons and line edit
	Button* pStart_ = nullptr;
	Button* pConnect_ = nullptr;
	Button* pDisconnect_ = nullptr;
	Button* pStartServer_ = nullptr;
	Button* pStartClient_ = nullptr;
	Button* pQuit_ = nullptr;
	LineEdit* pLineEdit_ = nullptr;

	// Menu cam height
	float menuCamFloatStrength = 0.1f;
	Time* time = nullptr;

	// Player vars
	float speed_;
	float rotationSpeed_;
	float speedMultiplier_;
	int health_;
	int kills_;

	// Set of boids
	BoidSet boidSet1_;
	BoidSet boidSet2_;
	BoidSet boidSet3_;
	BoidSet boidSet4_;
	BoidSet boidSet5_;
	int numbOfBoids_;

	// The player
	Node* player_ = nullptr;
	MissileSet missileSet_;
	float fireTimer_;
	float fireTimerReset_;
	RigidBody* target_ = nullptr;

	// Game mode
	bool gameModeSingle;
	bool gameModeNetwork;
	bool gameModeServer;

	// FPS counter
	float fps = 0.0f;

	// Audio
	SoundSource* music;
	Sound* mainTitle;
	Sound* mainGame;
};