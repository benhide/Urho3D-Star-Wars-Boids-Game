// Inculude directives
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/DebugNew.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Audio/BufferedSoundStream.h>
#include <math.h>
#include <string>
#include <chrono>
#include "MainGame.h"

using namespace std::chrono;

// Which port this is running on
static const unsigned short SERVER_PORT = 2345;

// Custom remote event we use to tell the client which object they control
static const StringHash E_CLIENTOBJECTID("ClientObjectID");

// Identifier for the node ID parameter in the event data
static const StringHash PLAYER_ID("IDENTITY");

// Custom event on server, client has pressed button that it wants to start game
static const StringHash E_CLIENTISREADY("ClientReadyToStart");

// String for thr controls
static const String CONTROLS("controlsText");

// Movement controls
static const unsigned CTRL_FORWARD	= 1;
static const unsigned CTRL_BACK		= 2;
static const unsigned CTRL_LEFT		= 4;
static const unsigned CTRL_RIGHT	= 8;
static const unsigned CTRL_FIRE		= 16;

// URHO3D define
URHO3D_DEFINE_APPLICATION_MAIN(MainGame)

// ----------------------------------------------------------------------------------------------
// --------------------------------- INITIALISATION OF THE GAME ---------------------------------
// ----------------------------------------------------------------------------------------------

// Constructor
MainGame::MainGame(Context* context)
	: Sample(context),
	firstPerson_(false),
	drawDebug_(false),
	useGroups_(true),
	copy_(true),
	limit_(true),
	updateHalf_(true),
	numbOfBoids_(100),
	speed_(30.0f),
	rotationSpeed_(0.1f),
	speedMultiplier_(1.75f),
	health_(100),
	kills_(0),
	fireTimer_(0.5f),
	fireTimerReset_(fireTimer_),
	uiRoot_(GetSubsystem<UI>()->GetRoot())
{
	// Time constructor
	time = new Time(context);
}


// Destructor
MainGame::~MainGame()
{
}


// Start function
void MainGame::Start()
{
	// Execute base class startup
	Sample::Start();

	// Create the scene
	CreateScene();

	// Call the menu
	CreateMainMenu();

	// Opens the console window
	OpenConsoleWindow();

	// Subscribe to necessary events
	SubscribeToEvents();

	// Show help text
	CreateInstructions();

	// Set the mouse mode to use in the sample
	Sample::InitMouseMode(MM_RELATIVE);

	// Hide the start and discconect buttons
	pDisconnect_->SetVisible(false);
	pStartClient_->SetVisible(false);

	// Set the game type
	gameModeSingle = false;
	gameModeServer = false;
	gameModeNetwork = false;

	// Debug 
	debug_ = scene_->GetComponent<DebugRenderer>();
}


// Subscribe to events
void MainGame::SubscribeToEvents()
{
	// Subscribe to events
	SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(MainGame, HandlePhysicsPreStep));
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MainGame, HandleUpdate));
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(MainGame, HandlePostUpdate));
	SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(MainGame, HandleClientConnected));
	SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(MainGame, HandleClientDisconnected));

	SubscribeToEvent(E_CLIENTSCENELOADED, URHO3D_HANDLER(MainGame, HandleClientFinishedLoading));
	SubscribeToEvent(E_CLIENTISREADY, URHO3D_HANDLER(MainGame, HandleClientToServerReadyToStart));
	SubscribeToEvent(E_CLIENTOBJECTID, URHO3D_HANDLER(MainGame, HandleServerToClientObjectID));

	// Register remote events
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTISREADY);
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTOBJECTID);

	// Subscribe to events
	SubscribeToEvent(pStart_, E_RELEASED, URHO3D_HANDLER(MainGame, HandleStart));
	SubscribeToEvent(pQuit_, E_RELEASED, URHO3D_HANDLER(MainGame, HandleQuit));
	SubscribeToEvent(pStartServer_, E_RELEASED, URHO3D_HANDLER(MainGame, HandleStartServer));
	SubscribeToEvent(pConnect_, E_RELEASED, URHO3D_HANDLER(MainGame, HandleConnect));
	SubscribeToEvent(pStartClient_, E_RELEASED, URHO3D_HANDLER(MainGame, HandleClientStartGame));
	SubscribeToEvent(pDisconnect_, E_RELEASED, URHO3D_HANDLER(MainGame, HandleDisconnect));
}


// Create the scene
void MainGame::CreateScene()
{
	// Access to resources
	ResourceCache* cache_ = GetSubsystem<ResourceCache>();

	// scene_ is inherited from the sample class
	// context_ is inherited from the Object clas
	scene_ = new Scene(context_);

	// Create scene subsystem components
	scene_->CreateComponent<Octree>(LOCAL);
	scene_->CreateComponent<PhysicsWorld>(LOCAL);
	scene_->CreateComponent<DebugRenderer>(LOCAL);

	// Creates the camera - set the position - set the clip plane
	cameraNode_ = new Node(context_);//
	Camera* camera = cameraNode_->CreateComponent<Camera>(LOCAL);
	cameraNode_->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
	camera->SetFarClip(1000.0f);

	// Configure the viewport via the render subsystem (using the GetSubsystem() function)
	GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

	// Create static scene content. First create a zone for ambient	lighting and fog control
	// Creates a new node which is a child of the scene
	Node* zoneNode = scene_->CreateChild("Zone", LOCAL);

	// Creates a zone object - for lighting and fog
	Zone* zone = zoneNode->CreateComponent<Zone>();

	// Sets the ambient colour/fog colour/start and end of the fog and the bounding box
	zone->SetAmbientColor(Color(0.25f, 0.25f, 0.25f));
	zone->SetFogColor(Color(0.0f, 0.0f, 0.0f));
	zone->SetFogStart(1.0f);
	zone->SetFogEnd(250.0f);
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	// Create a directional light with cascaded shadow mapping
	// Creates a new node which is a child of the scene
	Node* lightNode = scene_->CreateChild("DirectionalLight", LOCAL);

	// Set the direction of the light source
	// The direction is set by setting the node transform rather than configuring the light object itself
	lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));

	// Creates the light object
	Light* light = lightNode->CreateComponent<Light>();

	// Set the light settings - light type/shadows/specular intensity
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
	light->SetSpecularIntensity(0.5f);

	// Skybox
	Node* skyNode = scene_->CreateChild("Sky", LOCAL);
	Skybox* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache_->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache_->GetResource<Material>("Materials/Skybox.xml"));

	// Initialise the envirnoment objects
	InitEnvironmentObjects();

	// Initialise the audio
	InitAudio();
}


