#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>

class ezScene2Document;

class ezSceneLayerBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneLayerBase, ezReflectedClass);

public:
  ezSceneLayerBase();
  ~ezSceneLayerBase();

public:
  mutable ezScene2Document* m_pDocument = nullptr;
};

class ezSceneLayer : public ezSceneLayerBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneLayer, ezSceneLayerBase);

public:
  ezSceneLayer();
  ~ezSceneLayer();

public:
  ezUuid m_Layer;
};

class ezSceneDocumentSettings : public ezSceneDocumentSettingsBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentSettings, ezSceneDocumentSettingsBase);

public:
  ezSceneDocumentSettings();
  ~ezSceneDocumentSettings();

public:
  ezDynamicArray<ezSceneLayerBase*> m_Layers;
  mutable ezScene2Document* m_pDocument = nullptr;
};

struct ezScene2LayerEvent
{
  enum class Type
  {
    LayerAdded,
    LayerRemoved,
    ActiveLayerChanged,
  };

  Type m_Type;
  ezUuid m_layerGuid;
};

class ezScene2Document : public ezSceneDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScene2Document, ezSceneDocument);

public:
  ezScene2Document(const char* szDocumentPath);
  ~ezScene2Document();

  const ezDocumentObjectManager* GetSceneObjectManager() const { return m_pSceneObjectManager.Borrow(); }
  ezDocumentObjectManager* GetSceneObjectManager() { return m_pSceneObjectManager.Borrow(); }
  ezSelectionManager* GetSceneSelectionManager() const { return m_sceneSelectionManager.Borrow(); }
  ezCommandHistory* GetSceneCommandHistory() const { return m_pSceneCommandHistory.Borrow(); }
  ezObjectAccessorBase* GetSceneObjectAccessor() const { return m_pSceneObjectAccessor.Borrow(); }

  ezSelectionManager* GetLayerSelectionManager() const { return &m_LayerSelection; }

  ezStatus CreateLayer(const char* szName, const ezUuid& out_layerGuid);
  ezStatus DeleteLayer(const ezUuid& layerGuid);

  const ezUuid& GetActiveLayer() const;
  ezStatus SetActiveLayer(const ezUuid& layerGuid);

  bool IsLayerLoaded(const ezUuid& layerGuid);
  ezStatus SetLayerLoaded(const ezUuid& layerGuid, bool bLoaded);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;
  virtual const ezDocumentObject* GetSettingsObject() const override;

public:
  mutable ezEvent<const ezScene2LayerEvent&> m_LayerEvents;

private:
  void LayerSelectionEventHandler(const ezSelectionManagerEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  
  void UpdateLayers();
  void LayerAdded(const ezUuid& layerGuid);
  void LayerRemoved(const ezUuid& layerGuid);

private:
  friend class ezSceneLayer;
  ezCopyOnBroadcastEvent<const ezDocumentObjectStructureEvent&>::Unsubscriber m_structureEventSubscriber;
  ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&>::Unsubscriber m_layerSelectionEventSubscriber;
  ezEvent<const ezCommandHistoryEvent&, ezMutex>::Unsubscriber m_commandHistoryEventSubscriber;

  // This is used for a flattened list of the ezSceneDocumentSettings hierarchy
  struct LayerInfo
  {
    ezSceneDocument* m_pLayer = nullptr;
    bool m_bVisible = true;
  };

  // Scene document cache
  ezUniquePtr<ezDocumentObjectManager> m_pSceneObjectManager;
  mutable ezUniquePtr<ezCommandHistory> m_pSceneCommandHistory;
  mutable ezUniquePtr<ezSelectionManager> m_sceneSelectionManager;
  mutable ezUniquePtr<ezObjectCommandAccessor> m_pSceneObjectAccessor;

  // Layer state
  mutable ezSelectionManager m_LayerSelection;
  ezUuid m_ActiveLayerGuid;
  ezHashTable<ezUuid, LayerInfo> m_Layers;
};
