#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <Plugin.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/View.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>
#include <Core/Graphics/Geometry.h>
