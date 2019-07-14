#pragma once

#include <Foundation/Basics.h>
#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

#ifdef EZ_USE_QT
#  include <QWidget>

// QtWidgets includes Windows.h
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <QtWidgets>
#endif
