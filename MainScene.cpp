#include <iostream>
#include <string>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/AudioEvents.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/UIEvents.h>

#include <Urho3D/DebugNew.h>

#include "Character.h"
#include "MainScene.h"
#include "Touch.h"

URHO3D_DEFINE_APPLICATION_MAIN(MainScene)

const unsigned NUM_FLOOR = 10;
const unsigned NUM_BOXES = 10;
const unsigned NUM_CARROTS = 10;

MainScene::MainScene(Context* context) :
	App(context), 
	time_(0), 
	level_(0), 
	currentLevel_(0),
	musicSource_(0),
	gamePaused_(true), 
	gameOver_(false)
{
	// Register factory and attributes for the Character component so it can be created via CreateComponent, and loaded / saved
	Character::RegisterObject(context);
}

MainScene::~MainScene()
{
}

void MainScene::Start()
{
	App::Start();

	if (touchEnabled_)
		touch_ = new Touch(context_, TOUCH_SENSITIVITY);

	CreateUI();
	

}


void MainScene::PlayGame(StringHash eventType, VariantMap& eventData) {
	if (gameOver_ == true) {
		scene_->SetUpdateEnabled(false);
		scene_->Clear(true, true);
		scene_->Remove();
		
		time_ = 0.0;
		
		gameOver_ = false;
	}
	level_ = 0;
	currentLevel_ = 0;
	CreateScene();

	scene_->SetUpdateEnabled(true);

	gamePaused_ = false;


	UI* ui = GetSubsystem<UI>();
	ui->GetRoot()->RemoveAllChildren();
	//ui->SetCursor(0);

	CreateCharacter();

	SubscribeToEvents();

	App::InitMouseMode(MM_RELATIVE);

	CreateText();


}

void MainScene::QuitGame(StringHash eventType, VariantMap& eventData) {
	engine_->Exit();
}

void MainScene::CreateUI()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();

	// Set up global UI style into the root UI element
	XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	ui->GetRoot()->SetDefaultStyle(style);

	// Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
	// control the camera, and when visible, it will interact with the UI
	SharedPtr<Cursor> cursor(new Cursor(context_));
	cursor->SetStyleAuto();
	ui->SetCursor(cursor);
	// Set starting position of the cursor at the rendering window center
	Graphics* graphics = GetSubsystem<Graphics>();
	cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

	// Load UI content prepared in the editor and add to the UI hierarchy
	SharedPtr<UIElement> layoutRoot = ui->LoadLayout(cache->GetResource<XMLFile>("bin/Data/UI/menuGry.xml"));
	ui->GetRoot()->AddChild(layoutRoot);
	// Subscribe to button actions (toggle scene lights when pressed then released)
	
	Button* button = layoutRoot->GetChildStaticCast<Button>("PlayGame", true);
	if (button)
		SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(MainScene, PlayGame));
	button = layoutRoot->GetChildStaticCast<Button>("Quit", true);
	if (button)
		SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(MainScene, QuitGame));
}


