#pragma once

#include <Urho3D/Input/Controls.h>
#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

const int CTRL_FORWARD = 1;
const int CTRL_BACK = 2;
const int CTRL_LEFT = 4;
const int CTRL_RIGHT = 8;
const int CTRL_JUMP = 16;

const float MOVE_FORCE = 1.6f;
const float INAIR_MOVE_FORCE = 0.5f;
const float BRAKE_FORCE = 0.2f;
const float JUMP_FORCE =80.0f;// 7.0f;
const float YAW_SENSITIVITY = 0.1f;
const float INAIR_THRESHOLD_TIME = 0.2f;



/// Character component, responsible for physical movement according to controls, as well as animation.
class Character : public LogicComponent
{
	URHO3D_OBJECT(Character, LogicComponent);

public:
	/// Construct.
	Character(Context* context);

	/// Register object factory and attributes.
	static void RegisterObject(Context* context);

	/// Handle startup. Called by LogicComponent base class.
	virtual void Start();
	/// Handle physics world update. Called by LogicComponent base class.
	virtual void FixedUpdate(float timeStep);

	/// Movement controls. Assigned by the main program each frame.
	Controls controls_;
	bool gameOver_;
	bool playCollectSound_;
	int collected_;

	float speed_;

private:
	/// Handle physics collision event.
	void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
	/// Grounded flag for movement.
	bool onGround_;
	/// Jump flag.
	bool okToJump_;
	/// In air timer. Due to possible physics inaccuracy, character can be off ground for max. 1/10 second and still be allowed to move.
	float inAirTimer_;

	bool onLeftLane_;
	bool onMiddleLane_;
	bool onRightLane_;


	
};
