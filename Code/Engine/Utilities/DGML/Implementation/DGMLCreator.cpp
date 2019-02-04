#include <PCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Utilities/DGML/DGMLCreator.h>
#include <Utilities/DGML/DGMLWriter.h>

void ezDGMLGraphCreator::FillGraphFromWorld(ezWorld* pWorld, ezDGMLGraph& Graph)
{
  if (!pWorld)
  {
    ezLog::Warning("ezDGMLGraphCreator::FillGraphFromWorld() called with null world!");
    return;
  }


  struct GraphVisitor
  {
    GraphVisitor(ezDGMLGraph& Graph)
        : m_Graph(Graph)
    {
      m_WorldNodeId = Graph.AddNode("World", ezColor::DarkRed, ezDGMLGraph::NodeShape::Button);
    }

    ezVisitorExecution::Enum Visit(ezGameObject* pObject)
    {
      ezStringBuilder name;
      name.Format("GameObject: \"{0}\"", ezStringUtils::IsNullOrEmpty(pObject->GetName()) ? "<Unnamed>" : pObject->GetName());

      // Create node for game object
      auto gameObjectNodeId =
          m_Graph.AddNode(name.GetData(), ezColor::CornflowerBlue /* The Original! */, ezDGMLGraph::NodeShape::Rectangle);

      m_VisitedObjects.Insert(pObject, gameObjectNodeId);

      // Add connection to parent if existent
      if (const ezGameObject* parent = pObject->GetParent())
      {
        auto it = m_VisitedObjects.Find(parent);

        if (it.IsValid())
        {
          m_Graph.AddConnection(gameObjectNodeId, it.Value());
        }
      }
      else
      {
        // No parent -> connect to world
        m_Graph.AddConnection(gameObjectNodeId, m_WorldNodeId);
      }

      // Add components
      for (auto component : pObject->GetComponents())
      {
        auto szComponentName = component->GetDynamicRTTI()->GetTypeName();

        auto componentNodeId = m_Graph.AddNode(szComponentName, ezColor::LimeGreen, ezDGMLGraph::NodeShape::RoundedRectangle);

        // And add the link to the game object

        m_Graph.AddConnection(componentNodeId, gameObjectNodeId);
      }

      return ezVisitorExecution::Continue;
    }

    ezDGMLGraph& m_Graph;

    ezDGMLGraph::NodeId m_WorldNodeId;
    ezMap<const ezGameObject*, ezDGMLGraph::NodeId> m_VisitedObjects;
  };

  GraphVisitor visitor(Graph);
  pWorld->Traverse(ezWorld::VisitorFunc(&GraphVisitor::Visit, &visitor), ezWorld::BreadthFirst);
}



EZ_STATICLINK_FILE(Utilities, Utilities_DGML_Implementation_DGMLCreator);