void MainScene::CreateText()
{
	// CZAS
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	GetSubsystem<UI>()->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
	// Let's create some text to display.
	text_ = new Text(context_);
	// Text will be updated later in the E_UPDATE handler. Keep readin'.
	text_->SetText("Score ");
	// If the engine cannot find the font, it comes with Urho3D.
	// Set the environment variables URHO3D_HOME, URHO3D_PREFIX_PATH or
	// change the engine parameter "ResourcePrefixPath" in the Setup method.
	text_->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.ttf"), 20);
	text_->SetColor(Color(255, 0, 0));
	text_->SetHorizontalAlignment(HA_LEFT);
	text_->SetVerticalAlignment(VA_TOP);
	GetSubsystem<UI>()->GetRoot()->AddChild(text_);

	textCollectible_ = new Text(context_);
	// Text will be updated later in the E_UPDATE handler. Keep readin'.
	textCollectible_->SetText("Items ");
	// If the engine cannot find the font, it comes with Urho3D.
	// Set the environment variables URHO3D_HOME, URHO3D_PREFIX_PATH or
	// change the engine parameter "ResourcePrefixPath" in the Setup method.
	textCollectible_->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.ttf"), 20);
	textCollectible_->SetColor(Color(255, 255, 255));
	textCollectible_->SetHorizontalAlignment(HA_RIGHT);
	textCollectible_->SetVerticalAlignment(VA_TOP);
	GetSubsystem<UI>()->GetRoot()->AddChild(textCollectible_);

	// POZYCJA BOHATERA

	text2_ = new Text(context_);
	// Text will be updated later in the E_UPDATE handler. Keep readin'.
	text2_->SetText("POS ");
	// If the engine cannot find the font, it comes with Urho3D.
	// Set the environment variables URHO3D_HOME, URHO3D_PREFIX_PATH or
	// change the engine parameter "ResourcePrefixPath" in the Setup method.
	text2_->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.ttf"), 20);
	text2_->SetColor(Color(1, 0, 0));
	text2_->SetHorizontalAlignment(HA_RIGHT);
	text2_->SetVerticalAlignment(VA_BOTTOM);
	GetSubsystem<UI>()->GetRoot()->AddChild(text2_);
}
void MainScene::CreateScene()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	scene_ = new Scene(context_);

	// Create scene subsystem components
	scene_->CreateComponent<Octree>();
	scene_->CreateComponent<PhysicsWorld>();
	scene_->CreateComponent<DebugRenderer>();

	// Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
	// so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
	cameraNode_ = new Node(context_);
	Camera* camera = cameraNode_->CreateComponent<Camera>();
	camera->SetFarClip(300.0f);
	GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

	// Create static scene content. First create a zone for ambient lighting and fog control
	Node* zoneNode = scene_->CreateChild("Zone");
	Zone* zone = zoneNode->CreateComponent<Zone>();
	zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
	zone->SetFogColor(Color(0.6f, 0.5f, 0.5f));
	zone->SetFogStart(20.0f);
	zone->SetFogEnd(100.0f);
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	// Create a directional light with cascaded shadow mapping
	Node* lightNode = scene_->CreateChild("DirectionalLight");
	lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
	Light* light = lightNode->CreateComponent<Light>();
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
	light->SetSpecularIntensity(0.5f);

	// SKY
	Node* skyNode = scene_->CreateChild("Sky");
	skyNode->SetScale(100.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/SkyboxMeadow.xml"));



	Node* floorNodeX = scene_->CreateChild("Floor");
	floorNodeX->SetPosition(Vector3(0.0f, -0.5f, 5.0f - 10.0f));
	floorNodeX->SetScale(Vector3(9.0f, 1.0f, 10.0f));
	StaticModel* objectX = floorNodeX->CreateComponent<StaticModel>();
	objectX->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	objectX->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/Path/pathM2.xml"));

	RigidBody* bodyX = floorNodeX->CreateComponent<RigidBody>();
	// Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
	// inside geometry
	bodyX->SetCollisionLayer(2);
	CollisionShape* shapeX = floorNodeX->CreateComponent<CollisionShape>();
	shapeX->SetBox(Vector3::ONE);


	CreateFloor(cache, level_);
	CreateCollectibles(cache, level_);
	//CreateObstacles(cache, level_);

	PlayMusic(cache);


	Node* efekt = scene_->CreateChild("Efekt");
	efekt->SetPosition(Vector3(0.0f, 1.0f, 100.0f));
	efekt->SetScale(Vector3(10.0f, 10.0f, 1.0f));
	ParticleEmitter* emitter = efekt->CreateComponent<ParticleEmitter>();
	emitter->SetEffect(cache->GetResource<ParticleEffect>("bin/Data/Particle/Dust.xml"));

	StaticModel* object = efekt->CreateComponent<StaticModel>(LOCAL);
	//object->duration = 10.f;
}

void MainScene::PlayMusic(ResourceCache* cache) {
	// Create music sound source
	musicSource_ = scene_->CreateComponent<SoundSource>();
	// Set the sound type to music so that master volume control works correctly
	musicSource_->SetSoundType(SOUND_MUSIC);

	Sound* music = cache->GetResource<Sound>("bin/Data/Sounds/Escape.wav");
	// Set the song to loop
	music->SetLooped(true);

	musicSource_->Play(music);
	musicSource_->SetGain(0.25f);
}

