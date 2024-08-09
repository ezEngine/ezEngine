#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;
class ezGameStateBase;
class ezGameModeMsgToEngine;
class ezWorldSettingsMsgToEngine;
class ezObjectsForDebugVisMsgToEngine;
class ezGridSettingsMsgToEngine;
class ezSimulationSettingsMsgToEngine;
struct ezResourceManagerEvent;
class ezExposedDocumentObjectPropertiesMsgToEngine;
class ezViewRedrawMsgToEngine;
class ezWorldWriter;
class ezDeferredFileWriter;
class ezLayerContext;
struct ezGameApplicationExecutionEvent;

class EZ_ENGINEPLUGINSCENE_DLL ezSceneContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneContext, ezEngineProcessDocumentContext);

public:
  ezSceneContext();
  ~ezSceneContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezDeque<ezGameObjectHandle>& GetSelection() const { return m_Selection; }
  const ezDeque<ezGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  bool GetRenderShapeIcons() const { return m_bRenderShapeIcons; }
  bool GetRenderSelectionBoxes() const { return m_bRenderSelectionBoxes; }
  float GetGridDensity() const { return ezMath::Abs(m_fGridDensity); }
  bool IsGridInGlobalSpace() const { return m_fGridDensity >= 0.0f; }
  ezTransform GetGridTransform() const { return m_GridTransform; }

  ezGameStateBase* GetGameState() const;
  bool IsPlayTheGameActive() const { return GetGameState() != nullptr; }

  ezUInt32 RegisterLayer(ezLayerContext* pLayer);
  void UnregisterLayer(ezLayerContext* pLayer);
  void AddLayerIndexTag(const ezEntityMsgToEngine& msg, ezWorldRttiConverterContext& ref_context, const ezTag& layerTag);
  const ezArrayPtr<const ezTag> GetInvisibleLayerTags() const;

  ezEngineProcessDocumentContext* GetActiveDocumentContext();
  const ezEngineProcessDocumentContext* GetActiveDocumentContext() const;
  ezWorldRttiConverterContext& GetActiveContext();
  const ezWorldRttiConverterContext& GetActiveContext() const;
  ezWorldRttiConverterContext* GetContextForLayer(const ezUuid& layerGuid);
  ezArrayPtr<ezWorldRttiConverterContext*> GetAllContexts();

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual ezStatus ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;
  void ExportExposedParameters(const ezWorldWriter& ww, ezDeferredFileWriter& file) const;

  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;
  virtual void OnThumbnailViewContextCreated() override;
  virtual void OnDestroyThumbnailViewContext() override;
  virtual void UpdateDocumentContext() override;
  virtual ezGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, ezComponentHandle hThis, ezStringView sProperty) const override;

private:
  struct TagGameObject
  {
    ezGameObjectHandle m_hObject;
    ezTag m_Tag;
  };

  void AddAmbientLight(bool bSetEditorTag, bool bForce);
  void RemoveAmbientLight();

  void HandleViewRedrawMsg(const ezViewRedrawMsgToEngine* pMsg);
  void HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg);
  void HandleGameModeMsg(const ezGameModeMsgToEngine* pMsg);
  void HandleSimulationSettingsMsg(const ezSimulationSettingsMsgToEngine* msg);
  void HandleGridSettingsMsg(const ezGridSettingsMsgToEngine* msg);
  void HandleWorldSettingsMsg(const ezWorldSettingsMsgToEngine* msg);
  void HandleObjectsForDebugVisMsg(const ezObjectsForDebugVisMsgToEngine* pMsg);
  void ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds);
  void HandleExposedPropertiesMsg(const ezExposedDocumentObjectPropertiesMsgToEngine* pMsg);
  void HandleSceneGeometryMsg(const ezExportSceneGeometryMsgToEngine* pMsg);
  void HandlePullObjectStateMsg(const ezPullObjectStateMsgToEngine* pMsg);
  void AnswerObjectStatePullRequest(const ezViewRedrawMsgToEngine* pMsg);
  void HandleActiveLayerChangedMsg(const ezActiveLayerChangedMsgToEngine* pMsg);
  void HandleTagMsgToEngineMsg(const ezObjectTagMsgToEngine* pMsg);
  void HandleLayerVisibilityChangedMsgToEngineMsg(const ezLayerVisibilityChangedMsgToEngine* pMsg);

  void DrawSelectionBounds(const ezViewHandle& hView);

  void UpdateInvisibleLayerTags();
  void InsertSelectedChildren(const ezGameObject* pObject);
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);
  void OnSimulationEnabled();
  void OnSimulationDisabled();
  void OnPlayTheGameModeStarted(ezStringView sStartPosition, const ezTransform& startPositionOffset);

  void OnResourceManagerEvent(const ezResourceManagerEvent& e);
  void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);

  bool m_bUpdateAllLocalBounds = false;
  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  bool m_bRenderSelectionBoxes;
  float m_fGridDensity;
  ezTransform m_GridTransform;

  ezDeque<ezGameObjectHandle> m_Selection;
  ezDeque<ezGameObjectHandle> m_SelectionWithChildren;
  ezSet<ezGameObjectHandle> m_SelectionWithChildrenSet;
  ezGameObjectHandle m_hSkyLight;
  ezGameObjectHandle m_hDirectionalLight;
  ezDynamicArray<ezExposedSceneProperty> m_ExposedSceneProperties;

  ezPushObjectStateMsgToEditor m_PushObjectStateMsg;

  ezUuid m_ActiveLayer;
  ezDynamicArray<ezLayerContext*> m_Layers;
  ezDynamicArray<ezWorldRttiConverterContext*> m_Contexts;

  // We use tags in the form of Layer_4 (Layer_Scene for the scene itself) to not pollute the tag registry with hundreds of unique tags. The tags do not need to be unique across documents so we can just use the layer index but that requires the Tags to be recomputed whenever we remove / add layers.
  // By caching the guids we do not need to send another message each time a layer is loaded as we send also guids of unloaded layers.
  ezTag m_LayerTag;
  ezHybridArray<ezUuid, 1> m_InvisibleLayers;
  bool m_bInvisibleLayersDirty = true;
  ezHybridArray<ezTag, 1> m_InvisibleLayerTags;

  ezDynamicArray<TagGameObject> m_ObjectsToTag;

  static ezWorld* s_pWorldLinkedWithGameState;
};
