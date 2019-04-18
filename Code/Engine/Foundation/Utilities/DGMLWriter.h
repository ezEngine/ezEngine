#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Strings/String.h>

class ezFormatString;

/// \brief This class encapsulates building a DGML compatible graph.
class EZ_FOUNDATION_DLL ezDGMLGraph
{
public:
  enum class Direction : ezUInt8
  {
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft
  };

  enum class Layout : ezUInt8
  {
    Free,
    Tree,
    DependencyMatrix
  };

  enum class NodeShape : ezUInt8
  {
    None,
    Rectangle,
    RoundedRectangle,
    Button
  };

  enum class GroupType : ezUInt8
  {
    None,
    Expanded,
    Collapsed,
  };

  typedef ezUInt32 NodeId;
  typedef ezUInt32 PropertyId;
  typedef ezUInt32 ConnectionId;

  struct NodeDesc
  {
    ezColor m_Color = ezColor::White;
    NodeShape m_Shape = NodeShape::Rectangle;
  };

  /// \brief Constructor for the graph.
  ezDGMLGraph(Direction GraphDirection = Direction::LeftToRight, Layout GraphLayout = Layout::Tree);

  /// \brief Adds a node to the graph.
  /// Adds a node to the graph and returns the node id which can be used to reference the node later to add connections etc.
  NodeId AddNode(const char* szTitle, const NodeDesc* desc = nullptr);

  /// \brief Adds a DGML node that can act as a group for other nodes
  NodeId AddGroup(const char* szTitle, GroupType type, const NodeDesc* desc = nullptr);

  /// \brief Inserts a node into an existing group node.
  void AddNodeToGroup(NodeId node, NodeId group);

  /// \brief Adds a directed connection to the graph (an arrow pointing from source to target node).
  ConnectionId AddConnection(NodeId Source, NodeId Target);

  /// \brief Adds a property type. All properties currently use the data type 'string'
  PropertyId AddPropertyType(const char* szName);

  /// \brief Adds a property of the specified type with the given value to a node
  void AddNodeProperty(NodeId node, PropertyId property, const ezFormatString& fmt);

protected:
  friend class ezDGMLGraphWriter;

  struct Connection
  {
    NodeId m_Source;
    NodeId m_Target;

    EZ_DECLARE_POD_TYPE();
  };

  struct PropertyType
  {
    ezString m_Name;
  };

  struct PropertyValue
  {
    PropertyId m_PropertyId;
    ezString m_sValue;
  };

  struct Node
  {
    ezString m_Title;
    GroupType m_GroupType = GroupType::None;
    NodeId m_ParentGroup = 0xFFFFFFFF;
    NodeDesc m_Desc;
    ezDynamicArray<PropertyValue> m_Properties;
  };

  ezHybridArray<Node, 16> m_Nodes;

  ezHybridArray<Connection, 32> m_Connections;

  ezHybridArray<PropertyType, 16> m_PropertyTypes;

  Direction m_Direction;

  Layout m_Layout;
};

/// \brief This class encapsulates the output of DGML compatible graphs to files and streams.
class EZ_FOUNDATION_DLL ezDGMLGraphWriter
{
public:
  /// \brief Helper method to write the graph to a file.
  static ezResult WriteGraphToFile(const char* szFileName, const ezDGMLGraph& Graph);

  /// \brief Writes the graph as a DGML formatted document to the given string builder.
  static ezResult WriteGraphToString(ezStringBuilder& StringBuilder, const ezDGMLGraph& Graph);
};