void MainScene::PlaySound(ResourceCache* cache, int type) {
	Sound* sound;
	if (type == 0) {
		sound = cache->GetResource<Sound>("bin/Data/Sounds/collect.wav");
	}
	else if (type == 1) {
		sound = cache->GetResource<Sound>("bin/Data/Sounds/Hard_hit.wav");
	}

	if (sound)
	{
		SoundSource* soundSource = scene_->CreateComponent<SoundSource>();
		// Component will automatically remove itself when the sound finished playing
		soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
		soundSource->Play(sound);
		//soundSource->SetGain(0.75f);
	}
}

void MainScene::CreateFloor(ResourceCache* cache, int level) {

	// Create the floor object
	for (unsigned i = 0; i < NUM_FLOOR; i++)
	{
		Node* floorNode = scene_->CreateChild("Floor" + level);
		floorNode->SetPosition(Vector3(0.0f, -0.5f, 5.0f + 10.0f * i + 100.0f * level));
		floorNode->SetScale(Vector3(9.0f, 1.0f, 10.0f));
		StaticModel* object = floorNode->CreateComponent<StaticModel>();
		object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		object->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/Path/pathM2.xml"));

		RigidBody* body = floorNode->CreateComponent<RigidBody>();
		// Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
		// inside geometry
		body->SetCollisionLayer(2);
		CollisionShape* shape = floorNode->CreateComponent<CollisionShape>();
		shape->SetBox(Vector3::ONE);

		Node* landscapeNode = scene_->CreateChild("LandscapeRight"+level);
		landscapeNode->SetPosition(Vector3(10.0f / 2 + 4.5f, -0.5f, 5.0f + 10.0f * i + 100.0f * level));
		landscapeNode->SetScale(Vector3(10.0f, 1.0f, 10.0f));
		StaticModel* landscapeObject = landscapeNode->CreateComponent<StaticModel>();
		landscapeObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		landscapeObject->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/Path/grass.xml"));
		Node* landscapeNode2 = scene_->CreateChild("LandscapeRight"+ level);
		landscapeNode2->SetPosition(Vector3(10.0f / 2 + 10.0f + 4.5f, -0.5f, 5.0f + 10.0f * i + 100.0f * level));
		landscapeNode2->SetScale(Vector3(10.0f, 1.0f, 10.0f));
		StaticModel* landscapeObject2 = landscapeNode2->CreateComponent<StaticModel>();
		landscapeObject2->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		landscapeObject2->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/Path/grass.xml"));

		float randomScale = Random(0.5f) + 1.0f;
		float randomTree = Random(2.0f);
		Node* treeNode = scene_->CreateChild("Tree" + level);
		treeNode->SetPosition(Vector3(Random(8.0f) + 6.0f, -0.5f, 5.0f + 10.0f * i + 100.0f * level));
		treeNode->SetScale(Vector3(randomScale, randomScale, randomScale));
		StaticModel* treeObject = treeNode->CreateComponent<StaticModel>();
		if (randomTree < 0.5f) {
			treeObject->SetModel(cache->GetResource<Model>("bin/Data/Models/tree3/tree.mdl"));
			treeObject->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/torch_wood.xml"));
			treeObject->SetCastShadows(true);
			treeObject = treeNode->CreateComponent<StaticModel>();
			treeObject->SetModel(cache->GetResource<Model>("bin/Data/Models/tree3/leaves.mdl"));
			treeObject->SetMaterial(cache->GetResource<Material>("bin/Data/Models/tree3/Material.004.xml"));
			treeObject->SetCastShadows(true);
		}
		else {
			treeObject->SetModel(cache->GetResource<Model>("bin/Data/Models/tree2/tree.mdl"));
			treeObject->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/torch_wood.xml"));
			treeObject->SetCastShadows(true);
			treeObject = treeNode->CreateComponent<StaticModel>();
			treeObject->SetModel(cache->GetResource<Model>("bin/Data/Models/tree2/leaves.mdl"));
			treeObject->SetMaterial(cache->GetResource<Material>("bin/Data/Models/tree3/Material.004.xml"));
			treeObject->SetCastShadows(true);

		}
		
		Node* landscapeNodeLeft = scene_->CreateChild("LandscapeLeft"+level);
		landscapeNodeLeft->SetPosition(Vector3(-(10.0f / 2 + 4.5f), -0.5f, 5.0f + 10.0f * i + 100.0f * level));
		landscapeNodeLeft->SetScale(Vector3(10.0f, 1.0f, 10.0f));
		StaticModel* landscapeObjectLeft = landscapeNodeLeft->CreateComponent<StaticModel>();
		landscapeObjectLeft->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		landscapeObjectLeft->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/Path/grass.xml"));
		Node* landscapeNodeLeft2 = scene_->CreateChild("LandscapeLeft"+ level);
		landscapeNodeLeft2->SetPosition(Vector3(-(10.0f / 2 + 10.0f + 4.5f), -0.5f, 5.0f + 10.0f * i + 100.0f * level));
		landscapeNodeLeft2->SetScale(Vector3(10.0f, 1.0f, 10.0f));
		StaticModel* landscapeObjectLeft2 = landscapeNodeLeft2->CreateComponent<StaticModel>();
		landscapeObjectLeft2->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		landscapeObjectLeft2->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/Path/grass.xml"));


		float randomScale2 = Random(0.2f) + 1.0f;
		Node* treeNode2 = scene_->CreateChild("Tree" + level);
		treeNode2->SetPosition(Vector3(-(Random(8.0f) + 6.0f), -0.5f, 5.0f + 10.0f * i + 100.0f * level));
		treeNode2->SetScale(Vector3(randomScale, randomScale, randomScale));
		StaticModel* treeObject2 = treeNode2->CreateComponent<StaticModel>();
		treeObject2->SetModel(cache->GetResource<Model>("bin/Data/Models/tree2/tree.mdl"));
		treeObject2->SetMaterial(cache->GetResource<Material>("bin/Data/Materials/torch_wood.xml"));
		treeObject2->SetCastShadows(true);
		treeObject2 = treeNode2->CreateComponent<StaticModel>();
		treeObject2->SetModel(cache->GetResource<Model>("bin/Data/Models/tree2/leaves.mdl"));
		treeObject2->SetMaterial(cache->GetResource<Material>("bin/Data/Models/tree2/Material.004.xml"));
		treeObject2->SetCastShadows(true);

		/////////////////////////////////////
		Node* leftWallNode = scene_->CreateChild("LeftWall" + level);
		leftWallNode->SetPosition(Vector3(-4.0f, 2.0f, 5.0f + 10.0f * i + 100.0f * level));
		leftWallNode->SetScale(Vector3(1.0f, 4.0f, 10.0f));
		StaticModel* object2 = leftWallNode->CreateComponent<StaticModel>();
		object2->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		object2->SetMaterial(cache->GetResource<Material>("Materials/Smoke.xml"));

		RigidBody* body2 = leftWallNode->CreateComponent<RigidBody>();
		body2->SetCollisionLayer(2);
		CollisionShape* shape2 = leftWallNode->CreateComponent<CollisionShape>();
		shape2->SetBox(Vector3::ONE);

		Node* rightWallNode = scene_->CreateChild("RightWall" + level);
		rightWallNode->SetPosition(Vector3(4.0f, 2.0f, 5.0f + 10.0f * i + 100.0f * level));
		rightWallNode->SetScale(Vector3(1.0f, 4.0f, 10.0f));
		StaticModel* object3 = rightWallNode->CreateComponent<StaticModel>();
		object3->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		object3->SetMaterial(cache->GetResource<Material>("Materials/Smoke.xml"));

		RigidBody* body3 = rightWallNode->CreateComponent<RigidBody>();
		body3->SetCollisionLayer(2);
		CollisionShape* shape3 = rightWallNode->CreateComponent<CollisionShape>();
		shape3->SetBox(Vector3::ONE);
	}


}


