#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <GuiFoundation/VisualGraph/Pin.h>

/// Qt graphics item for animation graph nodes.
///
/// Visual representation of nodes in an animation graph, such as animation clips, blend nodes, or state transitions.
class ezQtAnimationGraphNode : public ezQtVisualGraphNode
{
public:
  ezQtAnimationGraphNode();

  virtual void UpdateState() override;
};
