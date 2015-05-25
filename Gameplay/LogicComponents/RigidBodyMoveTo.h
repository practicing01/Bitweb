/*
 * RigidBodyMoveTo.h
 *
 *  Created on: Dec 25, 2014
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

static const StringHash E_RIGIDBODYMOVETOCOMPLETE("RigidBodyMoveToComplete");

class RigidBodyMoveTo: public LogicComponent
{
	OBJECT(RigidBodyMoveTo);
public:
	RigidBodyMoveTo(Context* context);
	virtual void FixedUpdate(float timeStep);
	void OnMoveToComplete();
	void MoveTo(Vector3 dest, float speed, bool stopOnCompletion);

	bool moveToStopOnTime_;
	bool isMoving_;
	float moveToSpeed_;
	float moveToTravelTime_;
	float moveToElapsedTime_;
	float inderp_;
	float remainingDist_;
	Vector3 moveToDest_;
	Vector3 moveToLoc_;
	Vector3 moveToDir_;

};
