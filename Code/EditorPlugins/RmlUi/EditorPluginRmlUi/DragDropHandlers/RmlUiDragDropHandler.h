#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class ezRmlUiComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiComponentDragDropHandler, ezComponentDragDropHandler);

public:
  float CanHandle(const ezDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;
};
