#include <Utilities/UtilitiesPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <Utilities/DGML/DGMLCreator.h>

void ezDGMLGraphCreator::FillGraphFromWorld(ezWorld* pWorld, ezDGMLGraph& ref_graph)
{
  if (!pWorld)
  {
    ezLog::Warning("ezDGMLGraphCreator::FillGraphFromWorld() called with null world!");
    return;
  }


  struct GraphVisitor
  {
    GraphVisitor(ezDGMLGraph& ref_graph)
      : m_Graph(ref_graph)
    {
      ezDGMLGraph::NodeDesc nd;
      nd.m_Color = ezColor::DarkRed;
      nd.m_Shape = ezDGMLGraph::NodeShape::Button;
      m_WorldNodeId = ref_graph.AddNode("World", &nd);
    }

    ezVisitorExecution::Enum Visit(ezGameObject* pObject)
    {
      ezStringBuilder name;
      name.SetFormat("GameObject: \"{0}\"", pObject->GetName().IsEmpty() ? "<Unnamed>" : pObject->GetName());

      // Create node for game object
      ezDGMLGraph::NodeDesc gameobjectND;
      gameobjectND.m_Color = ezColor::CornflowerBlue;
      gameobjectND.m_Shape = ezDGMLGraph::NodeShape::Rectangle;
      auto gameObjectNodeId = m_Graph.AddNode(name.GetData(), &gameobjectND);

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
        auto sComponentName = component->GetDynamicRTTI()->GetTypeName();

        ezDGMLGraph::NodeDesc componentND;
        componentND.m_Color = ezColor::LimeGreen;
        componentND.m_Shape = ezDGMLGraph::NodeShape::RoundedRectangle;
        auto componentNodeId = m_Graph.AddNode(sComponentName, &componentND);

        // And add the link to the game object

        m_Graph.AddConnection(componentNodeId, gameObjectNodeId);
      }

      return ezVisitorExecution::Continue;
    }

    ezDGMLGraph& m_Graph;

    ezDGMLGraph::NodeId m_WorldNodeId;
    ezMap<const ezGameObject*, ezDGMLGraph::NodeId> m_VisitedObjects;
  };

  GraphVisitor visitor(ref_graph);
  pWorld->Traverse(ezWorld::VisitorFunc(&GraphVisitor::Visit, &visitor), ezWorld::BreadthFirst);
}
