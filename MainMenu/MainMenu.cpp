/*
 * MainMenu.cpp
 *
 *  Created on: May 16, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Graphics/Viewport.h>

#include "MainMenu.h"

#include "../Gameplay/Gameplay.h"

MainMenu::MainMenu(Context* context, Urho3DPlayer* main) :
    Object(context)
{
	main_ = main;
	elapsedTime_ = 0.0f;

	new Gameplay(context_, main_);

	delete this;
}

MainMenu::~MainMenu()
{
}
