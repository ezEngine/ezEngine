#pragma once

#include <EditorPluginScene/DragDropHandlers/AssetDragDropHandler.h>

class ezMaterialDragDropHandler : public ezAssetDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialDragDropHandler, ezAssetDragDropHandler);

public:


protected:
  virtual void RequestConfiguration(ezDragDropConfig* pConfigToFillOut) override;
  virtual float CanHandle(const ezDragDropInfo* pInfo) const override;
  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;
  virtual void OnDragUpdate(const ezDragDropInfo* pInfo) override;
  virtual void OnDragCancel() override;
  virtual void OnDrop(const ezDragDropInfo* pInfo) override;

  ezUuid m_AppliedToComponent;
  ezInt32 m_uiAppliedToSlot;
};

