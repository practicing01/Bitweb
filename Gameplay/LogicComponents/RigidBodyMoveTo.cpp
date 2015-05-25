/*
 * RigidBodyMoveTo.cpp
 *
 *  Created on: Dec 25, 2014
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Core/Variant.h>

#include "RigidBodyMoveTo.h"

RigidBodyMoveTo::RigidBodyMoveTo(Context* context) :
		LogicComponent(context)
{
	isMoving_ = false;
	// Only the physics update event is needed: unsubscribe from the rest for optimization
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void RigidBodyMoveTo::OnMoveToComplete()
{
	SendEvent(E_RIGIDBODYMOVETOCOMPLETE);
}

void RigidBodyMoveTo::MoveTo(Vector3 dest, float speed, bool stopOnCompletion)
{
	moveToSpeed_ = speed;
	moveToDest_ = dest;
	moveToLoc_ = node_->GetWorldPosition();
	moveToDir_ = dest - moveToLoc_;
	moveToDir_.Normalize();
	moveToTravelTime_ = (moveToDest_ - moveToLoc_).Length() / speed;
	moveToElapsedTime_ = 0.0f;
	moveToStopOnTime_ = stopOnCompletion;
	isMoving_ = true;
	node_->GetComponent<RigidBody>()->SetLinearVelocity(moveToDir_ * speed);

	if (node_->GetComponent<AnimationController>())//quick hack to deal with animating the pets
	{
		node_->GetComponent<AnimationController>()->PlayExclusive("Models/petRun.ani", 0, true, 0.0f);
		node_->GetComponent<AnimationController>()->SetStartBone("Models/petRun.ani", "PantherPelvis");

		Vector3 lookAtPos = dest;
		lookAtPos.y_ = moveToLoc_.y_;
		node_->LookAt(lookAtPos);
	}
}

void RigidBodyMoveTo::FixedUpdate(float timeStep)
{
	if (isMoving_ == true)
	{
		moveToElapsedTime_ += timeStep;
		if (moveToElapsedTime_ >= moveToTravelTime_)
		{
			moveToLoc_ = node_->GetWorldPosition();
			if (moveToLoc_ != moveToDest_ && (moveToLoc_ - moveToDest_).Length() > 0.5f)
			{
				MoveTo(moveToDest_, moveToSpeed_, moveToStopOnTime_);
				return;
			}
			isMoving_ = false;
			if (moveToStopOnTime_ == true)
			{
				node_->GetComponent<RigidBody>()->SetLinearVelocity(Vector3::ZERO);

				if (node_->GetComponent<AnimationController>())//quick hack to deal with animating the pets
				{
					node_->GetComponent<AnimationController>()->StopAll(0.0f);
				}
			}
			OnMoveToComplete();
		}
	}
}