// Initialise the audio
void MainGame::InitAudio()
{
	// Access to resources
	ResourceCache* cache_ = GetSubsystem<ResourceCache>();
	Audio* audio_ = GetSubsystem<Audio>();
	music = scene_->CreateComponent<SoundSource>();
	music->SetSoundType(SOUND_MUSIC);
	mainTitle = cache_->GetResource<Sound>("Music/MainTitle.ogg");
	mainTitle->SetLooped(true);
	mainGame = cache_->GetResource<Sound>("Music/ImperialMarch.ogg");
	mainGame->SetLooped(true);
	music->Play(mainTitle);
}


// Initialise the environment objects - SET THE POSITIONS
void MainGame::InitEnvironmentObjects()
{
	// Create star destroyers
	CreateStarDestroyer(Vector3(50.0f, -30.0f, 250.0f), Quaternion(0.0f, 180.0f, 0.0f));
	CreateStarDestroyer(Vector3(0.0f, -50.0f, -200.0f), Quaternion(-15.0f, -130.0f, 30.0f));
	CreateStarDestroyer(Vector3(150.0f, 30.0f, 0.0f), Quaternion(15.0f, -80.0f, 0.0f));
	CreateStarDestroyer(Vector3(-200.0f, 0.0f, 0.0f), Quaternion(10.0f, -260.0f, 20.0f));
}


// Create star destroyer
void MainGame::CreateStarDestroyer(Vector3 position, Quaternion rotation)
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	// Creates the node and sets position, roation and scale
	Node* starDestNode = scene_->CreateChild("Star-Destroyer", LOCAL);
	starDestNode->SetPosition(position);
	starDestNode->SetRotation(rotation);
	starDestNode->SetScale(1.0f);

	// Crestes the model and applies rsources
	StaticModel* starDestObject = starDestNode->CreateComponent<StaticModel>();
	starDestObject->SetModel(cache->GetResource<Model>("Models/Star-Destroyer.mdl"));
	starDestObject->ApplyMaterialList("Models/Star-Destroyer.txt");
	starDestObject->SetCastShadows(true);

	// Creates the ridigbody
	RigidBody* starDestRigidBody = starDestNode->CreateComponent<RigidBody>();
	starDestRigidBody->SetCollisionLayer(2);
	starDestRigidBody->SetCollisionMask(1);

	// Creates the collision shape
	CollisionShape* starDestShape = starDestNode->CreateComponent<CollisionShape>();
	starDestShape->SetTriangleMesh(starDestObject->GetModel(), 0);
}

// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------





// ----------------------------------------------------------------------------------------------
// --------------------------- INITIALISATION OF THE SCENE OBJECTS ------------------------------
// ----------------------------------------------------------------------------------------------

// Create player function
Node* MainGame::CreatePlayer()
{
	// Access the resource cache
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	// Create node/rigidbody/model
	Node* node = scene_->CreateChild("Player");
	node->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
	node->SetScale(1.0f);
	StaticModel* pObject = node->CreateComponent<StaticModel>();
	RigidBody* pRigidBody = node->CreateComponent<RigidBody>();

	// Set the box model, material and shadows
	pObject->SetModel(cache->GetResource<Model>("Models/X-Wing.mdl"));
	pObject->ApplyMaterialList("Models/X-Wing.txt");
	pObject->SetCastShadows(true);

	// Set non-zero mass so that the body becomes dynamic
	pRigidBody->SetCollisionLayer(1);
	pRigidBody->SetMass(1.0f);
	pRigidBody->SetUseGravity(false);

	// Set linear velocity and damping
	pRigidBody->SetLinearVelocity(Vector3::ZERO);
	pRigidBody->SetAngularDamping(0.95f);
	pRigidBody->SetLinearDamping(0.95f);

	// Set a capsule shape for collision
	CollisionShape* shape = node->CreateComponent<CollisionShape>();
	shape->SetBox(Vector3(0.2f, 0.2f, 1.1f));

	// Create a light for the players ship
	Node* lightNode = node->CreateChild("Light");
	lightNode->SetDirection(cameraNode_->GetDirection());
	Light* light = lightNode->CreateComponent<Light>();

	// Set the light values
	light->SetColor(Color(1.0f, 1.0f, 1.0f));
	light->SetLightType(LIGHT_SPOT);
	light->SetBrightness(1.0f);
	light->SetTemperature(1500.0f);
	light->SetRange(1000);
	light->SetFov(30);
	light->SetEnabled(true);

	// Set the players vars
	node->SetVar("Health", health_);
	node->SetVar("Kills", kills_);
	node->SetVar("FireTimer", fireTimerReset_);

	// Set yaw pitch roll
	yaw_ = 0.0f;
	pitch_ = 0.0f;
	roll_ = 0.0f;

	// Return the node
	return node;
}


// Create instructions - TODO
void MainGame::CreateInstructions()
{
	// Access the resource and UI Subsystems
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	// Construct new Text object, set string to display and font to use
	Text* controlsText = uiRoot_->CreateChild<Text>(CONTROLS);
	controlsText->SetText
	(
		"W / S - CHANGE SPEED\n"
		"A / D - ROLL SHIP\n"
		"MOUSE - LOOK\n"
		"LMB - FIRE MISSILES\n"
		"C - SHOW THIS TEXT\n"
		"F - SWITCH VIEW\n"
		"M - SHOW MENU"
	);

	// Sete the font
	controlsText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

	// The text has multiple rows. Center them in relation to each other
	controlsText->SetTextAlignment(HA_CENTER);

	// Position the text relative to the screen center
	controlsText->SetHorizontalAlignment(HA_CENTER);
	controlsText->SetVerticalAlignment(VA_CENTER);
	controlsText->SetPosition(0, -(uiRoot_->GetHeight() / 4));
	controlsText->SetVisible(false);
}


