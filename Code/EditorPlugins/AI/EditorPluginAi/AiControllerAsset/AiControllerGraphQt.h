#pragma once

#include <GuiFoundation/VisualGraph/Node.h>

/// Node in the AI controller graph.
///
/// Shows the short type name as the title. The subtitle is the name for sensors and considerations and the category and score for behaviors.
class ezQtAiControllerNode : public ezQtVisualGraphNode
{
public:
  ezQtAiControllerNode();

  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject) override;
  virtual void UpdateState() override;
};
