#pragma once

#include <EditorPluginProceduralPlacement/EditorPluginProceduralPlacementDLL.h>
#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class EZ_EDITORPLUGINPROCEDURALPLACEMENT_DLL ezProceduralPlacementComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementComponentDragDropHandler, ezComponentDragDropHandler);

public:

  float CanHandle(const ezDragDropInfo* pInfo) const;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;


};

