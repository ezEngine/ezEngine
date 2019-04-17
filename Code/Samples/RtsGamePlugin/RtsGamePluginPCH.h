#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <RtsGamePlugin/RtsGamePluginDLL.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Logging/Log.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Input/Declarations.h>
#include <Core/Input/InputManager.h>
#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

#include <System/Window/Window.h>

#include <GameEngine/DearImgui/DearImgui.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <GameEngine/Prefabs/PrefabResource.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <Utilities/DataStructures/ObjectSelection.h>
