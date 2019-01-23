#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Node.h>

class ezQtProceduralPlacementNode : public ezQtNode
{
public:
  ezQtProceduralPlacementNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateTitle() override;
};

