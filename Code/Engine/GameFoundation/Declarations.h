#pragma once

#include <GameFoundation/Basics.h>
#include <Foundation/Types/Bitflags.h>

class ezWindowBase;
class ezWorld;
class ezWorldModule;
class ezGameApplication;

enum class ezGameUpdateState
{
  New,
  Running,
  Paused,
  Invalid,
};

enum class ezGameApplicationType
{
  StandAlone,
  EmbeddedInTool,
};


