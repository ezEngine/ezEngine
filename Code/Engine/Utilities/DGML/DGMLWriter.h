
#pragma once

#include <Utilities/Basics.h>

/// \brief This class encapsulates building a DGML compatible graph.
class EZ_UTILITIES_DLL ezDGMLGraph
{
public:

  enum class Direction
  {
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft
  };

  enum class Layout
  {
    Free,
    Tree,
    DependencyMatrix
  };

  enum class NodeShape
  {
    None,
    Rectangle,
    RoundedRectangle,
    Button
  };

  typedef ezUInt32 NodeId;

  struct Connection
  {
    NodeId m_Source;
    NodeId m_Target;
    
    EZ_DECLARE_POD_TYPE();
  };

  struct Node
  {
    ezString64 m_Title;
    ezColor m_Color;
    NodeShape m_Shape;
  };

  /// \brief Constructor for the graph.
  ezDGMLGraph(Direction GraphDirection = Direction::LeftToRight, Layout GraphLayout = Layout::Tree);

  /// \brief Adds a node to the graph.
  /// Adds a node to the graph and returns the node id which can be used to reference the node later to add connections etc.
  NodeId AddNode(const char* szTitle, ezColor Color = ezColor::White, NodeShape Shape = NodeShape::Rectangle);

  /// \brief Adds a connection to the graph.
  /// Adds a directed connection to the graph (an arrow pointing from source to target node).
  void AddConnection(NodeId Source, NodeId Target);

protected:

  friend class ezDGMLGraphWriter;

  ezHybridArray<Node, 16> m_Nodes;

  ezHybridArray<Connection, 32> m_Connections;

  Direction m_Direction;

  Layout m_Layout;
};

/// \brief This class encapsulates the output of DGML compatible graphs to files and streams.
class EZ_UTILITIES_DLL ezDGMLGraphWriter
{
public:

  /// \brief Helper method to write the graph to a file.
  static ezResult WriteGraphToFile(const char* szFileName, const ezDGMLGraph& Graph);

  /// \brief Writes the graph as a DGML formatted document to the given string builder.
  static ezResult WriteGraphToString(ezStringBuilder& StringBuilder, const ezDGMLGraph& Graph);
};

