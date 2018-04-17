#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library


#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Communication/RemoteMessage.h>