// Initialise the UI - TODO
void MainGame::InitUI()
{
	// Access the resource cache/ui/graphics
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	Graphics* graphics = GetSubsystem<Graphics>();
	Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

	// Get the window size
	float width = (float)graphics->GetWidth();
	float height = (float)graphics->GetHeight();

	// Draw the crosshair
	Texture2D* crosshair = cache->GetResource<Texture2D>("Textures/Crosshair2.png");
	Sprite* crosshairSprite = uiRoot_->CreateChild<Sprite>();
	crosshairSprite->SetName("crosshair");
	crosshairSprite->SetTexture(crosshair);
	crosshairSprite->SetHotSpot(40, 40);
	crosshairSprite->SetSize(80, 80);
	crosshairSprite->SetPosition(Vector2(width / 2, height / 2));
	crosshairSprite->SetPriority(-15);

	// Draw the crosshair - target in sight
	Texture2D* crosshairTarget = cache->GetResource<Texture2D>("Textures/Crosshair.png");
	Sprite* crosshairTargetSprite = uiRoot_->CreateChild<Sprite>();
	crosshairTargetSprite->SetName("crosshairTarget");
	crosshairTargetSprite->SetTexture(crosshairTarget);
	crosshairTargetSprite->SetHotSpot(40, 40);
	crosshairTargetSprite->SetSize(80, 80);
	crosshairTargetSprite->SetPosition(Vector2(width / 2, height / 2));
	crosshairTargetSprite->SetVisible(false);
	crosshairTargetSprite->SetPriority(-15);

	//// Draw the players health
	//Text* health = uiRoot_->CreateChild<Text>();
	//health->SetName("Health");
	//health->SetText("Health: " + String(scene_->GetChild("Player", true)->GetVar("Health").GetInt()));
	//health->SetFont(font, 20);
	//health->SetColor(Color::WHITE);
	//health->SetPosition(30, height - 50);
	//health->SetPriority(10);

	//// Draw the players kills
	//Text* kills = uiRoot_->CreateChild<Text>();
	//kills->SetName("Kills");
	//kills->SetText("Kills:" + String(scene_->GetChild("Player", true)->GetVar("Kills").GetInt()));
	//kills->SetFont(font, 20);
	//kills->SetColor(Color::WHITE);
	//kills->SetPosition(width - 150, height - 50);
}


// Initialse the boids
void MainGame::InitBoids()
{
	// Access the resource cache
	ResourceCache* cache_ = GetSubsystem<ResourceCache>();

	// Get the player
	Node* player = scene_->GetChild("Player", true);

	// Use grouping on the boids
	if (useGroups_)
	{
		boidSet1_.Initialise(cache_, scene_, (numbOfBoids_ / 5), copy_, limit_, updateHalf_);
		boidSet2_.Initialise(cache_, scene_, (numbOfBoids_ / 5), copy_, limit_, updateHalf_);
		boidSet3_.Initialise(cache_, scene_, (numbOfBoids_ / 5), copy_, limit_, updateHalf_);
		boidSet4_.Initialise(cache_, scene_, (numbOfBoids_ / 5), copy_, limit_, updateHalf_);
		boidSet5_.Initialise(cache_, scene_, (numbOfBoids_ / 5), copy_, limit_, updateHalf_);
	}

	// No groups
	else boidSet1_.Initialise(cache_, scene_, numbOfBoids_, copy_, limit_, updateHalf_);
}

// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------





// ----------------------------------------------------------------------------------------------
// ------------------------------ INITIALISATION OF THE MENU ------------------------------------
// ----------------------------------------------------------------------------------------------

// Creates the menu
void MainGame::CreateMainMenu()
{
	// Set the mouse mode to use in the sample
	// Access to resources
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	uiRoot_->SetDefaultStyle(uiStyle);

	// Create a Cursor UI element.We want to be able to hide and show the main menu
	// at will. When hidden, the mouse will control the camera, and when visible, the
	// mouse will be able to interact with the GUI.
	SharedPtr<Cursor> cursor(new Cursor(context_));
	cursor->SetStyleAuto(uiStyle);
	ui->SetCursor(cursor);

	// Create the Window and add it to the UI's root node
	window_ = new Window(context_);
	uiRoot_->AddChild(window_);

	// Set Window size and layout settings
	window_->SetMinWidth(384);
	window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	window_->SetAlignment(HA_CENTER, VA_BOTTOM);
	window_->SetName("Window");
	window_->SetStyleAuto();

	// Set the background
	Texture2D* background = cache->GetResource<Texture2D>("Textures/background.png");
	Sprite* sprite = uiRoot_->CreateChild<Sprite>();
	sprite->SetName("background");
	sprite->SetTexture(background);
	sprite->SetHotSpot(0, 0);
	sprite->SetSize(GetSubsystem<Graphics>()->GetWidth(), GetSubsystem<Graphics>()->GetHeight());
	sprite->SetPriority(-10);

	// Menu buttons and line edit
	pStart_ = CreateButton("Single Player", 24, window_);
	pConnect_ = CreateButton("Connect", 24, window_);
	pDisconnect_ = CreateButton("Disconnect", 24, window_);
	pStartServer_ = CreateButton("Start Server", 24, window_);
	pStartClient_ = CreateButton("Client: Start Game", 24, window_);
	pQuit_ = CreateButton("Quit Game", 24, window_);
	pLineEdit_ = CreateLineEdit("", 24, window_);
}


// Creates a buuton
Button* MainGame::CreateButton(const String& text, int height, Urho3D::Window* window)
{
	Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");
	Button* button = window->CreateChild<Button>();
	button->SetMinHeight(height);
	button->SetMaxHeight(height);
	button->SetStyleAuto();
	Text* buttonText = button->CreateChild<Text>();
	buttonText->SetFont(font, 12);
	buttonText->SetAlignment(HA_CENTER, VA_CENTER);
	buttonText->SetText(text);
	window_->AddChild(button);
	return button;
}


