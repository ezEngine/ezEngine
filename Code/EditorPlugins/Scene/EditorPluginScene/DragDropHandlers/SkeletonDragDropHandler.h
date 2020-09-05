#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class ezSkeletonComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonComponentDragDropHandler, ezComponentDragDropHandler);

public:
  virtual float CanHandle(const ezDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;
};
