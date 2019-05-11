#pragma once

#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>
#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class EZ_EDITORPLUGINPROCGEN_DLL ezProcPlacementComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcPlacementComponentDragDropHandler, ezComponentDragDropHandler);

public:

  float CanHandle(const ezDragDropInfo* pInfo) const;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;


};