// Creates a line edit
LineEdit* MainGame::CreateLineEdit(const String& text, int height, Urho3D::Window* window)
{
	LineEdit* lineEdit = window->CreateChild<LineEdit>();
	lineEdit->SetMinHeight(height);
	lineEdit->SetMaxHeight(height);
	lineEdit->SetAlignment(HA_CENTER, VA_CENTER);
	lineEdit->SetText(text);
	lineEdit->SetStyleAuto();
	return lineEdit;
}


// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------





// ----------------------------------------------------------------------------------------------
// -------------------------------- START / QUIT / UPDATE EVENTS --------------------------------
// ----------------------------------------------------------------------------------------------

// Quit application
void MainGame::HandleQuit(StringHash eventType, VariantMap& eventData)
{
	engine_->Exit();
}


// Handle start
void MainGame::HandleStart(StringHash eventType, VariantMap& eventData)
{
	// Create the player
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	player_ = CreatePlayer();
	missileSet_.Initialise(cache, scene_, player_);

	// Initialise the UI and boids
	InitBoids();
	InitUI();

	// Set the game mode
	gameModeSingle = true;

	// Set button visibilty
	pStart_->SetVisible(false);
	pConnect_->SetVisible(false);
	pStartServer_->SetVisible(false);
	pLineEdit_->SetVisible(false);

	// Show controls text
	UIElement* controls = uiRoot_->GetChild(CONTROLS);
	controls->SetVisible(true);

	// Hide the menu
	menuVisible_ = !menuVisible_;

	// Set the cursor visibility
	GetSubsystem<UI>()->GetCursor()->SetVisible(menuVisible_);

	// Play music
	music->Play(mainGame);
}


// Update - called each frame by the game engine
void MainGame::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	// Using the update namespace
	using namespace Update;

	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	// Get the input and UI subsystems
	Input* input = GetSubsystem<Input>();
	UI* ui = GetSubsystem<UI>();

	// Single player game
	if (gameModeSingle)
	{
		// If menu not visibile
		if (!menuVisible_)
		{
			// Handle player update
			PlayerUpdate(timeStep);

			//// Start the clock
			//auto start = high_resolution_clock::now();

			// Handle boids update
			BoidsUpdate(timeStep);
			{
				//// End the clock
				//auto end = high_resolution_clock::now();

				//// Constructs a duration object using milliseconds
				//duration<double> elapsed = duration_cast<duration<double>>(end - start);
				//std::string timer = 
				//	"; Time (seconds); "
				//	+ std::to_string(elapsed.count()) 
				//	+ "; NUMB BOIDS; "
				//	+ std::to_string(numbOfBoids_)
				//	+ ";";
				//URHO3D_LOGINFO(timer.c_str());
			}
		}

		// Show the menu
		if (input->GetKeyPress(KEY_M))
		{
			UIElement* instruction = ui->GetRoot()->GetChild(CONTROLS);
			instruction->SetVisible(false);
			menuVisible_ = !menuVisible_;

			// Set the cursor visibility
			GetSubsystem<UI>()->GetCursor()->SetVisible(menuVisible_);
		}

		// Show the controls
		if (input->GetKeyPress(KEY_C) && !menuVisible_)
		{
			UIElement* instruction = ui->GetRoot()->GetChild(CONTROLS);
			instruction->SetVisible(!instruction->IsVisible());
		}

		// Switch between 1st and 3rd person
		if (input->GetKeyPress(KEY_F))
			firstPerson_ = !firstPerson_;

		// Handle single player collisions
		HandleCollisionsSingle();
	}

	// Network player game
	else if (gameModeNetwork)
	{
		// Reset mouse controls
		pitch_ = 0.0f;
		yaw_ = 0.0f;

		// Set the speeds
		float speed = speed_;
		float rotationSpeed = rotationSpeed_;

		// Set speed and rotation speed
		if (input->GetKeyDown(KEY_W))
			rotationSpeed *= speedMultiplier_;

		// Brake speed
		else if (input->GetKeyDown(KEY_S))
			rotationSpeed /= speedMultiplier_;

		// Set the camera yaw and pitch from the mouse movement
		yaw_ += (float)input->GetMouseMoveX() * rotationSpeed_;
		pitch_ += (float)input->GetMouseMoveY() * rotationSpeed_;

		// Show the menu
		if (input->GetKeyPress(KEY_M))
		{
			UIElement* instruction = ui->GetRoot()->GetChild(CONTROLS);
			instruction->SetVisible(false);
			menuVisible_ = !menuVisible_;

			// Set the cursor visibility
			GetSubsystem<UI>()->GetCursor()->SetVisible(menuVisible_);
		}

		// Show the controls
		if (input->GetKeyPress(KEY_C))
		{
			UIElement* instruction = ui->GetRoot()->GetChild(CONTROLS);
			instruction->SetVisible(!instruction->IsVisible());
		}

		// Switch between 1st and 3rd person
		if (input->GetKeyPress(KEY_F))
			firstPerson_ = !firstPerson_;
	}

	// Server game
	else if (gameModeServer)
	{
		// Toggle menu
		if (input->GetKeyPress(KEY_M))
		{
			menuVisible_ = !menuVisible_;

			// Set the cursor visibility
			GetSubsystem<UI>()->GetCursor()->SetVisible(menuVisible_);
		}

		// Network connection
		Network* network = GetSubsystem<Network>();
		const Vector<SharedPtr<Connection>>& connections = network->GetClientConnections();

		//// Targets set
		//bool targetset = false;

		// Loop through connections - for missiles
		for (unsigned i = 0; i < connections.Size(); ++i)
		{
			// Connections
			Connection* connection = connections[i];
			Node* player = serverObjects_[connection];

			// If the player exsists and the fire timer is above 0
			if (player && player->GetVar("FireTimer").GetFloat() > 0.0f)
			{
				// If the fire players local firetimer is less than 0
				if (fireTimer_ <= 0)
					fireTimer_ = player->GetVar("FireTimer").GetFloat();

				// Reduce the timer
				fireTimer_ -= timeStep;

				// Reset the timer
				if (fireTimer_ <= 0)
					player->SetVar("FireTimer", 0.0f);

				// Handle collisions
				HandleCollisionsServer(connection);

				//// Set the target
				//SetBoidTargets(player);
				//targetset = true;
			}
		}

		// Handle boids update
		BoidsUpdate(timeStep);
	}

	// Toggle physics debug geometry with space
	if (input->GetKeyPress(KEY_G))
		drawDebug_ = !drawDebug_;
}


