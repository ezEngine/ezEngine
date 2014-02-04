#pragma once

#include <InspectorPlugin/Plugin.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/Stats.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/IO/IBinaryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Time/Clock.h>
#include <Core/Input/InputManager.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

