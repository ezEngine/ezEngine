#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class ezDecalComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalComponentDragDropHandler, ezComponentDragDropHandler);

public:
  virtual float CanHandle(const ezDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;
};