//  Handle player update
void MainGame::PlayerUpdate(float timeStep)
{
	// Get the input and UI subsystems
	Input* input = GetSubsystem<Input>();
	UI* ui = GetSubsystem<UI>();

	// Return if no scene or cursor visibile
	if (GetSubsystem<UI>()->GetFocusElement()) return;

	// The force applied to the player
	Vector3 force = Vector3::FORWARD;

	// Get the player and rigidbody
	Node* player = scene_->GetChild("Player", true);
	RigidBody* rigidbody = player->GetComponent<RigidBody>();

	// Reset mouse controls
	roll_ = 0.0f;
	pitch_ = 0.0f;
	yaw_ = 0.0f;

	// Set the speeds
	float speed = speed_;
	float rotationSpeed = rotationSpeed_;

	// Set speed and rotation speed
	if (input->GetKeyDown(KEY_W))
	{
		speed *= speedMultiplier_;
		rotationSpeed *= speedMultiplier_;
	}

	// Brake speed
	else if (input->GetKeyDown(KEY_S))
	{
		speed /= speedMultiplier_;
		rotationSpeed /= speedMultiplier_;
	}

	// Set force and roll
	if (input->GetKeyDown(KEY_A))	roll_ -= 1.0f * rotationSpeed;
	if (input->GetKeyDown(KEY_D))	roll_ += 1.0f * rotationSpeed;

	// Set the camera yaw and pitch from the mouse movement
	yaw_ += (float)input->GetMouseMoveX() *  rotationSpeed;
	pitch_ += (float)input->GetMouseMoveY() * rotationSpeed;

	// Apply forces
	rigidbody->ApplyTorque(rigidbody->GetRotation() * Vector3(pitch_, yaw_, roll_));
	rigidbody->ApplyForce(rigidbody->GetRotation() * force * speed);

	// Missiles
	{
		// Reduce the fire timer
		if (fireTimer_ > 0.0f)
			fireTimer_ -= timeStep;

		// Fire missile
		if (input->GetMouseButtonDown(MOUSEB_LEFT) && fireTimer_ <= 0 && target_ != nullptr)
		{
			// Fire missile
			missileSet_.Shoot(rigidbody->GetRotation() * Vector3::FORWARD, target_);
			fireTimer_ = fireTimerReset_;
		}

		// Update missiles
		missileSet_.Update(timeStep);
	}
}


// Handle boids update
void MainGame::BoidsUpdate(float timeStep)
{
	// Use grouping on the boids
	if (useGroups_)
	{
		boidSet1_.Update(timeStep);
		boidSet2_.Update(timeStep);
		boidSet3_.Update(timeStep);
		boidSet4_.Update(timeStep);
		boidSet5_.Update(timeStep);
	}

	// No groups
	else boidSet1_.Update(timeStep);
}


// Set the target 
void MainGame::SetBoidTargets(Node* node)
{
	// Use grouping on the boids
	if (useGroups_)
	{
		boidSet1_.SetTargets(node, 2.0f);
		boidSet2_.SetTargets(node, 2.0f);
		boidSet3_.SetTargets(node, 2.0f);
		boidSet4_.SetTargets(node, 2.0f);
		boidSet5_.SetTargets(node, 2.0f);
	}

	// No groups
	else boidSet1_.SetTargets(node, 2.0f);
}


// Handle the post update logic
void MainGame::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	// Using the update namespace
	using namespace Update;

	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	// Return if no scene or cursor visibile
	if (GetSubsystem<UI>()->GetFocusElement()) return;

	// Move the camera
	MoveCamera(timeStep);

	// If draw debug mode is enabled, draw physics debug geometry. Use depth test to make the result easier to interpret
	if (drawDebug_)
		scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);

	// Ui and input
	UI* ui = GetSubsystem<UI>();
	Input* input = GetSubsystem<Input>();
	window_->SetVisible(menuVisible_);

	// Hide the background
	uiRoot_->GetChild("background", false)->SetVisible(menuVisible_);
}


