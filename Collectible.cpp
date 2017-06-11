
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Resource/ResourceCache.h>

#include "Collectible.h"

Collectible::Collectible(Context* context) : LogicComponent(context) {
	SetUpdateEventMask(USE_FIXEDUPDATE);
}
void Collectible::Init(Vector3 spawnPosition) {

}

void Collectible::Update(float timeStep) {

}