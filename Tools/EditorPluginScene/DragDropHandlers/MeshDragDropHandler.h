#pragma once

#include <EditorPluginScene/DragDropHandlers/ComponentDragDropHandler.h>

class ezMeshComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshComponentDragDropHandler, ezComponentDragDropHandler);

public:

  bool CanHandle(const ezDragDropInfo* pInfo) const;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;


};