// Move the camera
void MainGame::MoveCamera(float timeStep)
{
	// If single player game
	if (gameModeSingle)
	{
		// If we have a player
		if (player_)
		{
			// Physics update has completed. Position camera behind vehicle
			Quaternion dir(player_->GetRotation());

			// First person view
			if (firstPerson_)
			{
				// Set the camera positions
				Vector3 cameraTargetPos = player_->GetPosition() - dir * Vector3(0.0f, -0.2f, 0.2f);

				// Set the camera positions
				cameraNode_->SetPosition(cameraTargetPos);
				cameraNode_->SetRotation(dir * Quaternion(0.0f, 0.0f, 0.0f));
			}

			// Third person view
			else
			{
				// Set the camera positions
				Vector3 cameraTargetPos = player_->GetPosition() - dir * Vector3(0.0f, CAMERA_HEIGHT + (sinf(time->GetElapsedTime()) * menuCamFloatStrength), CAMERA_DISTANCE);
				Vector3 cameraStartPos = player_->GetPosition();

				// Set the camera position and rotation
				cameraNode_->SetPosition(cameraNode_->GetPosition().Lerp(cameraTargetPos, (timeStep * CAMERA_SPEED)));
				cameraNode_->SetRotation(cameraNode_->GetRotation().Slerp(dir * Quaternion(CAMERA_X_ROTATION, 0.0f, 0.0f), (timeStep * CAMERA_SPEED)));
			}

			// Crosshair target
			{
				// Ray positions
				Vector3 startCenter = cameraNode_->GetComponent<Camera>()->ScreenToWorldPoint(Vector3(0.5f, 0.5f, 5.0f));
				Vector3 endCenter = cameraNode_->GetComponent<Camera>()->ScreenToWorldPoint(Vector3(0.5f, 0.5f, 1000.0f));

				// Sphere casting
				Ray ray(startCenter, endCenter);
				PhysicsRaycastResult result;
				scene_->GetComponent<PhysicsWorld>()->SphereCast(result, ray, 0.75f, 10000.0f, 1);

				// On hit
				if (result.body_)
				{
					// Get the node
					Node* node = result.body_->GetNode();

					// If the node is a boid
					if (node->GetName() == "Boid")
					{
						// Fire missile
						target_ = result.body_;

						// Set the crosshair
						uiRoot_->GetChild("crosshairTarget", false)->SetVisible(true);
						uiRoot_->GetChild("crosshair", false)->SetVisible(false);
					}
				}

				// No hit
				else
				{
					// Set the target
					target_ = nullptr;

					// Set the crosshair
					uiRoot_->GetChild("crosshairTarget", false)->SetVisible(false);
					uiRoot_->GetChild("crosshair", false)->SetVisible(true);
				}
			}
		}
	}

	// If network player game
	if (gameModeNetwork)
	{
		// Only move the camera if we have a controllable object
		if (clientObjectID_)
		{
			// Get the player
			Node* player = this->scene_->GetNode(clientObjectID_);

			// If we have a player
			if (player)
			{
				// Physics update has completed. Position camera behind vehicle
				Quaternion dir(player->GetRotation());

				// First person view
				if (firstPerson_)
				{
					// Set the camera positions
					Vector3 cameraTargetPos = player->GetPosition() - dir * Vector3(0.0f, -0.2f, 0.2f);

					// Set the camera positions
					cameraNode_->SetPosition(cameraTargetPos);
					cameraNode_->SetRotation(dir * Quaternion(0.0f, 0.0f, 0.0f));
				}

				// Third person view
				else
				{
					// Set the camera positions
					Vector3 cameraTargetPos = player->GetPosition() - dir * Vector3(0.0f, CAMERA_HEIGHT + (sinf(time->GetElapsedTime()) * menuCamFloatStrength), CAMERA_DISTANCE);
					Vector3 cameraStartPos = player->GetPosition();

					// Set the camera position and rotation
					cameraNode_->SetPosition(cameraNode_->GetPosition().Lerp(cameraTargetPos, (timeStep * CAMERA_SPEED)));
					cameraNode_->SetRotation(cameraNode_->GetRotation().Slerp(dir * Quaternion(CAMERA_X_ROTATION, 0.0f, 0.0f), (timeStep * CAMERA_SPEED)));
				}

				// Crosshair target
				{
					// Ray positions
					Vector3 startCenter = cameraNode_->GetComponent<Camera>()->ScreenToWorldPoint(Vector3(0.5f, 0.5f, 5.0f));
					Vector3 endCenter = cameraNode_->GetComponent<Camera>()->ScreenToWorldPoint(Vector3(0.5f, 0.5f, 1000.0f));

					// Sphere casting
					Ray ray(startCenter, endCenter);
					PhysicsRaycastResult result;
					scene_->GetComponent<PhysicsWorld>()->SphereCast(result, ray, 0.75f, 10000.0f, 1);

					// On hit
					if (result.body_)
					{
						// Get the node
						Node* node = result.body_->GetNode();

						// If the node is a boid
						if (node->GetName() == "Boid")
						{
							// Set the crosshair
							uiRoot_->GetChild("crosshairTarget", false)->SetVisible(true);
							uiRoot_->GetChild("crosshair", false)->SetVisible(false);
						}
					}

					// No hit
					else
					{
						// Set the crosshair
						uiRoot_->GetChild("crosshairTarget", false)->SetVisible(false);
						uiRoot_->GetChild("crosshair", false)->SetVisible(true);
					}
				}
			}
		}
	}

	// If server
	if (gameModeServer)
	{
		// Get the input subsystem
		Input* input = GetSubsystem<Input>();

		// Set the camera yaw and pitch from the mouse movement
		yaw_ += (float)input->GetMouseMoveX() * MOUSE_SENSITIVITY;
		pitch_ += (float)input->GetMouseMoveY() * MOUSE_SENSITIVITY;
		pitch_ = Clamp(pitch_, -90.0f, 90.0f);

		// Construct new orientation for the camera scene node from
		// yaw and pitch. Roll is fixed to zero
		cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

		// Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
		// Use the Translate() function (default local space) to move relative to the node's orientation.
		// Move camera forward
		if (input->GetKeyDown(KEY_W))
			cameraNode_->Translate(Vector3::FORWARD * (timeStep * CAMERA_SPEED));

		// Move camera backward
		if (input->GetKeyDown(KEY_S))
			cameraNode_->Translate(Vector3::BACK * (timeStep * CAMERA_SPEED));

		// Move camera left
		if (input->GetKeyDown(KEY_A))
			cameraNode_->Translate(Vector3::LEFT * (timeStep * CAMERA_SPEED));

		// Move camera right
		if (input->GetKeyDown(KEY_D))
			cameraNode_->Translate(Vector3::RIGHT * (timeStep * CAMERA_SPEED));

		// Move camera up
		if (input->GetKeyDown(KEY_Q))
			cameraNode_->Translate(Vector3::UP * (timeStep * CAMERA_SPEED));

		// Move camera down
		if (input->GetKeyDown(KEY_E))
			cameraNode_->Translate(Vector3::DOWN * (timeStep * CAMERA_SPEED));
	}
}


// Handle collisions - single player
void MainGame::HandleCollisionsSingle()
{
	// Foreach missile in the set
	for (auto& missile : missileSet_.missileList)
	{
		// If the missile is active
		if (missile.IsActive())
		{
			// Ray positions
			Vector3 position = missile.pNodeMissile->GetPosition();
			Vector3 lastPosition = missile.lastPosition_;

			// Sphere casting
			Ray ray(lastPosition, position - lastPosition);
			PhysicsRaycastResult result;

			// Distance travelled
			float distTravelled = (position - lastPosition).Length();

			// Missile moved at least 0.5 units
			if (distTravelled > 0.5f)
			{
				// Spherecasting
				scene_->GetComponent<PhysicsWorld>()->SphereCast(result, ray, 0.25f, distTravelled, 1);

				// Something between the last and current position
				if (result.distance_ <= distTravelled)
				{
					// On hit
					if (result.body_)
					{
						// Get the node
						Node* node = result.body_->GetNode();

						// If the node is a boid
						if (node->GetName() == "Boid")
						{
							node->SetEnabled(false); // Reset position instead of disabled?
							missile.DisableMissile();
						}
					}
				}

				// Reset position
				missile.lastPosition_ = position;
			}
		}
	}
}