void MainScene::DeleteFloor(int level) {
	for (int i = 0; i < NUM_FLOOR; i++) {
		Node* removeNode = scene_->GetChild("Floor" + level);
		scene_->RemoveChild(removeNode);
		removeNode = scene_->GetChild("LandscapeRight" + level);
		scene_->RemoveChild(removeNode);
		removeNode = scene_->GetChild("LandscapeLeft" + level);
		scene_->RemoveChild(removeNode);
		removeNode = scene_->GetChild("LeftWall" + level);
		scene_->RemoveChild(removeNode);
		removeNode = scene_->GetChild("RightWall" + level);
		scene_->RemoveChild(removeNode);
	}
	for (int i = 0; i < NUM_BOXES; i++) {
		Node* removeNode = scene_->GetChild("Box" + level);
		scene_->RemoveChild(removeNode);
	}
	for (int i = 0; i < NUM_CARROTS; i++) {
		Node* removeNode = scene_->GetChild("Carrot" + level);
		scene_->RemoveChild(removeNode);
	}
	
}

void MainScene::CreateObstacles(ResourceCache* cache, int level) {


	for (unsigned i = 0; i < NUM_BOXES; ++i)
	{
		Node* objectNode = scene_->CreateChild("Box" + level);
		objectNode->SetPosition(Vector3((int(Random(3.0f)) - 1.0f) * 2.5, 0.0f, int(i * 100/NUM_BOXES) + 100.0f * level));
		std::cout << objectNode->GetPosition().z_ << std::endl;
		//objectNode->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
		//objectNode->SetScale(Vector3(1.5f, 1.5f, 0.2f));
		StaticModel* object5 = objectNode->CreateComponent<StaticModel>();
		object5->SetModel(cache->GetResource<Model>("bin/Data/Models/skala/Models/Skala_low_poly_B.mdl"));
		object5->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
		object5->SetCastShadows(true);

		RigidBody* body5 = objectNode->CreateComponent<RigidBody>();
		body5->SetCollisionLayer(3);
		// Bigger boxes will be heavier and harder to move
		//body5->SetMass(20.0f);
		CollisionShape* shape5 = objectNode->CreateComponent<CollisionShape>();
		shape5->SetBox(Vector3::ONE);
	}
}

