#include <FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <Foundation/Strings/FormatString.h>

ezDGMLGraph::ezDGMLGraph(ezDGMLGraph::Direction GraphDirection /*= LeftToRight*/, ezDGMLGraph::Layout GraphLayout /*= Tree*/)
  : m_Direction(GraphDirection)
  , m_Layout(GraphLayout)
{
}

ezDGMLGraph::NodeId ezDGMLGraph::AddNode(const char* szTitle, const NodeDesc* desc)
{
  return AddGroup(szTitle, GroupType::None, desc);
}

ezDGMLGraph::NodeId ezDGMLGraph::AddGroup(const char* szTitle, GroupType type, const NodeDesc* desc /*= nullptr*/)
{
  ezDGMLGraph::Node& Node = m_Nodes.ExpandAndGetRef();

  Node.m_Title = szTitle;
  Node.m_GroupType = type;

  if (desc)
  {
    Node.m_Desc = *desc;
  }

  return m_Nodes.GetCount() - 1;
}

void ezDGMLGraph::AddNodeToGroup(NodeId node, NodeId group)
{
  EZ_ASSERT_DEBUG(m_Nodes[group].m_GroupType != GroupType::None, "The given group node has not been created as a group node");

  m_Nodes[node].m_ParentGroup = group;
}

ezDGMLGraph::ConnectionId ezDGMLGraph::AddConnection(ezDGMLGraph::NodeId Source, ezDGMLGraph::NodeId Target)
{
  ezDGMLGraph::Connection& Connection = m_Connections.ExpandAndGetRef();

  Connection.m_Source = Source;
  Connection.m_Target = Target;

  return m_Connections.GetCount() - 1;
}

ezDGMLGraph::PropertyId ezDGMLGraph::AddPropertyType(const char* szName)
{
  auto& prop = m_PropertyTypes.ExpandAndGetRef();
  prop.m_Name = szName;
  return m_PropertyTypes.GetCount() - 1;
}

void ezDGMLGraph::AddNodeProperty(NodeId node, PropertyId property, const ezFormatString& fmt)
{
  ezStringBuilder tmp;

  auto& prop = m_Nodes[node].m_Properties.ExpandAndGetRef();
  prop.m_PropertyId = property;
  prop.m_sValue = fmt.GetText(tmp);
}

