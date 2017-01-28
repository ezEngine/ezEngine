
#include <Utilities/PCH.h>
#include <Utilities/DGML/DGMLWriter.h>

ezDGMLGraph::ezDGMLGraph(ezDGMLGraph::Direction GraphDirection /*= LeftToRight*/, ezDGMLGraph::Layout GraphLayout /*= Tree*/)
  : m_Direction( GraphDirection ),
    m_Layout( GraphLayout )
{
}

ezDGMLGraph::NodeId ezDGMLGraph::AddNode(const char* szTitle, ezColor Color /*= ezColor::GetWhite()*/, NodeShape Shape /*= Rectangle*/)
{
  ezDGMLGraph::Node& Node = m_Nodes.ExpandAndGetRef();

  Node.m_Title = szTitle;
  Node.m_Color = Color;
  Node.m_Shape = Shape;

  return m_Nodes.GetCount() - 1;
}

void ezDGMLGraph::AddConnection(ezDGMLGraph::NodeId Source, ezDGMLGraph::NodeId Target)
{
  ezDGMLGraph::Connection& Connection = m_Connections.ExpandAndGetRef();

  Connection.m_Source = Source;
  Connection.m_Target = Target;
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

  StringBuilder.AppendFormat("<DirectedGraph xmlns=\"http://schemas.microsoft.com/vs/2009/dgml\" GraphDirection=\"{0}\" Layout=\"{1}\">\n", szDirection, szLayout);

  // Write out all the nodes
  StringBuilder.Append("\t<Nodes>\n");
  for (ezUInt32 i = 0; i < Graph.m_Nodes.GetCount(); ++i)
  {
    ezStringBuilder SanitizedName = Graph.m_Nodes[i].m_Title;
    SanitizedName.ReplaceAll("&", "&#038;");
    SanitizedName.ReplaceAll("<", "&lt;");
    SanitizedName.ReplaceAll(">", "&gt;");
    SanitizedName.ReplaceAll("\"", "&quot;");
    SanitizedName.ReplaceAll("'", "&apos;");
    SanitizedName.ReplaceAll("\n", "&#xA;");

    ezStringBuilder ColorValue = "#FF";
    ezColorGammaUB RGBA(Graph.m_Nodes[i].m_Color);
    ColorValue.AppendFormat("{0}{1}{2}", ezArgU(RGBA.r, 2, true, 16, true), ezArgU(RGBA.g, 2, true, 16, true), ezArgU(RGBA.b, 2, true, 16, true));

    ezStringBuilder StyleString;
    switch (Graph.m_Nodes[i].m_Shape)
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

    StringBuilder.AppendFormat("\t\t<Node Id=\"N_{0}\" Label=\"{1}\" Background=\"{2}\" {3} />\n", i, SanitizedName.GetData(), ColorValue.GetData(), StyleString.GetData());

  }
  StringBuilder.Append("\t</Nodes>\n");

  // Write out the links
  StringBuilder.Append("\t<Links>\n");
  for (ezUInt32 i = 0; i < Graph.m_Connections.GetCount(); ++i)
  {
    StringBuilder.AppendFormat("\t\t<Link Source=\"N_{0}\" Target=\"N_{1}\" />\n", Graph.m_Connections[i].m_Source, Graph.m_Connections[i].m_Target);
  }
  StringBuilder.Append("\t</Links>\n");

  StringBuilder.Append("</DirectedGraph>\n");

  return EZ_SUCCESS;
}
