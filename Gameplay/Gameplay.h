/*
 * Gameplay.h
 *
 *  Created on: May 16, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include "../Urho3DPlayer.h"

using namespace Urho3D;

class Gameplay : public Object
{
	OBJECT(Gameplay);
public:
	Gameplay(Context* context, Urho3DPlayer* main);
	~Gameplay();

	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandleElementResize(StringHash eventType, VariantMap& eventData);
	void HandlePressed(StringHash eventType, VariantMap& eventData);
	void HandleReleased(StringHash eventType, VariantMap& eventData);
	void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
	void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);
	void HandleNodeCollisionStart(StringHash eventType, VariantMap& eventData);
	void HandleKeyDown(StringHash eventType, VariantMap& eventData);
	void HandleKeyUp(StringHash eventType, VariantMap& eventData);
	void HandleMouseDown(StringHash eventType, VariantMap& eventData);

	void MoveArcher();
	void RecursiveAnimate(Node* noed, String animation, char layer, bool loop, float fadeTime, bool exclusive, float speed);
	void RandomizeGates();
	void XorInnerGates();
	void XorOuterGates();
	void SpawnMonster();
	void MoveMonsters();
	void SpawnArrow();
	void SpawnPotion();
	void SpawnChest();
	void SpawnElf();
	void ShootArrow();

	Urho3DPlayer* main_;
	float elapsedTime_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> cells_;
	SharedPtr<Node> archer_;
	SharedPtr<Node> arrow_;
	SharedPtr<Node> potion_;
	SharedPtr<Node> chest_;
	SharedPtr<Node> elf_;
	SharedPtr<Node> topScore_;
	SharedPtr<Node> topScoreText_;
	SharedPtr<Node> scoreText_;
	SharedPtr<Node> archerHitArrow_;
	SharedPtr<Node> archerHitChest_;
	SharedPtr<Node> archerHitElf_;
	SharedPtr<Node> archerHitPotion_;
	SharedPtr<Node> arrowHitDog_;
	SharedPtr<Node> arrowHitWall_;
	SharedPtr<Node> dogHitArcher_;
	SharedPtr<Node> gateOpen_;
	SharedPtr<Node> shootArrow_;

	Vector<Node*> closedCells_;
	Vector<Node*> openCells_;
	Vector<Node*> baseMonsters_;
	Vector<Node*> spawnedMonsters_;
	Vector<Node*> spawnedArrows_;
	Vector<Node*> quiver_;

	bool wDown_;
	bool aDown_;
	bool sDown_;
	bool dDown_;
	bool invincible_;

	IntVector2 previousExtents_;

	float archerSpeed_;
	float randomizeGatesElapsedTime_;
	float randomizeGatesInterval_;
	float monsterSpawnElapsedTime_;
	float monsterSpawnInterval_;
	float monsterMoveElapsedTime_;
	float monsterMoveInterval_;
	float monsterSpeed_;
	float arrowSpawnElapsedTime_;
	float arrowSpawnInterval_;
	float arrowSpeed_;
	float invincibilityElapsedTime_;
	float invincibilityInterval_;

	int monsterCount_;
	int monsterMoveTurn_;
	int monsterMax_;
	int arrowCount_;
	int arrowMax_;
	int score_;

	char archerDir_;
};
