#pragma once

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpTool.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <McpPlugin/McpPluginDLL.h>
