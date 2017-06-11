#pragma once

#include "App.h"

namespace Urho3D
{
	class Node;
	class Scene;
}

class Character;
class Touch;

class MainScene : public App
{
	URHO3D_OBJECT(MainScene, App);

public:
	SharedPtr<Text> text_;
	SharedPtr<Text> textCollectible_;
	SharedPtr<Text> text2_;
	SharedPtr<Text> gameOverText_;
	float time_;
	int collected_;
	float characterPositionX;
	float characterPositionZ;
	bool gamePaused_;

	MainScene(Context* context);
	~MainScene();

	virtual void Start();

private:
	void PlayGame(StringHash eventType, VariantMap& eventData);
	void QuitGame(StringHash eventType, VariantMap& eventData);
	// Utworzenie sceny
	void CreateScene();
	// Utworzenie bohatera
	void CreateCharacter();
	void CreateUI();
	void CreateText();
	void UpdateScore();
	void UpdateCollected();

	void SubscribeToEvents();

	/// Handle application update. Set controls to character.
	void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	/// Handle application post-update. Update camera position after character has moved.
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
	void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
	void GameOver();


	/// Touch utility object.
	SharedPtr<Touch> touch_;
	/// The controllable character component.
	WeakPtr<Character> character_;
};