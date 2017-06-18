#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Types/Bitflags.h>

class ezWindowBase;
class ezWorld;
class ezWorldModule;
class ezGameApplication;


/// \brief The type of application that is currently active
enum class ezGameApplicationType
{
  StandAlone,             ///< The application is stand-alone (e.g. ezPlayer or a custom game)
  StandAloneMixedReality, ///< The application is stand-alone and runs on a VR headset or as an exclusive Hololens app.
  EmbeddedInTool,         ///< The application is embedded into a tool, such as the editor
};