void MainScene::CreateCollectibles(ResourceCache* cache, int level) {
	for (unsigned i = 0; i < NUM_CARROTS; ++i)
	{

		Node* carrotNode = scene_->CreateChild("Carrot" + level);
		carrotNode->SetPosition(Vector3((int(Random(3.0f)) - 1.0f) * 2.5, 1.5f, int(i * 100 / NUM_BOXES) + 100.0f * level));
		carrotNode->SetRotation(Quaternion(0.0f, 0.0f, 160.0f));
		carrotNode->SetScale(0.2f);
		StaticModel* carrot = carrotNode->CreateComponent<StaticModel>();
		carrot->SetModel(cache->GetResource<Model>("bin/Data/Models/marchewka/Models/marchewka.mdl"));
		carrot->SetMaterial(cache->GetResource<Material>("bin/Data/Models/marchewka/Materials/orange.xml"));
		carrot->SetCastShadows(true);

		RigidBody* carrotBody = carrotNode->CreateComponent<RigidBody>();
		carrotBody->SetCollisionLayer(4);
		carrotBody->SetTrigger(true);
		CollisionShape* carrotShape = carrotNode->CreateComponent<CollisionShape>();
		carrotShape->SetBox(Vector3::ONE);
	}	

}
void MainScene::CreateCharacter() {
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	Node* objectNode = scene_->CreateChild("Jack");
	objectNode->SetPosition(Vector3(0.0f, 1.1f, 0.0f));
	objectNode->SetScale(Vector3(0.6f, 0.6f, 0.6f));
	//objectNode->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));

	// Create the rendering component + animation controller
	AnimatedModel* object = objectNode->CreateComponent<AnimatedModel>();
	object->SetModel(cache->GetResource<Model>("bin/Data/Models/kach/Kachujin.mdl"));
	object->SetMaterial(cache->GetResource<Material>("bin/Data/Models/Kachujin/Materials/kachujin.mdl"));
	object->SetCastShadows(true);
	objectNode->CreateComponent<AnimationController>();

	// Set the head bone for manual control
	//object->GetSkeleton().GetBone("Mutant:Head")->animated_ = false;

	// Create rigidbody, and set non-zero mass so that the body becomes dynamic
	RigidBody* body = objectNode->CreateComponent<RigidBody>();
	body->SetCollisionLayer(1);
	body->SetMass(15.0f);

	// Set zero angular factor so that physics doesn't turn the character on its own.
	// Instead we will control the character yaw manually
	body->SetAngularFactor(Vector3::ZERO);

	// Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
	body->SetCollisionEventMode(COLLISION_ALWAYS);

	// Set a capsule shape for collision
	CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
	///shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f));
	shape->SetCapsule(1.9f, 2.8f, Vector3(0.0f, 1.2f, 0.0f));

	// Create the character logic component, which takes care of steering the rigidbody
	// Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
	// and keeps it alive as long as it's not removed from the hierarchy
	character_ = objectNode->CreateComponent<Character>();
	//////////////////

	File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "bin/Data/Scenes/GameScene.xml", FILE_WRITE);
	scene_->SaveXML(saveFile);
	std::cout << "SAAAAAAAAAAAAAAAVE" << std::endl;
}


