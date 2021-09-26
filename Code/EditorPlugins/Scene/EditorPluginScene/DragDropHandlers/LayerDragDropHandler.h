#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

/// \brief Base class for drag and drop handler that drop on a ezSceneLayer.
class ezLayerDragDropHandler : public ezDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerDragDropHandler, ezDragDropHandler);

public:
  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override {}
  virtual void OnDragUpdate(const ezDragDropInfo* pInfo) override {}
  virtual void OnDragCancel() override {}

protected:
  const ezRTTI* GetCommonBaseType(const ezDragDropInfo* pInfo) const;
};

class ezLayerOnLayerDragDropHandler : public ezLayerDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerOnLayerDragDropHandler, ezLayerDragDropHandler);

public:
  virtual float CanHandle(const ezDragDropInfo* pInfo) const override;
  virtual void OnDrop(const ezDragDropInfo* pInfo) override;
};

class ezGameObjectOnLayerDragDropHandler : public ezLayerDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectOnLayerDragDropHandler, ezLayerDragDropHandler);

public:
  virtual float CanHandle(const ezDragDropInfo* pInfo) const override;
  virtual void OnDrop(const ezDragDropInfo* pInfo) override;
};
