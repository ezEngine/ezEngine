#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <EnginePluginScene/EnginePluginSceneDLL.h>


#include <Foundation/Strings/StringBuilder.h>

#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
