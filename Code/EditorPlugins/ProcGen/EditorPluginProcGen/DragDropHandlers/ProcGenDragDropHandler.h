#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>

class EZ_EDITORPLUGINPROCGEN_DLL ezProcPlacementComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcPlacementComponentDragDropHandler, ezComponentDragDropHandler);

public:
  virtual float CanHandle(const ezDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;
};
