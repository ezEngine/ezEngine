#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class ezKrautTreeComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeComponentDragDropHandler, ezComponentDragDropHandler);

public:

  float CanHandle(const ezDragDropInfo* pInfo) const;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;


};

