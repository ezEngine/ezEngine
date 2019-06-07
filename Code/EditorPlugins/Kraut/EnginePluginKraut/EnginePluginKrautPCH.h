#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <EnginePluginKraut/EnginePluginKrautDLL.h>


#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Core/Graphics/Geometry.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>