ezResult ezDGMLGraphWriter::WriteGraphToFile(const char* szFileName, const ezDGMLGraph& Graph)
{
  ezStringBuilder StringBuilder;

  // Write to memory object and then to file
  if (WriteGraphToString(StringBuilder, Graph).Succeeded())
  {
    ezFileWriter FileWriter;
    if (!FileWriter.Open(szFileName).Succeeded())
      return EZ_FAILURE;

    FileWriter.WriteBytes(StringBuilder.GetData(), StringBuilder.GetElementCount());

    FileWriter.Close();

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezDGMLGraphWriter::WriteGraphToString(ezStringBuilder& StringBuilder, const ezDGMLGraph& Graph)
{
  const char* szDirection = nullptr;
  const char* szLayout = nullptr;

  switch (Graph.m_Direction)
  {
    case ezDGMLGraph::Direction::TopToBottom:
      szDirection = "TopToBottom";
      break;
    case ezDGMLGraph::Direction::BottomToTop:
      szDirection = "BottomToTop";
      break;
    case ezDGMLGraph::Direction::LeftToRight:
      szDirection = "LeftToRight";
      break;
    case ezDGMLGraph::Direction::RightToLeft:
      szDirection = "RightToLeft";
      break;
  }

  switch (Graph.m_Layout)
  {
    case ezDGMLGraph::Layout::Free:
      szLayout = "None";
      break;
    case ezDGMLGraph::Layout::Tree:
      szLayout = "Sugiyama";
      break;
    case ezDGMLGraph::Layout::DependencyMatrix:
      szLayout = "DependencyMatrix";
      break;
  }

  StringBuilder.AppendFormat(
    "<DirectedGraph xmlns=\"http://schemas.microsoft.com/vs/2009/dgml\" GraphDirection=\"{0}\" Layout=\"{1}\">\n", szDirection, szLayout);

  // Write out all the properties
  if (!Graph.m_PropertyTypes.IsEmpty())
  {
    StringBuilder.Append("\t<Properties>\n");

    for (ezUInt32 i = 0; i < Graph.m_PropertyTypes.GetCount(); ++i)
    {
      const auto& prop = Graph.m_PropertyTypes[i];

      StringBuilder.AppendFormat("\t\t<Property Id=\"P_{0}\" Label=\"{1}\" DataType=\"String\"/>\n", i, prop.m_Name);
    }

    StringBuilder.Append("\t</Properties>\n");
  }

  // Write out all the nodes
  if (!Graph.m_Nodes.IsEmpty())
  {
    ezStringBuilder ColorValue;
    ezStringBuilder PropertiesString;
    ezStringBuilder SanitizedName;
    const char* szGroupString;

    StringBuilder.Append("\t<Nodes>\n");
    for (ezUInt32 i = 0; i < Graph.m_Nodes.GetCount(); ++i)
    {
      const ezDGMLGraph::Node& node = Graph.m_Nodes[i];

      SanitizedName = node.m_Title;
      SanitizedName.ReplaceAll("&", "&#038;");
      SanitizedName.ReplaceAll("<", "&lt;");
      SanitizedName.ReplaceAll(">", "&gt;");
      SanitizedName.ReplaceAll("\"", "&quot;");
      SanitizedName.ReplaceAll("'", "&apos;");
      SanitizedName.ReplaceAll("\n", "&#xA;");

      ColorValue = "#FF";
      ezColorGammaUB RGBA(node.m_Desc.m_Color);
      ColorValue.AppendFormat(
        "{0}{1}{2}", ezArgU(RGBA.r, 2, true, 16, true), ezArgU(RGBA.g, 2, true, 16, true), ezArgU(RGBA.b, 2, true, 16, true));

      ezStringBuilder StyleString;
      switch (node.m_Desc.m_Shape)
      {
        case ezDGMLGraph::NodeShape::None:
          StyleString = "Shape=\"None\"";
          break;
        case ezDGMLGraph::NodeShape::Rectangle:
          StyleString = "NodeRadius=\"0\"";
          break;
        case ezDGMLGraph::NodeShape::RoundedRectangle:
          StyleString = "NodeRadius=\"4\"";
          break;
        case ezDGMLGraph::NodeShape::Button:
          StyleString = "";
          break;
      }

      switch (node.m_GroupType)
      {

        case ezDGMLGraph::GroupType::Expanded:
          szGroupString = " Group=\"Expanded\"";
          break;

        case ezDGMLGraph::GroupType::Collapsed:
          szGroupString = " Group=\"Collapsed\"";
          break;

        case ezDGMLGraph::GroupType::None:
        default:
          szGroupString = nullptr;
          break;
      }

      PropertiesString.Clear();
      for (const auto& prop : node.m_Properties)
      {
        PropertiesString.AppendFormat(" {0}=\"{1}\"", Graph.m_PropertyTypes[prop.m_PropertyId].m_Name, prop.m_sValue);
      }

      StringBuilder.AppendFormat(
        "\t\t<Node Id=\"N_{0}\" Label=\"{1}\" Background=\"{2}\" {3}{4}{5}/>\n", i, SanitizedName, ColorValue, StyleString, szGroupString, PropertiesString);
    }
    StringBuilder.Append("\t</Nodes>\n");
  }

  // Write out the links
  if (!Graph.m_Connections.IsEmpty())
  {
    StringBuilder.Append("\t<Links>\n");
    {
      for (ezUInt32 i = 0; i < Graph.m_Connections.GetCount(); ++i)
      {
        StringBuilder.AppendFormat(
          "\t\t<Link Source=\"N_{0}\" Target=\"N_{1}\" />\n", Graph.m_Connections[i].m_Source, Graph.m_Connections[i].m_Target);
      }

      for (ezUInt32 i = 0; i < Graph.m_Nodes.GetCount(); ++i)
      {
        const ezDGMLGraph::Node& node = Graph.m_Nodes[i];

        if (node.m_ParentGroup != 0xFFFFFFFF)
        {
          StringBuilder.AppendFormat("\t\t<Link Category=\"Contains\" Source=\"N_{0}\" Target=\"N_{1}\" />\n", node.m_ParentGroup, i);
        }
      }
    }
    StringBuilder.Append("\t</Links>\n");
  }

  StringBuilder.Append("</DirectedGraph>\n");

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Utilities, Utilities_DGML_Implementation_DGMLWriter);
