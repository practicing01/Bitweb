//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Urho3D.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Core/Main.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "Urho3DPlayer.h"

#include <Urho3D/DebugNew.h>

#include "MainMenu/MainMenu.h"

DEFINE_APPLICATION_MAIN(Urho3DPlayer);

Urho3DPlayer::Urho3DPlayer(Context* context) :
    Application(context)
{
}

void Urho3DPlayer::Setup()
{
	engineParameters_["WindowWidth"] = 800;
	engineParameters_["WindowHeight"] = 600;
	engineParameters_["WindowResizable"] = true;
	engineParameters_["FullScreen"] = false;
	engineParameters_["VSync"] = true;
	engineParameters_["WindowTitle"] = "Bitweb";
	engineParameters_["RenderPath"] = "CoreData/RenderPaths/Deferred.xml";
}

void Urho3DPlayer::Start()
{
	SetRandomSeed(GetSubsystem<Time>()->GetTimeSinceEpoch());
	input_ = GetSubsystem<Input>();
	input_->SetMouseVisible(true);
	//input_->SetTouchEmulation(true);
	graphics_ = GetSubsystem<Graphics>();
	cache_ = GetSubsystem<ResourceCache>();
	filesystem_ = GetSubsystem<FileSystem>();
	renderer_ = GetSubsystem<Renderer>();
	network_ = GetSubsystem<Network>();
	ui_ = GetSubsystem<UI>();
	engine_ = GetSubsystem<Engine>();
	audio_ = GetSubsystem<Audio>();

	new MainMenu(context_, this);
	//SubscribeToEvents();
}

void Urho3DPlayer::Stop()
{
}

void Urho3DPlayer::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    //SubscribeToEvent(E_UPDATE, HANDLER(Urho3DPlayer, HandleUpdate));
}

void Urho3DPlayer::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	timeStep_ = eventData[P_TIMESTEP].GetFloat();

}
