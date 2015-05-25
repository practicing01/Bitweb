/*
 * Gameplay.cpp
 *
 *  Created on: May 16, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>

#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Graphics/Viewport.h>

#include "Gameplay.h"
#include "LogicComponents/RigidBodyMoveTo.h"

Gameplay::Gameplay(Context* context, Urho3DPlayer* main) :
    Object(context)
{
	main_ = main;
	elapsedTime_ = 0.0f;
	previousExtents_ = IntVector2(800, 600);

	context->RegisterFactory<RigidBodyMoveTo>();

	archerSpeed_ = 20.0f;
	monsterSpeed_ = 40.0f;

	wDown_ = false;
	aDown_ = false;
	sDown_ = false;
	dDown_ = false;
	invincible_ = false;

	randomizeGatesElapsedTime_ = 10.0f;
	randomizeGatesInterval_ = 10.0f;

	monsterSpawnElapsedTime_ = 5.0f;
	monsterSpawnInterval_ = 5.0f;

	monsterMoveElapsedTime_ = 0.0f;
	monsterMoveInterval_ = 1.0f;

	arrowSpawnElapsedTime_ = 5.0f;
	arrowSpawnInterval_ = 5.0f;

	invincibilityElapsedTime_ = 0.0f;
	invincibilityInterval_ = 10.0f;

	monsterMoveTurn_ = 0;
	monsterMax_ = 6;
	monsterCount_ = 0;

	arrowCount_ = 0;
	arrowMax_ = 5;

	scene_ = new Scene(context_);
	cameraNode_ = new Node(context_);

	File loadFile(context_,main_->filesystem_->GetProgramDir()
			+ "Data/Scenes/bitweb.xml", FILE_READ);
	scene_->LoadXML(loadFile);

	cameraNode_ = scene_->GetChild("camera");

	main_->viewport_ = new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>());
	main_->renderer_->SetViewport(0, main_->viewport_);
	main_->viewport_->SetScene(scene_);
	main_->viewport_->SetCamera(cameraNode_->GetComponent<Camera>());

	archer_ = scene_->GetChild("archer");
	cells_ = scene_->GetChild("cells");
	baseMonsters_.Push(scene_->GetChild("pet1"));
	baseMonsters_.Push(scene_->GetChild("pet2"));
	baseMonsters_.Push(scene_->GetChild("pet3"));
	baseMonsters_.Push(scene_->GetChild("pet4"));

	arrow_ = scene_->GetChild("quartz");
	potion_ = scene_->GetChild("potion");
	chest_ = scene_->GetChild("chest");
	elf_ = scene_->GetChild("elf");

	SpawnPotion();
	SpawnChest();
	SpawnElf();

	XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/TopScore.xml");
	topScore_ = scene_->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

	score_ = 0;

	topScoreText_ = scene_->GetChild("TopScore");
	topScoreText_->GetComponent<Text3D>()->SetText("Top Score " + String( topScore_->GetVar("TopScore").GetInt() ));
	topScoreText_->GetComponent<Text3D>()->SetWidth(12);
	topScoreText_->ApplyAttributes();

	scoreText_ = scene_->GetChild("Score");
	scoreText_->GetComponent<Text3D>()->SetText("The Score " + String( score_ ));
	scoreText_->GetComponent<Text3D>()->SetWidth(12);
	scoreText_->ApplyAttributes();

	archerDir_ = 1;//0=up,1=down,2=left,3=right

	archerHitArrow_ = scene_->GetChild("archerHitArrow");
	archerHitChest_ = scene_->GetChild("archerHitChest");
	archerHitElf_ = scene_->GetChild("archerHitElf");
	archerHitPotion_ = scene_->GetChild("archerHitPotion");
	arrowHitDog_ = scene_->GetChild("arrowHitDog");
	arrowHitWall_ = scene_->GetChild("arrowHitWall");
	dogHitArcher_ = scene_->GetChild("dogHitArcher");
	gateOpen_ = scene_->GetChild("gateOpen");
	shootArrow_ = scene_->GetChild("shootArrow");

	SubscribeToEvent(archer_->GetChild("archer"), E_NODECOLLISIONSTART, HANDLER(Gameplay, HandleNodeCollisionStart));

	SubscribeToEvent(E_RESIZED, HANDLER(Gameplay, HandleElementResize));

    //SubscribeToEvent(E_UPDATE, HANDLER(Gameplay, HandleUpdate));

    //SubscribeToEvent(E_POSTRENDERUPDATE, HANDLER(Gameplay, HandlePostRenderUpdate));

    SubscribeToEvent(E_PHYSICSPRESTEP, HANDLER(Gameplay, HandlePhysicsPreStep));

    SubscribeToEvent(E_KEYDOWN, HANDLER(Gameplay, HandleKeyDown));

    SubscribeToEvent(E_KEYUP, HANDLER(Gameplay, HandleKeyUp));

    SubscribeToEvent(E_MOUSEBUTTONDOWN, HANDLER(Gameplay, HandleMouseDown));
}

Gameplay::~Gameplay()
{
}

void Gameplay::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
	//main_->renderer_->DrawDebugGeometry(true);
	scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}

void Gameplay::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	float timeStep = eventData[P_TIMESTEP].GetFloat();

	//LOGERRORF("client loop");
	elapsedTime_ += timeStep;
}

void Gameplay::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
	using namespace PhysicsPreStep;

	float timeStep = eventData[P_TIMESTEP].GetFloat();

	MoveArcher();

	randomizeGatesElapsedTime_ += timeStep;

	if (randomizeGatesElapsedTime_ >= randomizeGatesInterval_)
	{
		randomizeGatesElapsedTime_ = 0.0f;
		RandomizeGates();
	}

	monsterSpawnElapsedTime_ += timeStep;

	if (monsterSpawnElapsedTime_ >= monsterSpawnInterval_)
	{
		monsterSpawnElapsedTime_ = 0.0f;
		SpawnMonster();
	}

	monsterMoveElapsedTime_ += timeStep;

	if (monsterMoveElapsedTime_ >= monsterMoveInterval_)
	{
		monsterMoveElapsedTime_ = 0.0f;
		MoveMonsters();
	}

	arrowSpawnElapsedTime_ += timeStep;

	if (arrowSpawnElapsedTime_ >= arrowSpawnInterval_)
	{
		arrowSpawnElapsedTime_ = 0.0f;
		SpawnArrow();
	}

	if (invincible_)
	{
		invincibilityElapsedTime_ += timeStep;

		if (invincibilityElapsedTime_ >= invincibilityInterval_)
		{
			archer_->GetChild("archer")->GetChild("invincibilitysparkle")->SetEnabled(false);
			invincibilityElapsedTime_ = 0.0f;
			invincible_ = false;
		}
	}
}

void Gameplay::HandleElementResize(StringHash eventType, VariantMap& eventData)
{
	using namespace Resized;

	UIElement* ele = static_cast<UIElement*>(eventData[ElementAdded::P_ELEMENT].GetPtr());

	IntVector2 rootExtent = main_->ui_->GetRoot()->GetSize();

	IntVector2 scaledExtent;

	scaledExtent.x_ = ( ele->GetWidth() *  rootExtent.x_ ) / previousExtents_.x_;
	scaledExtent.y_ = ( ele->GetHeight() *  rootExtent.y_ ) / previousExtents_.y_;

	ele->SetSize(scaledExtent);

	IntVector2 scaledPosition = IntVector2(
			( ele->GetPosition().x_ *  rootExtent.x_ ) / previousExtents_.x_,
			( ele->GetPosition().y_ *  rootExtent.y_ ) / previousExtents_.y_);

	ele->SetPosition(scaledPosition);
}

void Gameplay::HandlePressed(StringHash eventType, VariantMap& eventData)
{
	using namespace Pressed;

	UIElement* ele = static_cast<UIElement*>(eventData[ElementAdded::P_ELEMENT].GetPtr());
}

void Gameplay::HandleReleased(StringHash eventType, VariantMap& eventData)
{
	using namespace Released;

	UIElement* ele = static_cast<UIElement*>(eventData[ElementAdded::P_ELEMENT].GetPtr());
}

void Gameplay::HandleNodeCollisionStart(StringHash eventType, VariantMap& eventData)
{
	using namespace NodeCollisionStart;

	Node* noed = static_cast<RigidBody*>(eventData[P_BODY].GetPtr())->GetNode();
	Node* otherNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());
	bool trigger = eventData[P_TRIGGER].GetBool();

	if (noed->GetName() == "archer")
	{
		if (otherNode->GetName() == "elf")
		{
			score_ += 2;

			scoreText_->GetComponent<Text3D>()->SetText("The Score " + String( score_ ));
			scoreText_->GetComponent<Text3D>()->SetWidth(12);
			scoreText_->ApplyAttributes();

			if (score_ > topScore_->GetVar("TopScore").GetInt())
			{
				topScore_->SetVar("TopScore", score_);
				topScoreText_->GetComponent<Text3D>()->SetText("Top Score " + String( topScore_->GetVar("TopScore").GetInt() ));
				topScoreText_->GetComponent<Text3D>()->SetWidth(12);
				topScoreText_->ApplyAttributes();
			}

			SpawnElf();

			archerHitElf_->GetComponent<SoundSource>()->Play(archerHitElf_->GetComponent<SoundSource>()->GetSound());
		}
		else if (otherNode->GetName() == "chest")
		{
			score_++;

			scoreText_->GetComponent<Text3D>()->SetText("The Score " + String( score_ ));
			scoreText_->GetComponent<Text3D>()->SetWidth(12);
			scoreText_->ApplyAttributes();

			if (score_ > topScore_->GetVar("TopScore").GetInt())
			{
				topScore_->SetVar("TopScore", score_);
				topScoreText_->GetComponent<Text3D>()->SetText("Top Score " + String( topScore_->GetVar("TopScore").GetInt() ));
				topScoreText_->GetComponent<Text3D>()->SetWidth(12);
				topScoreText_->ApplyAttributes();
			}

			File saveFile(context_, main_->filesystem_->GetProgramDir() + "Data/Objects/TopScore.xml", FILE_WRITE);
			topScore_->SaveXML(saveFile);
			SpawnChest();

			archerHitChest_->GetComponent<SoundSource>()->Play(archerHitChest_->GetComponent<SoundSource>()->GetSound());
		}
		else if (otherNode->GetName() == "potion")
		{
			score_++;

			scoreText_->GetComponent<Text3D>()->SetText("The Score " + String( score_ ));
			scoreText_->GetComponent<Text3D>()->SetWidth(12);
			scoreText_->ApplyAttributes();

			if (score_ > topScore_->GetVar("TopScore").GetInt())
			{
				topScore_->SetVar("TopScore", score_);
				topScoreText_->GetComponent<Text3D>()->SetText("Top Score " + String( topScore_->GetVar("TopScore").GetInt() ));
				topScoreText_->GetComponent<Text3D>()->SetWidth(12);
				topScoreText_->ApplyAttributes();
			}

			archer_->GetChild("archer")->GetChild("invincibilitysparkle")->SetEnabled(true);
			invincibilityElapsedTime_ = 0.0f;
			invincible_ = true;
			SpawnPotion();

			archerHitPotion_->GetComponent<SoundSource>()->Play(archerHitPotion_->GetComponent<SoundSource>()->GetSound());
		}
		else if (otherNode->GetName() == "quartz")
		{
			if (otherNode->GetVar("Fired").GetBool())
			{
				return;
			}

			score_++;

			scoreText_->GetComponent<Text3D>()->SetText("The Score " + String( score_ ));
			scoreText_->GetComponent<Text3D>()->SetWidth(12);
			scoreText_->ApplyAttributes();

			if (score_ > topScore_->GetVar("TopScore").GetInt())
			{
				topScore_->SetVar("TopScore", score_);
				topScoreText_->GetComponent<Text3D>()->SetText("Top Score " + String( topScore_->GetVar("TopScore").GetInt() ));
				topScoreText_->GetComponent<Text3D>()->SetWidth(12);
				topScoreText_->ApplyAttributes();
			}

			otherNode->SetEnabledRecursive(false);
			quiver_.Push(otherNode);

			archerHitArrow_->GetComponent<SoundSource>()->Play(archerHitArrow_->GetComponent<SoundSource>()->GetSound());
		}
		else if (otherNode->GetName() == "pet1"
				|| otherNode->GetName() == "pet2"
						|| otherNode->GetName() == "pet3"
								|| otherNode->GetName() == "pet4")
		{
			if (invincible_)
			{
				return;
			}
			score_--;

			if (score_ < 0)
			{
				score_ = 0;
			}

			scoreText_->GetComponent<Text3D>()->SetText("The Score " + String( score_ ));
			scoreText_->GetComponent<Text3D>()->SetWidth(12);
			scoreText_->ApplyAttributes();

			dogHitArcher_->GetComponent<SoundSource>()->Play(dogHitArcher_->GetComponent<SoundSource>()->GetSound());
		}
	}
	else if (noed->GetName() == "quartz")//arrow
	{
		if (otherNode->GetName() == "walls")
		{
			noed->GetComponent<RigidBody>()->SetLinearVelocity(Vector3::ZERO);
			noed->GetComponent<RigidBodyMoveTo>()->isMoving_ = false;
			noed->SetVar("Fired", false);

			arrowHitWall_->GetComponent<SoundSource>()->Play(arrowHitWall_->GetComponent<SoundSource>()->GetSound());
		}
		else if ( (otherNode->GetName() == "pet1"
				|| otherNode->GetName() == "pet2"
						|| otherNode->GetName() == "pet3"
								|| otherNode->GetName() == "pet4") && noed->GetVar("Fired").GetBool())
		{
			spawnedMonsters_.Remove(otherNode);
			otherNode->RemoveAllComponents();
			otherNode->RemoveAllChildren();
			otherNode->Remove();
			monsterCount_--;

			score_++;

			scoreText_->GetComponent<Text3D>()->SetText("The Score " + String( score_ ));
			scoreText_->GetComponent<Text3D>()->SetWidth(12);
			scoreText_->ApplyAttributes();

			if (score_ > topScore_->GetVar("TopScore").GetInt())
			{
				topScore_->SetVar("TopScore", score_);
				topScoreText_->GetComponent<Text3D>()->SetText("Top Score " + String( topScore_->GetVar("TopScore").GetInt() ));
				topScoreText_->GetComponent<Text3D>()->SetWidth(12);
				topScoreText_->ApplyAttributes();
			}

			arrowHitDog_->GetComponent<SoundSource>()->Play(arrowHitDog_->GetComponent<SoundSource>()->GetSound());
		}
	}
}

void Gameplay::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;
	int key = eventData[P_KEY].GetInt();

	if (key == KEY_W)//up
	{
		wDown_ = true;
	}
	else if (key == KEY_A)//left
	{
		aDown_ = true;
	}
	else if (key == KEY_S)//down
	{
		sDown_ = true;
	}
	else if (key == KEY_D)//right
	{
		dDown_ = true;
	}
	else if (key == KEY_SPACE)//projectile
	{
		ShootArrow();
	}
	else  if (key == KEY_ESC)
	{
		main_->GetSubsystem<Engine>()->Exit();
	}
}

void Gameplay::HandleKeyUp(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyUp;
	int key = eventData[P_KEY].GetInt();

	if (key == KEY_W)//up
	{
		wDown_ = false;
	}
	else if (key == KEY_A)//left
	{
		aDown_ = false;
	}
	else if (key == KEY_S)//down
	{
		sDown_ = false;
	}
	else if (key == KEY_D)//right
	{
		dDown_ = false;
	}
}

void Gameplay::MoveArcher()
{
	Quaternion rot = archer_->GetRotation();
	Vector3 moveDir = Vector3::ZERO;

	if (wDown_)
	{
		moveDir += Vector3::FORWARD;
		archer_->GetChild("archer")->SetRotation(Quaternion(45.0f, 0.0f, 0.0f));
		archerDir_ = 0;
	}
	else if (sDown_)
	{
		moveDir += Vector3::BACK;
		archer_->GetChild("archer")->SetRotation(Quaternion(-45.0f, 180.0f, 0.0f));
		archerDir_ = 1;
	}

	if (aDown_)
	{
		moveDir += Vector3::LEFT;
		archer_->GetChild("archer")->SetRotation(Quaternion(0.0f, -90.0f, -45.0f));
		archerDir_ = 2;
	}
	else if (dDown_)
	{
		moveDir += Vector3::RIGHT;
		archer_->GetChild("archer")->SetRotation(Quaternion(0.0f, 90.0f, 45.0f));
		archerDir_ = 3;
	}

	if (moveDir.LengthSquared() > 0.0f)
	{
		moveDir.Normalize();
	}

	if (moveDir == Vector3::ZERO)
	{
		if (!archer_->GetChild("archer")->GetComponent<AnimationController>()->IsPlaying("Models/archerIdle.ani")
				&& !archer_->GetChild("archer")->GetComponent<AnimationController>()->IsPlaying("Models/archerAttack.ani"))
		{
			RecursiveAnimate(archer_->GetChild("archer"), "Models/archerIdle.ani", 0, true, 0.0f, true, 1.0f);
		}

		archer_->GetChild("archer")->GetComponent<RigidBody>()->SetLinearVelocity(Vector3::ZERO);
		return;
	}

	if (!archer_->GetChild("archer")->GetComponent<AnimationController>()->IsPlaying("archerRun1")
			&& !archer_->GetChild("archer")->GetComponent<AnimationController>()->IsPlaying("Models/archerAttack.ani"))
	{
		RecursiveAnimate(archer_->GetChild("archer"), "Models/archerRun1.ani", 0, true, 0.0f, true, 1.0f);
	}

	archer_->GetChild("archer")->GetComponent<RigidBody>()->SetLinearVelocity((rot * moveDir) * archerSpeed_);
}

void Gameplay::RecursiveAnimate(Node* noed, String animation, char layer, bool loop, float fadeTime, bool exclusive, float speed)
{
	if (noed->GetComponent<AnimationController>())
	{
		if (exclusive)
		{
			noed->GetComponent<AnimationController>()->PlayExclusive(animation, layer, loop, fadeTime);
			noed->GetComponent<AnimationController>()->SetSpeed(animation, speed);
		}
		else
		{
			noed->GetComponent<AnimationController>()->Play(animation, layer, loop, fadeTime);
			noed->GetComponent<AnimationController>()->SetSpeed(animation, speed);
		}

		if (!loop)
		{
			noed->GetComponent<AnimationController>()->SetAutoFade(animation, 0.25f);
		}
	}

	for (int x = 0; x < noed->GetNumChildren(); x++)
	{
		RecursiveAnimate(noed->GetChild(x), animation, layer, loop, fadeTime, exclusive, speed);
	}
}

void Gameplay::RandomizeGates()
{
	closedCells_.Clear();
	openCells_.Clear();

	PODVector<StaticModel*> components;

	BoundingBox archerBB = archer_->GetChild("archer")->
			GetComponent<CollisionShape>()->GetWorldBoundingBox();

	for (int x = 0; x < cells_->GetNumChildren(); x++)
	{
		components.Clear();
		if ( cells_->GetChild(x)->GetChild("closedTop")->GetComponent<CollisionShape>()->
				GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
		{
			cells_->GetChild(x)->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			cells_->GetChild(x)->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		components.Clear();
		if ( cells_->GetChild(x)->GetChild("closedBottom")->GetComponent<CollisionShape>()->
				GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
		{
			cells_->GetChild(x)->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			cells_->GetChild(x)->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		components.Clear();
		if ( cells_->GetChild(x)->GetChild("closedLeft")->GetComponent<CollisionShape>()->
				GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
		{
			cells_->GetChild(x)->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			cells_->GetChild(x)->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		components.Clear();
		if ( cells_->GetChild(x)->GetChild("closedRight")->GetComponent<CollisionShape>()->
				GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
		{
			cells_->GetChild(x)->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			cells_->GetChild(x)->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		closedCells_.Push(cells_->GetChild(x));
	}

	for (int x = 0; x < cells_->GetNumChildren() * 0.5f; x++)
	{
		Node* cell = closedCells_[Random(0, closedCells_.Size())];
		openCells_.Push(cell);
		closedCells_.Remove(cell);
	}

	//Disabling/Enabling a CollisionShape during collision crashes.  Turn to trigger instead.

	for (int x = 0; x < closedCells_.Size(); x++)
	{
		//Top
		components.Clear();
		closedCells_[x]->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
		if (Random(0,2))
		{
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//closedCells_[x]->GetChild("closedTop")->GetComponent<CollisionShape>()->SetEnabled(false);
			closedCells_[x]->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			if ( closedCells_[x]->GetChild("closedTop")->GetComponent<CollisionShape>()->
					GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
			{
				components[0]->SetEnabled(true);
				components[1]->SetEnabled(false);
				//closedCells_[x]->GetChild("closedTop")->GetComponent<CollisionShape>()->SetEnabled(true);
				closedCells_[x]->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
			}
		}

		//Bottom
		components.Clear();
		closedCells_[x]->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
		if (Random(0,2))
		{
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//closedCells_[x]->GetChild("closedBottom")->GetComponent<CollisionShape>()->SetEnabled(false);
			closedCells_[x]->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			if ( closedCells_[x]->GetChild("closedBottom")->GetComponent<CollisionShape>()->
					GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
			{
				components[0]->SetEnabled(true);
				components[1]->SetEnabled(false);
				//closedCells_[x]->GetChild("closedBottom")->GetComponent<CollisionShape>()->SetEnabled(true);
				closedCells_[x]->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
			}
		}

		//Left
		components.Clear();
		closedCells_[x]->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
		if (Random(0,2))
		{
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//closedCells_[x]->GetChild("closedLeft")->GetComponent<CollisionShape>()->SetEnabled(false);
			closedCells_[x]->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			if ( closedCells_[x]->GetChild("closedLeft")->GetComponent<CollisionShape>()->
					GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
			{
				components[0]->SetEnabled(true);
				components[1]->SetEnabled(false);
				//closedCells_[x]->GetChild("closedLeft")->GetComponent<CollisionShape>()->SetEnabled(true);
				closedCells_[x]->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
			}
		}

		//Right
		components.Clear();
		closedCells_[x]->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
		if (Random(0,2))
		{
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//closedCells_[x]->GetChild("closedRight")->GetComponent<CollisionShape>()->SetEnabled(false);
			closedCells_[x]->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			if ( closedCells_[x]->GetChild("closedRight")->GetComponent<CollisionShape>()->
					GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
			{
				components[0]->SetEnabled(true);
				components[1]->SetEnabled(false);
				//closedCells_[x]->GetChild("closedRight")->GetComponent<CollisionShape>()->SetEnabled(true);
				closedCells_[x]->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
			}
		}
	}

	for (int x = 0; x < openCells_.Size(); x++)
	{
		components.Clear();
		if (Random(0,2))//Top
		{
			openCells_[x]->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//openCells_[x]->GetChild("closedTop")->GetComponent<CollisionShape>()->SetEnabled(false);
			openCells_[x]->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else//Bottom
		{
			openCells_[x]->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//openCells_[x]->GetChild("closedBottom")->GetComponent<CollisionShape>()->SetEnabled(false);
			openCells_[x]->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
		}

		components.Clear();
		if (Random(0,2))//Left
		{
			openCells_[x]->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//openCells_[x]->GetChild("closedLeft")->GetComponent<CollisionShape>()->SetEnabled(false);
			openCells_[x]->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else//Right
		{
			openCells_[x]->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			//openCells_[x]->GetChild("closedRight")->GetComponent<CollisionShape>()->SetEnabled(false);
			openCells_[x]->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
		}
	}

	gateOpen_->GetComponent<SoundSource>()->Play(gateOpen_->GetComponent<SoundSource>()->GetSound());
}


void Gameplay::HandleMouseDown(StringHash eventType, VariantMap& eventData)
{
	using namespace MouseButtonDown;
	int butts = eventData[P_BUTTONS].GetInt();

	if (butts & MOUSEB_LEFT)
	{
		XorInnerGates();
	}

	if (butts & MOUSEB_RIGHT)
	{
		XorOuterGates();
	}
}

void Gameplay::XorInnerGates()
{
	PODVector<StaticModel*> components;

	BoundingBox archerBB = archer_->GetChild("archer")->
			GetComponent<CollisionShape>()->GetWorldBoundingBox();

	PODVector<PhysicsRaycastResult> raeResult;

	Ray mouseRay = cameraNode_->GetComponent<Camera>()->GetScreenRay(
			(float) main_->input_->GetMousePosition().x_ / main_->graphics_->GetWidth(),
			(float) main_->input_->GetMousePosition().y_ / main_->graphics_->GetHeight());

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, mouseRay, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			Node* targCell = raeResult[x].body_->GetNode();

			Ray archerRay;
			archerRay.origin_ = archer_->GetChild("archer")->GetComponent<RigidBody>()->GetPosition();
			archerRay.direction_ = Vector3::DOWN;

			PODVector<PhysicsRaycastResult> raeResult2;

			scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult2, archerRay, 1000.0f);

			for (int y = 0; y < raeResult2.Size(); y++)
			{
				if (raeResult2[y].body_->GetNode()->GetName() == "cell")
				{
					Node* destCell = raeResult2[y].body_->GetNode();

					bool isDestTrigger, isTargTrigger, destXor, targXor;

					//Top
					isDestTrigger = destCell->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger();
					isTargTrigger = targCell->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger();

					destXor = isDestTrigger ^ isTargTrigger;
					targXor = isTargTrigger ^ destXor;

					components.Clear();
					if (destXor)
					{
						destCell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						destCell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( destCell->GetChild("closedTop")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							destCell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							destCell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					components.Clear();
					if (targXor)
					{
						targCell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						targCell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( targCell->GetChild("closedTop")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							targCell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							targCell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					//Bottom
					isDestTrigger = destCell->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger();
					isTargTrigger = targCell->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger();

					destXor = isDestTrigger ^ isTargTrigger;
					targXor = isTargTrigger ^ destXor;

					components.Clear();
					if (destXor)
					{
						destCell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						destCell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( destCell->GetChild("closedBottom")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							destCell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							destCell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					components.Clear();
					if (targXor)
					{
						targCell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						targCell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( targCell->GetChild("closedBottom")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							targCell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							targCell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					//Left
					isDestTrigger = destCell->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger();
					isTargTrigger = targCell->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger();

					destXor = isDestTrigger ^ isTargTrigger;
					targXor = isTargTrigger ^ destXor;

					components.Clear();
					if (destXor)
					{
						destCell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						destCell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( destCell->GetChild("closedLeft")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							destCell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							destCell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					components.Clear();
					if (targXor)
					{
						targCell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						targCell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( targCell->GetChild("closedLeft")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							targCell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							targCell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					//Right
					isDestTrigger = destCell->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger();
					isTargTrigger = targCell->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger();

					destXor = isDestTrigger ^ isTargTrigger;
					targXor = isTargTrigger ^ destXor;

					components.Clear();
					if (destXor)
					{
						destCell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						destCell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( destCell->GetChild("closedRight")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							destCell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							destCell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					components.Clear();
					if (targXor)
					{
						targCell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
						components[0]->SetEnabled(false);
						components[1]->SetEnabled(true);
						targCell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
					}
					else
					{
						if ( targCell->GetChild("closedRight")->GetComponent<CollisionShape>()->
								GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
						{
							targCell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
							components[0]->SetEnabled(true);
							components[1]->SetEnabled(false);
							targCell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
						}
					}

					break;
				}
			}

			break;
		}
	}

	gateOpen_->GetComponent<SoundSource>()->Play(gateOpen_->GetComponent<SoundSource>()->GetSound());
}

void Gameplay::XorOuterGates()
{
	Node* destCell = NULL;
	Node* destCellUp = NULL;
	Node* destCellDown = NULL;
	Node* destCellLeft = NULL;
	Node* destCellRight = NULL;
	Node* targCell = NULL;
	Node* targCellUp = NULL;
	Node* targCellDown = NULL;
	Node* targCellLeft = NULL;
	Node* targCellRight = NULL;

	PODVector<StaticModel*> components;

	bool isDestTrigger, isTargTrigger, destXor, targXor;

	PODVector<PhysicsRaycastResult> raeResult;

	Ray mouseRay = cameraNode_->GetComponent<Camera>()->GetScreenRay(
			(float) main_->input_->GetMousePosition().x_ / main_->graphics_->GetWidth(),
			(float) main_->input_->GetMousePosition().y_ / main_->graphics_->GetHeight());

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, mouseRay, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			targCell = raeResult[x].body_->GetNode();
			break;
		}
	}

	if (!targCell){return;}

	Ray archerRay;
	archerRay.origin_ = archer_->GetChild("archer")->GetComponent<RigidBody>()->GetPosition();
	archerRay.direction_ = Vector3::DOWN;

	raeResult.Clear();
	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, archerRay, 1000.0f);

	for (int y = 0; y < raeResult.Size(); y++)
	{
		if (raeResult[y].body_->GetNode()->GetName() == "cell")
		{
			destCell = raeResult[y].body_->GetNode();
			break;
		}
	}

	if (!destCell){return;}

	Ray rae;

	Vector3 cellSize = targCell->GetComponent<CollisionShape>()->GetSize();

	//Top (In the scene, the cells are Top=Right, Bottom=Left, Left=Top, Right=Bottom,
	raeResult.Clear();
	rae.origin_ = targCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(0.0f, 1.0f, cellSize.z_);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			targCellUp = raeResult[x].body_->GetNode();
			break;
		}
	}

	raeResult.Clear();
	rae.origin_ = destCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(0.0f, 1.0f, cellSize.z_);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			destCellUp = raeResult[x].body_->GetNode();
			break;
		}
	}

	if (destCellUp && targCellUp)
	{
		isDestTrigger = destCellUp->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger();
		isTargTrigger = targCellUp->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger();

		destXor = isDestTrigger ^ isTargTrigger;
		targXor = isTargTrigger ^ destXor;

		components.Clear();
		if (destXor)
		{
			destCellUp->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			destCellUp->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			destCellUp->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			destCellUp->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		components.Clear();
		if (targXor)
		{
			targCellUp->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			targCellUp->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			targCellUp->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			targCellUp->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
		}
	}

	//Bottom (In the scene, the cells are Top=Right, Bottom=Left, Left=Top, Right=Bottom,
	raeResult.Clear();
	rae.origin_ = targCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(0.0f, 1.0f, -cellSize.z_);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			targCellDown = raeResult[x].body_->GetNode();
			break;
		}
	}

	raeResult.Clear();
	rae.origin_ = destCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(0.0f, 1.0f, -cellSize.z_);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			destCellDown = raeResult[x].body_->GetNode();
			break;
		}
	}

	if (targCellDown && destCellDown)
	{
		isDestTrigger = destCellDown->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger();
		isTargTrigger = targCellDown->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger();

		destXor = isDestTrigger ^ isTargTrigger;
		targXor = isTargTrigger ^ destXor;

		components.Clear();
		if (destXor)
		{
			destCellDown->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			destCellDown->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			destCellDown->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			destCellDown->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		components.Clear();
		if (targXor)
		{
			targCellDown->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			targCellDown->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			targCellDown->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			targCellDown->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
		}
	}

	//Left (In the scene, the cells are Top=Right, Bottom=Left, Left=Top, Right=Bottom,
	raeResult.Clear();
	rae.origin_ = targCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(-cellSize.x_, 1.0f, 0.0f);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			targCellLeft = raeResult[x].body_->GetNode();
			break;
		}
	}

	raeResult.Clear();
	rae.origin_ = destCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(-cellSize.x_, 1.0f, 0.0f);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			destCellLeft = raeResult[x].body_->GetNode();
			break;
		}
	}

	if (destCellLeft && targCellLeft)
	{
		isDestTrigger = destCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger();
		isTargTrigger = targCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger();

		destXor = isDestTrigger ^ isTargTrigger;
		targXor = isTargTrigger ^ destXor;

		components.Clear();
		if (destXor)
		{
			destCellLeft->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			destCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			destCellLeft->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			destCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		components.Clear();
		if (targXor)
		{
			targCellLeft->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			targCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			targCellLeft->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			targCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
		}
	}

	//Right (In the scene, the cells are Top=Right, Bottom=Left, Left=Top, Right=Bottom,
	raeResult.Clear();
	rae.origin_ = targCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(cellSize.x_, 1.0f, 0.0f);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			targCellRight = raeResult[x].body_->GetNode();
			break;
		}
	}

	raeResult.Clear();
	rae.origin_ = destCell->GetComponent<RigidBody>()->GetPosition();
	rae.origin_ += Vector3(cellSize.x_, 1.0f, 0.0f);
	rae.direction_ = Vector3::DOWN;

	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

	for (int x = 0; x < raeResult.Size(); x++)
	{
		if (raeResult[x].body_->GetNode()->GetName() == "cell")
		{
			destCellRight = raeResult[x].body_->GetNode();
			break;
		}
	}

	if (destCellRight && targCellRight)
	{
		isDestTrigger = destCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger();
		isTargTrigger = targCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger();

		destXor = isDestTrigger ^ isTargTrigger;
		targXor = isTargTrigger ^ destXor;

		components.Clear();
		if (destXor)
		{
			destCellRight->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			destCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			destCellRight->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			destCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
		}

		components.Clear();
		if (targXor)
		{
			targCellRight->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(false);
			components[1]->SetEnabled(true);
			targCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
		}
		else
		{
			targCellRight->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
			components[0]->SetEnabled(true);
			components[1]->SetEnabled(false);
			targCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
		}
	}

	gateOpen_->GetComponent<SoundSource>()->Play(gateOpen_->GetComponent<SoundSource>()->GetSound());
}

void Gameplay::SpawnMonster()
{
	if (monsterCount_ >= monsterMax_){return;}

	Node* archerCell;

	PODVector<PhysicsRaycastResult> raeResult;

	Ray archerRay;
	archerRay.origin_ = archer_->GetChild("archer")->GetComponent<RigidBody>()->GetPosition();
	archerRay.direction_ = Vector3::DOWN;

	raeResult.Clear();
	scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, archerRay, 1000.0f);

	for (int y = 0; y < raeResult.Size(); y++)
	{
		if (raeResult[y].body_->GetNode()->GetName() == "cell")
		{
			archerCell = raeResult[y].body_->GetNode();
			break;
		}
	}

	Node* cell = cells_->GetChild(Random(0,cells_->GetNumChildren()));

	if (archerCell == cell)
	{
		SpawnMonster();
		return;
	}

	Node* monster = baseMonsters_[Random(0,4)]->Clone(LOCAL);

	spawnedMonsters_.Push(monster);

	RigidBodyMoveTo* _RigidBodyMoveTo = new RigidBodyMoveTo(context_);
	monster->AddComponent(_RigidBodyMoveTo, 0, LOCAL);

	monsterCount_++;

	monster->SetPosition(cell->GetPosition() + Vector3(0.0f, 6.0f, 0.0f));
}

void Gameplay::MoveMonsters()
{
	PODVector<StaticModel*> components;

	BoundingBox archerBB = archer_->GetChild("archer")->
			GetComponent<CollisionShape>()->GetWorldBoundingBox();

	bool isDestTrigger, isTargTrigger, destXor, targXor;

	if (monsterMoveTurn_ >= spawnedMonsters_.Size())
	{
		monsterMoveTurn_ = 0;
	}

	Node* monster = spawnedMonsters_[monsterMoveTurn_];

	//for (int x = 0; x < spawnedMonsters_.Size(); x++)//pets close each others gates.
	if (monster)
	{
		monsterMoveTurn_++;
		//Node* monster = spawnedMonsters_[x];
		Vector3 monsterPos = monster->GetComponent<RigidBody>()->GetPosition();
		Vector3 monsterCellPos;
		Vector3 archerPos = archer_->GetChild("archer")->GetComponent<RigidBody>()->GetPosition();

		Node* monsterCell = NULL;
		Node* monsterCellTop = NULL;
		Node* monsterCellBottom = NULL;
		Node* monsterCellLeft = NULL;
		Node* monsterCellRight = NULL;

		PODVector<PhysicsRaycastResult> raeResult;

		Ray rae;
		rae.origin_ = monsterPos + Vector3(0.0f, 1.0f, 0.0f);
		rae.direction_ = Vector3::DOWN;

		scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

		for (int y = 0; y < raeResult.Size(); y++)
		{
			if (raeResult[y].body_->GetNode()->GetName() == "cell")
			{
				monsterCell = raeResult[y].body_->GetNode();
				break;
			}
		}

		if (!monsterCell)//This shouldn't be necessary. Why would monsterCell ever be null?
		{
			return;
		}

		//monsterCellPos = monsterCell->GetComponent<RigidBody>()->GetPosition();
		monsterCellPos = monsterCell->GetPosition();
		monsterCellPos.y_ = monsterPos.y_;

		Vector3 cellSize = monsterCell->GetComponent<CollisionShape>()->GetSize();

		//Top
		rae.origin_ = monsterCellPos + Vector3(0.0f, 1.0f, cellSize.z_);

		raeResult.Clear();

		scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

		for (int y = 0; y < raeResult.Size(); y++)
		{
			if (raeResult[y].body_->GetNode()->GetName() == "cell")
			{
				monsterCellTop = raeResult[y].body_->GetNode();
				break;
			}
		}

		//Bottom
		rae.origin_ = monsterCellPos + Vector3(0.0f, 1.0f, -cellSize.z_);

		raeResult.Clear();

		scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

		for (int y = 0; y < raeResult.Size(); y++)
		{
			if (raeResult[y].body_->GetNode()->GetName() == "cell")
			{
				monsterCellBottom = raeResult[y].body_->GetNode();
				break;
			}
		}

		//Left
		rae.origin_ = monsterCellPos + Vector3(-cellSize.x_, 1.0f, 0.0f);

		raeResult.Clear();

		scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

		for (int y = 0; y < raeResult.Size(); y++)
		{
			if (raeResult[y].body_->GetNode()->GetName() == "cell")
			{
				monsterCellLeft = raeResult[y].body_->GetNode();
				break;
			}
		}

		//Right
		rae.origin_ = monsterCellPos + Vector3(cellSize.x_, 1.0f, 0.0f);

		raeResult.Clear();

		scene_->GetComponent<PhysicsWorld>()->Raycast(raeResult, rae, 1000.0f);

		for (int y = 0; y < raeResult.Size(); y++)
		{
			if (raeResult[y].body_->GetNode()->GetName() == "cell")
			{
				monsterCellRight = raeResult[y].body_->GetNode();
				break;
			}
		}

		float xDist, zDist;

		xDist = Abs(monsterPos.x_ - archerPos.x_);
		zDist = Abs(monsterPos.z_ - archerPos.z_);

		if (xDist > zDist)
		{
			if (monsterPos.x_ < archerPos.x_)
			{
				char locks = 2;
				if (monsterCell->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedBottom")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (monsterCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCellRight->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCellRight->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedTop")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (locks == 0)
				{
					monster->GetComponent<RigidBodyMoveTo>()->
							MoveTo(monsterCellPos + Vector3(cellSize.x_, 0.0f, 0.0f),
									monsterSpeed_, true);
				}
			}
			else if (monsterPos.x_ > archerPos.x_)
			{
				char locks = 2;
				if (monsterCell->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedTop")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedTop")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedTop")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedTop")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (monsterCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedBottom")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCellLeft->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCellLeft->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedBottom")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedBottom")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedBottom")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (locks == 0)
				{
					monster->GetComponent<RigidBodyMoveTo>()->
							MoveTo(monsterCellPos + Vector3(-cellSize.x_, 0.0f, 0.0f),
									monsterSpeed_, true);
				}
			}
		}
		else
		{
			if (monsterPos.z_ < archerPos.z_)
			{
				char locks = 2;
				if (monsterCell->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedRight")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (monsterCellTop->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCellTop->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCellTop->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedLeft")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (locks == 0)
				{
					monster->GetComponent<RigidBodyMoveTo>()->
							MoveTo(monsterCellPos + Vector3(0.0f, 0.0f, cellSize.z_),
									monsterSpeed_, true);
				}
			}
			else if (monsterPos.z_ > archerPos.z_)
			{
				char locks = 2;
				if (monsterCell->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedLeft")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedLeft")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedLeft")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedLeft")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (monsterCellBottom->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger())
				{
					locks--;
				}
				else
				{
					isDestTrigger = false;

					for (int y = 0; y < cells_->GetNumChildren(); y++)
					{
						Node* cell = cells_->GetChild(y);

						if (cell->GetChild("closedRight")->GetComponent<RigidBody>()->IsTrigger())
						{
							isTargTrigger = true;

							destXor = isDestTrigger ^ isTargTrigger;
							targXor = isTargTrigger ^ destXor;

							components.Clear();
							if (destXor)
							{
								monsterCellBottom->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								monsterCellBottom->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
							}

							components.Clear();
							if (targXor)
							{
								cell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
								components[0]->SetEnabled(false);
								components[1]->SetEnabled(true);
								cell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(true);
							}
							else
							{
								if ( cell->GetChild("closedRight")->GetComponent<CollisionShape>()->
										GetWorldBoundingBox().IsInside(archerBB) == OUTSIDE)
								{
									cell->GetChild("closedRight")->GetComponents<StaticModel>(components, false);
									components[0]->SetEnabled(true);
									components[1]->SetEnabled(false);
									cell->GetChild("closedRight")->GetComponent<RigidBody>()->SetTrigger(false);
								}
							}

							locks--;
							break;
						}
					}
				}

				if (locks == 0)
				{
					monster->GetComponent<RigidBodyMoveTo>()->
							MoveTo(monsterCellPos + Vector3(0.0f, 0.0f, -cellSize.z_),
									monsterSpeed_, true);
				}
			}
		}
	}
}

void Gameplay::SpawnArrow()
{
	if (arrowCount_ >= arrowMax_){return;}

	Node* cell = cells_->GetChild(Random(0, cells_->GetNumChildren()));

	Node* arrow = arrow_->Clone(LOCAL);

	SubscribeToEvent(arrow, E_NODECOLLISIONSTART, HANDLER(Gameplay, HandleNodeCollisionStart));

	RigidBodyMoveTo* _RigidBodyMoveTo = new RigidBodyMoveTo(context_);
	arrow->AddComponent(_RigidBodyMoveTo, 0, LOCAL);

	arrow->SetPosition(cell->GetPosition() + Vector3(0.0f, 4.0f, 0.0f));

	spawnedArrows_.Push(arrow);

	arrowCount_++;
}

void Gameplay::SpawnPotion()
{
	Node* cell = cells_->GetChild(Random(0, cells_->GetNumChildren()));

	potion_->SetPosition(cell->GetPosition() + Vector3(0.0f, 4.0f, 0.0f));
}

void Gameplay::SpawnChest()
{
	Node* cell = cells_->GetChild(Random(0, cells_->GetNumChildren()));

	chest_->SetPosition(cell->GetPosition() + Vector3(0.0f, 4.0f, 0.0f));
}

void Gameplay::SpawnElf()
{
	Node* cell = cells_->GetChild(Random(0, cells_->GetNumChildren()));

	elf_->SetPosition(cell->GetPosition() + Vector3(0.0f, 4.0f, 0.0f));

	elf_->GetComponent<AnimationController>()->PlayExclusive("Models/elfIdle.ani", 0, true, 0.0f);
}

void Gameplay::ShootArrow()
{
	if (quiver_.Size())
	{
		Node* arrow  = quiver_[0];
		quiver_.Remove(arrow);

		arrow->SetVar("Fired", true);
		arrow->SetEnabledRecursive(true);

		arrow->SetPosition(archer_->GetChild("archer")->GetPosition());

		if (archerDir_ == 0)
		{
			arrow->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));

			Vector3 dest = arrow->GetPosition();
			dest += Vector3::FORWARD * 1000.0f;

			arrow->GetComponent<RigidBodyMoveTo>()->
					MoveTo(dest, monsterSpeed_, true);
		}
		else if (archerDir_ == 1)
		{
			arrow->SetRotation(Quaternion(90.0f, 180.0f, 0.0f));

			Vector3 dest = arrow->GetPosition();
			dest += Vector3::BACK * 1000.0f;

			arrow->GetComponent<RigidBodyMoveTo>()->
					MoveTo(dest, monsterSpeed_, true);
		}
		else if (archerDir_ == 2)
		{
			arrow->SetRotation(Quaternion(90.0f, -90.0f, 0.0f));

			Vector3 dest = arrow->GetPosition();
			dest += Vector3::LEFT * 1000.0f;

			arrow->GetComponent<RigidBodyMoveTo>()->
					MoveTo(dest, monsterSpeed_, true);
		}
		else if (archerDir_ == 3)
		{
			arrow->SetRotation(Quaternion(90.0f, 90.0f, 0.0f));

			Vector3 dest = arrow->GetPosition();
			dest += Vector3::RIGHT * 1000.0f;

			arrow->GetComponent<RigidBodyMoveTo>()->
					MoveTo(dest, monsterSpeed_, true);
		}

		RecursiveAnimate(archer_->GetChild("archer"), "Models/archerAttack.ani", 0, false, 0.0f, true, 3.0f);

		shootArrow_->GetComponent<SoundSource>()->Play(shootArrow_->GetComponent<SoundSource>()->GetSound());
	}
}
