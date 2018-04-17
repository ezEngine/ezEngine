#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <Plugin.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Core/Graphics/Geometry.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
