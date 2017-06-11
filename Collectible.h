#pragma once

#include <Urho3D\Scene\LogicComponent.h>

using namespace Urho3D;

class Collectible : public LogicComponent
{
	URHO3D_OBJECT(Collectible, LogicComponent);

public:
	/// Construct.
	Collectible(Context* context);
	void Init(Vector3 spawnPosition);

	virtual void Update(float timeStep);
};