#pragma once

#include <Core/World/Declarations.h>
#include <Utilities/Basics.h>

class ezWorld;
class ezDGMLGraph;

/// \brief This class encapsulates creating graphs from various core engine structures (like the game object graph etc.)
class EZ_UTILITIES_DLL ezDGMLGraphCreator
{
public:
  /// \brief Adds the world hierarchy (game objects and components) to the given graph object.
  static void FillGraphFromWorld(ezWorld* pWorld, ezDGMLGraph& Graph);
};