// Handle collisions - server
void MainGame::HandleCollisionsServer(Connection* connection)
{
	// The network player
	Node* playerNode = serverObjects_[connection];

	// Foreach missile in the set
	for (auto& missile : serverMissileSets_[connection]->missileList)
	{
		// If the missile is active
		if (missile.IsActive())
		{
			// Ray positions
			Vector3 position = missile.pNodeMissile->GetPosition();
			Vector3 lastPosition = missile.lastPosition_;

			// Sphere casting
			Ray ray(lastPosition, position - lastPosition);
			PhysicsRaycastResult result;

			// Distance travelled
			float distTravelled = (position - lastPosition).Length();

			// Missile moved at least 0.5 units
			if (distTravelled > 0.5f)
			{
				// Spherecasting
				scene_->GetComponent<PhysicsWorld>()->SphereCast(result, ray, 0.25f, distTravelled, 1);

				// Something between the last and current position
				if (result.distance_ <= distTravelled)
				{
					// On hit
					if (result.body_)
					{
						// Get the node
						Node* node = result.body_->GetNode();

						// If the node is a boid
						if (node->GetName() == "Boid")
						{
							node->SetEnabled(false); // Reset position instead of disabled?
							missile.DisableMissile();
						}
					}
				}

				// Reset position
				missile.lastPosition_ = position;
			}
		}
	}
}

// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------





// ----------------------------------------------------------------------------------------------
// ---------------------------------- CONNECTION / DISCONNECTION --------------------------------
// ----------------------------------------------------------------------------------------------

// Client connected
void MainGame::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
	printf("(HandleClientConnected) A client has connected!\n");
	using namespace ClientConnected;

	// When a client connects, assign to a scene
	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	newConnection->SetScene(scene_);
}


// Client disconnected
void MainGame::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
	// Network conection
	using namespace ClientConnected;
	Connection* connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	// Remove the player
	Node* player = serverObjects_[connection];
	if (player != nullptr)
		player->Remove();
	serverObjects_.Erase(connection);

	// Remove the missiles
	MissileSet* missiles = serverMissileSets_[connection];
	if (missiles != nullptr)
		delete missiles;
	serverMissileSets_.Erase(connection);

	// Set the network game mode
	gameModeNetwork = false;
}


// Server started
void MainGame::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
	printf("(HandleStartServer called) Server is started!");

	// Initialise the boids
	InitBoids();

	// Connection
	Network* network = GetSubsystem<Network>();
	network->StartServer(SERVER_PORT);

	// Server game
	gameModeServer = true;

	// Set button visibilty
	pStart_->SetVisible(false);
	pConnect_->SetVisible(false);
	pDisconnect_->SetVisible(true);
	pStartServer_->SetVisible(false);
	pLineEdit_->SetVisible(false);

	// Hide the menu
	menuVisible_ = !menuVisible_;

	// Set the cursor visibility
	GetSubsystem<UI>()->GetCursor()->SetVisible(menuVisible_);

	// Play music
	music->Play(mainGame);
}


// Handle a client connected
void MainGame::HandleConnect(StringHash eventType, VariantMap& eventData)
{
	// Clears scene, prepares it for receiving
	Network* network = GetSubsystem<Network>();
	String address = pLineEdit_->GetText().Trimmed();
	if (address.Empty()) { address = "localhost"; }

	// Reset own object ID from possible previous connection
	clientObjectID_ = 0;

	// Specify scene to use as a client for replication
	network->Connect(address, SERVER_PORT, scene_);

	// Set the network game mode
	gameModeNetwork = true;

	// Set button visibility
	pStart_->SetVisible(false);
	pConnect_->SetVisible(false);
	pStartServer_->SetVisible(false);
	pStartClient_->SetVisible(true);
	pLineEdit_->SetVisible(false);
}


// Handle a disconnects
void MainGame::HandleDisconnect(StringHash eventType, VariantMap & eventData)
{
	printf("HandleDisconnect has been pressed. \n");
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();

	// Running as Client
	if (serverConnection)
	{
		serverConnection->Disconnect();
		scene_->Clear(true, false);
		clientObjectID_ = 0;
		CreateScene();

		// Set button visibilty
		gameModeNetwork = false;
		pStart_->SetVisible(true);
		pLineEdit_->SetVisible(true);
		pDisconnect_->SetVisible(false);
		pStartServer_->SetVisible(true);
		pConnect_->SetVisible(true);
	}

	// Running as a server, stop it
	else if (network->IsServerRunning())
	{
		network->StopServer();
		scene_->Clear(true, false);
		gameModeServer = false;
		CreateScene();

		// Set button visibilty
		pStart_->SetVisible(true);
		pConnect_->SetVisible(true);
		pDisconnect_->SetVisible(false);
		pStartServer_->SetVisible(true);
		pLineEdit_->SetVisible(true);

		// Set window visibility
		window_->SetVisible(menuVisible_);

		// Set the cursor visibility
		GetSubsystem<UI>()->GetCursor()->SetVisible(menuVisible_);

		// Hide the background
		uiRoot_->GetChild("background", false)->SetVisible(menuVisible_);
	}
}

// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------





// ----------------------------------------------------------------------------------------------
// --------------------------------------- NETWORK CONTROLS -------------------------------------
// ----------------------------------------------------------------------------------------------

