#pragma once

#include <Core/World/Declarations.h>
#include <Utilities/UtilitiesDLL.h>

class ezWorld;
class ezDGMLGraph;

/// \brief Creates DGML (Directed Graph Markup Language) graphs from engine structures for visualization and debugging.
///
/// DGML is a Visual Studio format for representing directed graphs that can be viewed in the Visual Studio graph viewer.
/// This class provides utilities to export engine structures like world hierarchies into DGML format,
/// which is useful for debugging complex object relationships, component dependencies, and scene structure.
///
/// The generated DGML files can be opened in Visual Studio to provide an interactive graph view
/// where you can explore relationships, search for specific nodes, and analyze the structure visually.
class EZ_UTILITIES_DLL ezDGMLGraphCreator
{
public:
  /// \brief Adds the world hierarchy (game objects and components) to the given graph object.
  ///
  /// Creates nodes for each game object and component in the world, with edges representing
  /// parent-child relationships and component ownership. The resulting graph provides a complete
  /// view of the world's structure that can be visualized in tools supporting DGML format.
  ///
  /// \param pWorld The world whose hierarchy should be added to the graph. Must not be nullptr.
  /// \param ref_graph The DGML graph object that will receive the world structure data.
  static void FillGraphFromWorld(ezWorld* pWorld, ezDGMLGraph& ref_graph);
};
