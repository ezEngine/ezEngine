#pragma once

#include <EditorPluginScene/DragDropHandlers/ComponentDragDropHandler.h>

class ezMeshComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshComponentDragDropHandler, ezComponentDragDropHandler);

public:

  float CanHandle(const ezDragDropInfo* pInfo) const;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;


};

