#pragma once

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

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
    LayerLoaded,
    LayerUnloaded,
    LayerVisible,
    LayerInvisible,
    ActiveLayerChanged,
  };

  Type m_Type;
  ezUuid m_layerGuid;
};

class EZ_EDITORPLUGINSCENE_DLL ezScene2Document : public ezSceneDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScene2Document, ezSceneDocument);

public:
  ezScene2Document(ezStringView sDocumentPath);
  ~ezScene2Document();

  /// \name Scene Data Accessors
  ///@{

  const ezDocumentObjectManager* GetSceneObjectManager() const { return m_pSceneObjectManager.Borrow(); }
  ezDocumentObjectManager* GetSceneObjectManager() { return m_pSceneObjectManager.Borrow(); }
  ezSelectionManager* GetSceneSelectionManager() const { return m_pSceneSelectionManager.Borrow(); }
  ezCommandHistory* GetSceneCommandHistory() const { return m_pSceneCommandHistory.Borrow(); }
  ezObjectAccessorBase* GetSceneObjectAccessor() const { return m_pSceneObjectAccessor.Borrow(); }
  const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>* GetSceneDocumentObjectMetaData() const { return m_pSceneDocumentObjectMetaData.Borrow(); }
  ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>* GetSceneDocumentObjectMetaData() { return m_pSceneDocumentObjectMetaData.Borrow(); }
  const ezObjectMetaData<ezUuid, ezGameObjectMetaData>* GetSceneGameObjectMetaData() const { return m_pSceneGameObjectMetaData.Borrow(); }
  ezObjectMetaData<ezUuid, ezGameObjectMetaData>* GetSceneGameObjectMetaData() { return m_pSceneGameObjectMetaData.Borrow(); }

  ///@}
  /// \name Layer Functions
  ///@{

  ezSelectionManager* GetLayerSelectionManager() const { return m_pLayerSelection.Borrow(); }

  ezStatus CreateLayer(const char* szName, ezUuid& out_layerGuid);
  ezStatus DeleteLayer(const ezUuid& layerGuid);

  const ezUuid& GetActiveLayer() const;
  ezStatus SetActiveLayer(const ezUuid& layerGuid);

  bool IsLayerLoaded(const ezUuid& layerGuid) const;
  ezStatus SetLayerLoaded(const ezUuid& layerGuid, bool bLoaded);
  void GetAllLayers(ezDynamicArray<ezUuid>& out_layerGuids);
  void GetLoadedLayers(ezDynamicArray<ezSceneDocument*>& out_layers) const;

  bool IsLayerVisible(const ezUuid& layerGuid) const;
  ezStatus SetLayerVisible(const ezUuid& layerGuid, bool bVisible);

  const ezDocumentObject* GetLayerObject(const ezUuid& layerGuid) const;
  ezSceneDocument* GetLayerDocument(const ezUuid& layerGuid) const;

  virtual ezGameObjectDocument* GetRedirectedGameObjectDoc() override;

  bool IsAnyLayerModified() const;

  ///@}
  /// \name Base Class Functions
  ///@{

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;
  virtual const ezDocumentObject* GetSettingsObject() const override;
  virtual void HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  virtual ezTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override;
  virtual void SendGameWorldToEngine() override;
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& assetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ///@}
  /// \name Selection Specific Functions
  ///@{

  void PreventDoubleSelectionChange(bool b);
  virtual void UndoSelection() override;

  ///@}


public:
  mutable ezEvent<const ezScene2LayerEvent&> m_LayerEvents;

private:
  void LayerSelectionEventHandler(const ezSelectionManagerEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);
  void HandleObjectStateFromEngineMsg2(const ezPushObjectStateMsgToEditor* pMsg);

  void UpdateLayers();
  void SendLayerVisibility();
  void LayerAdded(const ezUuid& layerGuid, const ezUuid& layerObjectGuid);
  void LayerRemoved(const ezUuid& layerGuid);

private:
  friend class ezSceneLayer;
  ezCopyOnBroadcastEvent<const ezDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventSubscriber;
  ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&>::Unsubscriber m_LayerSelectionEventSubscriber;
  ezEvent<const ezCommandHistoryEvent&, ezMutex>::Unsubscriber m_CommandHistoryEventSubscriber;
  ezCopyOnBroadcastEvent<const ezDocumentManager::Event&>::Unsubscriber m_DocumentManagerEventSubscriber;

  // This is used for a flattened list of the ezSceneDocumentSettings hierarchy
  struct LayerInfo
  {
    ezSceneDocument* m_pLayer = nullptr;
    ezUuid m_objectGuid;
    bool m_bVisible = true;
  };

  // Scene document cache
  ezUniquePtr<ezDocumentObjectManager> m_pSceneObjectManager;
  mutable ezUniquePtr<ezCommandHistory> m_pSceneCommandHistory;
  mutable ezUniquePtr<ezSelectionManager> m_pSceneSelectionManager;
  mutable ezUniquePtr<ezObjectCommandAccessor> m_pSceneObjectAccessor;
  ezUniquePtr<ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>> m_pSceneDocumentObjectMetaData;
  ezUniquePtr<ezObjectMetaData<ezUuid, ezGameObjectMetaData>> m_pSceneGameObjectMetaData;

  // Layer state
  mutable ezUniquePtr<ezSelectionManager> m_pLayerSelection;
  ezUuid m_ActiveLayerGuid;
  ezHashTable<ezUuid, LayerInfo> m_Layers;

  void ActiveLayerGameObjectEventHandler(const ezGameObjectEvent& e);

  ezEvent<const ezGameObjectEvent&>::Unsubscriber m_ActiveLayerGoEvUnsubscriber;
};