// Clients controls to the server
Controls MainGame::FromClientToServerControls()
{
	// Get the input system and controls
	UI* ui = GetSubsystem<UI>();
	Input* input = GetSubsystem<Input>();
	Controls controls;

	// Only apply WASD controls if there is no focused UI element
	if (!ui->GetFocusElement())
	{
		// Check which button has been pressed, keep track
		controls.Set(CTRL_FORWARD,	input->GetKeyDown(KEY_W));
		controls.Set(CTRL_BACK,		input->GetKeyDown(KEY_S));
		controls.Set(CTRL_LEFT,		input->GetKeyDown(KEY_A));
		controls.Set(CTRL_RIGHT,	input->GetKeyDown(KEY_D));
		controls.Set(CTRL_FIRE,		input->GetMouseButtonDown(MOUSEB_LEFT));
	}

	// mouse yaw to server
	controls.yaw_ = yaw_;
	controls.pitch_ = pitch_;

	// Return controls
	return controls;
}


// Process the clients controls
void MainGame::ProcessClientControls()
{
	// Network connection
	Network* network = GetSubsystem<Network>();
	const Vector<SharedPtr<Connection>>& connections = network->GetClientConnections();

	//Server: go through every client connected
	for (unsigned i = 0; i < connections.Size(); ++i)
	{
		// The connection
		Connection* connection = connections[i];
		UI* ui = GetSubsystem<UI>();

		// Get the object this connection is controlling
		Node* player = serverObjects_[connection];
		MissileSet* missileSet = serverMissileSets_[connection];

		// Client has no item connected
		if (!player) continue;
		RigidBody* rigidbody = player->GetComponent<RigidBody>();

		// Get the last controls sent by the client
		const Controls& controls = connection->GetControls();

		// The force applied to the player
		Vector3 force = Vector3::FORWARD;

		// Reset mouse controls
		float roll = 0.0f;

		// Set the speeds
		float speed = speed_;
		float rotationSpeed = rotationSpeed_;

		// Set speed and rotation speed
		if (controls.buttons_ & CTRL_FORWARD)
		{
			speed *= speedMultiplier_;
			rotationSpeed *= speedMultiplier_;
		}

		// Brake speed
		else if (controls.buttons_ & CTRL_BACK)
		{
			speed /= speedMultiplier_;
			rotationSpeed /= speedMultiplier_;
		}

		// Set force and roll
		if (controls.buttons_ & CTRL_LEFT)	roll -= 1.0f * rotationSpeed;
		if (controls.buttons_ & CTRL_RIGHT)	roll += 1.0f * rotationSpeed;

		// Apply forces
		rigidbody->ApplyTorque(rigidbody->GetRotation() * Vector3(controls.pitch_, controls.yaw_, roll));
		rigidbody->ApplyForce(rigidbody->GetRotation() * force * speed);

		// Fire missile
		if (controls.buttons_ & CTRL_FIRE && player->GetVar("FireTimer").GetFloat() <= 0.0f)
		{
			// Fire missile
			missileSet->Shoot(rigidbody->GetRotation() * Vector3::FORWARD, nullptr);
			player->SetVar("FireTimer", fireTimerReset_);
		}

		// Update missiles
		missileSet->Update();
	}
}


// Physics pre-step
void MainGame::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();

	// Using the update namespace
	using namespace Update;

	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	// Client: collect controls
	if (serverConnection)
	{
		// send camera position too
		serverConnection->SetPosition(cameraNode_->GetPosition());

		// send controls to server
		serverConnection->SetControls(FromClientToServerControls());
	}

	// Server: Read Controls, Apply them if needed
	else if (network->IsServerRunning())
	{
		// take data from clients, process it
		ProcessClientControls();
	}
}

// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------





// ----------------------------------------------------------------------------------------------
// ---------------------------------------- NETWORK EVENTS --------------------------------------
// ----------------------------------------------------------------------------------------------

// Finished loading client
void MainGame::HandleClientFinishedLoading(StringHash eventType, VariantMap & eventData)
{
	printf("(HandleClientFinishedLoading) Client has finished loading up the scene from the server \n");
}


// Client object ID
void MainGame::HandleServerToClientObjectID(StringHash eventType, VariantMap& eventData)
{
	clientObjectID_ = eventData[PLAYER_ID].GetUInt();
	printf("Client ID : %i \n", clientObjectID_);
}


// Client ready to start game
void MainGame::HandleClientToServerReadyToStart(StringHash eventType, VariantMap& eventData)
{
	printf("Event sent by the Client and running on Server: Client is ready to start the game \n");
	using namespace ClientConnected;

	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	// Create a controllable object for that client
	Node* player = CreatePlayer();
	serverObjects_[newConnection] = player;

	// Missiles
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	MissileSet* missileSet = new MissileSet();
	missileSet->Initialise(cache, scene_, player);
	serverMissileSets_[newConnection] = missileSet;

	// Finally send the object's node ID using a remote event
	VariantMap remoteEventData;
	remoteEventData[PLAYER_ID] = player->GetID();
	newConnection->SendRemoteEvent(E_CLIENTOBJECTID, true, remoteEventData);
}


// Client has connected and started game
void MainGame::HandleClientStartGame(StringHash eventType, VariantMap& eventData)
{
	printf("Client has pressed START GAME \n");

	// Client is still observer
	if (clientObjectID_ == 0)
	{
		// Connection
		Network* network = GetSubsystem<Network>();
		Connection* serverConnection = network->GetServerConnection();
		if (serverConnection)
		{
			VariantMap remoteEventData;
			remoteEventData[PLAYER_ID] = 0;
			serverConnection->SendRemoteEvent(E_CLIENTISREADY, true, remoteEventData);
			menuVisible_ = !menuVisible_;
		}
	}

	// Show controls text
	UIElement* controls = uiRoot_->GetChild(CONTROLS);
	controls->SetVisible(true);

	// Set button visibilty
	pStartClient_->SetVisible(false);
	pDisconnect_->SetVisible(true);
	pLineEdit_->SetVisible(false);

	// Set the cursor visibility
	GetSubsystem<UI>()->GetCursor()->SetVisible(menuVisible_);

	// Hide the background
	uiRoot_->GetChild("background", false)->SetVisible(menuVisible_);

	// Initialise the UI
	InitUI();
}

// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------