void MainScene::SubscribeToEvents()
{
	// Subscribe to Update event for setting the character controls before physics simulation
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MainScene, HandleUpdate));

	// Subscribe to PostUpdate event for updating the camera position after physics simulation
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(MainScene, HandlePostUpdate));
	SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(MainScene, HandlePostRenderUpdate));
	// Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample

	//SubscribeToEvent(GetNode(), E_NODECOLLISIONSTART, HANDLER(MyLogicObject, HandleNodeCollisionStart));

	UnsubscribeFromEvent(E_SCENEUPDATE);
}
void MainScene::GameOver(){
	////////////// GAME OVER /////////////////////
	musicSource_->Stop();
	scene_->SetUpdateEnabled(false);
	gameOver_ = true;
	
	//std::cout << character_->gameOver_ << std::endl;
	UI* ui = GetSubsystem<UI>();

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	ui->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
	gameOverText_ = new Text(context_);
	gameOverText_->SetText("Game Over");
	gameOverText_->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.ttf"), 80);
	gameOverText_->SetColor(Color(1, 1, 1));
	gameOverText_->SetHorizontalAlignment(HA_CENTER);
	gameOverText_->SetVerticalAlignment(VA_CENTER);
	GetSubsystem<UI>()->GetRoot()->AddChild(gameOverText_);

	// Load UI content prepared in the editor and add to the UI hierarchy
	SharedPtr<UIElement> layoutRoot = ui->LoadLayout(cache->GetResource<XMLFile>("bin/Data/UI/menuGry.xml"));
	ui->GetRoot()->AddChild(layoutRoot);
	// Subscribe to button actions (toggle scene lights when pressed then released)

	Button* button = layoutRoot->GetChildStaticCast<Button>("PlayGame", true);
	if (button)
		SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(MainScene, PlayGame));
	button = layoutRoot->GetChildStaticCast<Button>("Quit", true);
	if (button)
		SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(MainScene, QuitGame));
}

void MainScene::UpdateScore() {
	// update wyswietlanego score
	time_ += 0.01;
	std::string str;
	str.append("Score: ");
	str.append(std::to_string(int(time_ * 10) + collected_ * 1000));
	str.append(" Level: ");
	str.append(std::to_string(level_));
	str.append(" Current Level: ");
	str.append(std::to_string(currentLevel_));
	String s(str.c_str(), str.size());
	text_->SetText(s);
}

void MainScene::UpdateCollected() {
	// update wyswietlanego score
	collected_ = character_->collected_;
	std::string str;
	str.append("Items: ");
	str.append(std::to_string(collected_));
	String s(str.c_str(), str.size());
	textCollectible_->SetText(s);
}

void MainScene::CreateNewObstacles() {
	if (character_)
	{
		ResourceCache* cache = GetSubsystem<ResourceCache>();
		Node* characterNode = character_->GetNode();





		////////////////// Wodne szesciany
		if (characterNode->GetPosition().z_ > 10 && characterNode->GetPosition().z_ < 11)
		{

			Node* objectNode2 = scene_->CreateChild("Boxx");
			objectNode2->SetPosition(Vector3((int(Random(3.0f) - 1.0f))*3.0f - 3.0f, 1.75f, 40.0f));
			//objectNode2->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
			objectNode2->SetScale(1.5f);
			StaticModel* object52 = objectNode2->CreateComponent<StaticModel>();
			object52->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
			object52->SetMaterial(cache->GetResource<Material>("Materials/Water.xml"));
			object52->SetCastShadows(true);

		}

		if (characterNode->GetPosition().z_ > 20) {
			scene_->RemoveChild(scene_->GetChild("Boxx", true));
		}
	}
}

void MainScene::Collect() {

}

void MainScene::HandleUpdate(StringHash eventType, VariantMap& eventData)
{

	using namespace Update;

	Input* input = GetSubsystem<Input>();


	if (character_)
	{
		ResourceCache* cache = GetSubsystem<ResourceCache>();
		Node* characterNode = character_->GetNode();

		if (character_->gameOver_ == true) {
			PlaySound(cache, 1);
			GameOver();
			character_->gameOver_ = false;
		}
		else {
			Node* efekt = scene_->GetChild("Efekt");
			efekt->SetPosition(Vector3(0.0f, 0.0f, characterNode->GetPosition().z_ + 99.0f));
			


			if (character_->playCollectSound_ == true) {
				PlaySound(cache, 0);
				character_->playCollectSound_ = false;
			}
			//// Usuwanie sciezki, ktora bohater juz przeszedl
			if (characterNode->GetPosition().z_ > 100.0f * (currentLevel_+ 1)) {
				DeleteFloor(currentLevel_);
				
				currentLevel_ += 1;
				character_->speed_ += 0.1;
				
			}
			//// Tworzenie nowej œcie¿ki
			if (characterNode->GetPosition().z_ >= 100.0f * (level_) + 20.0f) {
				//level_ += 1;
				CreateFloor(cache, level_+ 1);
				CreateCollectibles(cache, level_+ 1);
				CreateObstacles(cache, level_+1);
				level_ += 1;
			}

			UpdateScore();
			UpdateCollected();

			// update wyswietlanej pozycji bohatera
			std::string str2;
			str2.append("posX: ");
			str2.append(std::to_string(int(characterNode->GetPosition().x_)));
			str2.append(" posY: ");
			str2.append(std::to_string(int(characterNode->GetPosition().y_)));
			str2.append(" posZ: ");
			str2.append(std::to_string(int(characterNode->GetPosition().z_)));
			String s2(str2.c_str(), str2.size());
			text2_->SetText(s2);
		}


		CreateNewObstacles();



		// Clear previous controls
		character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

		// Update controls using touch utility class
		if (touch_)
			touch_->UpdateTouches(character_->controls_);




		// Update controls using keys
		UI* ui = GetSubsystem<UI>();
		if (!ui->GetFocusElement())
		{
			if (!touch_ || !touch_->useGyroscope_)
			{

				character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
				character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_DOWN));
				character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_LEFT));
				character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_RIGHT));
			}
			character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_UP));

			
			////////////////// AUTO chodzenie do przodu
			////////////character_->controls_.Set(CTRL_FORWARD);

			// Limit pitch
			//character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
			// Set rotation already here so that it's updated every rendering frame instead of every physics frame
			character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));


			// Turn on/off gyroscope on mobile platform
			if (touch_ && input->GetKeyPress(KEY_G))
				touch_->useGyroscope_ = !touch_->useGyroscope_;

			// Check for loading / saving the scene
			if (input->GetKeyPress(KEY_F5))
			{
				File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_WRITE);
				scene_->SaveXML(saveFile);
			}
			if (input->GetKeyPress(KEY_F7))
			{
				File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/CharacterDemo.xml", FILE_READ);
				scene_->LoadXML(loadFile);
				// After loading we have to reacquire the weak pointer to the Character component, as it has been recreated
				// Simply find the character's scene node by name as there's only one of them
				Node* characterNode = scene_->GetChild("Jack", true);
				if (characterNode)
					character_ = characterNode->GetComponent<Character>();
			}
		}

	}
}


void MainScene::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	if (!character_)
		return;

	Node* characterNode = character_->GetNode();

	
	// Get camera lookat dir from character yaw + pitch
	Quaternion rot = characterNode->GetRotation();
	Quaternion dir = rot * Quaternion(character_->controls_.pitch_+15.0f, Vector3::RIGHT);

	// Third person camera: position behind the character
	Vector3 aimPoint = characterNode->GetPosition() + rot * Vector3(0.0f, 2.8f, -1.1f);

	// Collide camera ray with static physics objects (layer bitmask 2) to ensure we see the character properly
	Vector3 rayDir = dir * Vector3::BACK;
	float rayDistance = touch_ ? touch_->cameraDistance_ : CAMERA_INITIAL_DIST;
	PhysicsRaycastResult result;
	scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(aimPoint, rayDir), rayDistance, 2);
	if (result.body_)
		rayDistance = Min(rayDistance, result.distance_);
	rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

	cameraNode_->SetPosition(aimPoint + rayDir * rayDistance);
	cameraNode_->SetRotation(dir);


}

void MainScene::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
	//GetSubsystem<Renderer>()->DrawDebugGeometry(true);
}

