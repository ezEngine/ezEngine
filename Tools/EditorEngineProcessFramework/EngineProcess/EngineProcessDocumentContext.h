#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

#include <Core/World/World.h>
#include <Foundation/Types/Uuid.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunication.h>

class ezEditorEngineSyncObjectMsg;
class ezEditorEngineSyncObject;
class ezEditorEngineDocumentMsg;
class ezEngineProcessViewContext;
class ezEngineProcessCommunicationChannel;
class ezProcessMessage;
class ezEntityMsgToEngine;
class ezExportDocumentMsgToEngine;
class ezCreateThumbnailMsgToEngine;
class ezScene;
struct ezResourceEvent;

template<typename HandleType>
class ezEditorGuidEngineHandleMap
{
public:
  void Clear()
  {
    m_GuidToHandle.Clear();
    m_HandleToGuid.Clear();
  }

  void RegisterObject(ezUuid guid, HandleType handle)
  {
    m_GuidToHandle[guid] = handle;
    m_HandleToGuid[handle] = guid;

    // apparently this happens during undo/redo (same guid, new handle on undo)
    //EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(ezUuid guid)
  {
    const HandleType handle = m_GuidToHandle[guid];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    // apparently this happens during undo/redo (same guid, new handle on undo)
    //EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(HandleType handle)
  {
    const ezUuid guid = m_HandleToGuid[handle];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    // apparently this happens during undo/redo (same guid, new handle on undo)
    //EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  HandleType GetHandle(ezUuid guid) const
  {
    HandleType res = HandleType();
    m_GuidToHandle.TryGetValue(guid, res);
    return res;
  }

  ezUuid GetGuid(HandleType handle) const
  {
    return m_HandleToGuid.GetValueOrDefault(handle, ezUuid());
  }

private:
  ezHashTable<ezUuid, HandleType> m_GuidToHandle;
  ezMap<HandleType, ezUuid> m_HandleToGuid;
};

/// \brief The world rtti converter context tracks created objects and is capable of also handling
///  components / game objects. Used by the ezIPCObjectMirror to create / destroy objects.
///
/// Atm it does not remove owner ptr when a parent is deleted, so it will accumulate zombie entries.
/// As requests to dead objects shouldn't generally happen this is for the time being not a problem.
class ezWorldRttiConverterContext : public ezRttiConverterContext
{
public:
  ezWorldRttiConverterContext() : m_pWorld(nullptr), m_uiNextComponentPickingID(1) {}

  virtual void Clear() override;

  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override;
  virtual void DeleteObject(const ezUuid& guid) override;

  virtual void RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject) override;
  virtual void UnregisterObject(const ezUuid& guid) override;

  virtual ezRttiConverterObject GetObjectByGUID(const ezUuid& guid) const override;
  virtual ezUuid GetObjectGUID(const ezRTTI* pRtti, const void* pObject) const override;

  ezWorld* m_pWorld;
  ezEditorGuidEngineHandleMap<ezGameObjectHandle> m_GameObjectMap;
  ezEditorGuidEngineHandleMap<ezComponentHandle> m_ComponentMap;

  ezEditorGuidEngineHandleMap<ezUInt32> m_OtherPickingMap;
  ezEditorGuidEngineHandleMap<ezUInt32> m_ComponentPickingMap;
  ezUInt32 m_uiNextComponentPickingID;
  ezUInt32 m_uiHighlightID;
};

enum class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineProcessMode
{
  Primary,
  PrimaryOwnWindow,
  Remote,
};

/// \brief A document context is the counter part to an editor document on the engine side.
///
/// For every document in the editor that requires engine output (rendering, picking, etc.), there is a ezEngineProcessDocumentContext
/// created in the engine process.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineProcessDocumentContext : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineProcessDocumentContext, ezReflectedClass);

public:
  ezEngineProcessDocumentContext();
  virtual ~ezEngineProcessDocumentContext();

  static ezEditorEngineProcessMode s_Mode;

  void Initialize(const ezUuid& DocumentGuid, ezEngineProcessCommunicationChannel* pIPC);
  void Deinitialize(bool bFullDestruction);

  void SendProcessMessage(ezProcessMessage* pMsg = nullptr);
  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg);

  static ezEngineProcessDocumentContext* GetDocumentContext(ezUuid guid);
  static void AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pView, ezEngineProcessCommunicationChannel* pIPC);
  static bool PendingOperationsInProgress();
  static void UpdateDocumentContexts();
  static void DestroyDocumentContext(ezUuid guid);

  // \brief Returns the bounding box of the objects in the world.
  ezBoundingBoxSphere GetWorldBounds(ezWorld* pWorld);

  void ProcessEditorEngineSyncObjectMsg(const ezEditorEngineSyncObjectMsg& msg);

  const ezUuid& GetDocumentGuid() const { return m_DocumentGuid; }

  virtual void Reset();

  ezIPCObjectMirrorEngine m_Mirror;
  ezWorldRttiConverterContext m_Context; //TODO: Move actual context into the EngineProcessDocumentContext

  ezWorld* GetWorld() const { return m_pWorld; }

protected:
  virtual void OnInitialize() {}
  virtual void OnDeinitialize() {}

  /// \brief Needs to be implemented to create a view context used for windows and thumbnails rendering.
  virtual ezEngineProcessViewContext* CreateViewContext() = 0;
  /// \brief Needs to be implemented to destroy the view context created in CreateViewContext.
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) = 0;

  /// \brief Should return true if this context has any operation in progress like thumbnail rendering
  /// and thus needs to continue rendering even if no new messages from the editor come in.
  virtual bool PendingOperationInProgress() const;

  /// \brief A tick functions that allows each document context to do processing that continues
  /// over multiple frames and can't be handled in HandleMessage directly.
  ///
  /// Make sure to call the base implementation when overwriting as this handles the thumbnail
  /// rendering that takes multiple frames to complete.
  virtual void UpdateDocumentContext();

  /// \brief Exports to current document resource to file. Make sure to write ezAssetFileHeader at the start of it.
  virtual bool ExportDocument(const ezExportDocumentMsgToEngine* pMsg);
  void UpdateSyncObjects();

  /// \brief Creates the thumbnail view context. It uses 'CreateViewContext' in combination with an off-screen render target.
  void CreateThumbnailViewContext(const ezCreateThumbnailMsgToEngine* pMsg);

  /// \brief Once a thumbnail is successfully rendered, the thumbnail view context is destroyed again.
  void DestroyThumbnailViewContext();

  /// \brief Overwrite this function to apply the thumbnail render settings to the given context.
  ///
  /// Return false if you need more frames to be rendered to setup everything correctly.
  /// If true is returned for 'ThumbnailConvergenceFramesTarget' frames in a row the thumbnail image is taken.
  /// This is to allow e.g. camera updates after more resources have been streamed in. The frame counter
  /// will start over to count to 'ThumbnailConvergenceFramesTarget' when a new resource is being loaded
  /// to make sure we do not make an image of half-streamed in data.
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext);

  /// \brief Called after a thumbnail context was created. Allows to insert code before the thumbnail is generated.
  virtual void OnThumbnailViewContextCreated() {}
  /// \brief Called before a thumbnail context is destroyed. Used for cleanup of what was done in OnThumbnailViewContextCreated()
  virtual void OnDestroyThumbnailViewContext() {}

  ezWorld* m_pWorld;

  /// \brief Sets or removes the given tag on the object and optionally all children
  void SetTagOnObject(const ezUuid& object, const char* szTag, bool bSet, bool recursive);

  /// \brief Sets the given tag on the object and all children.
  void SetTagRecursive(ezGameObject* pObject, const ezTag& tag);
  /// \brief Clears the given tag on the object and all children.
  void ClearTagRecursive(ezGameObject* pObject, const ezTag& tag);

private:
  friend class ezEditorEngineSyncObject;

  void AddSyncObject(ezEditorEngineSyncObject* pSync);
  void RemoveSyncObject(ezEditorEngineSyncObject* pSync);
  ezEditorEngineSyncObject* FindSyncObject(const ezUuid& guid);


private:
  void ResourceEventHandler(const ezResourceEvent& e);
  void ClearViewContexts();

  // Maps a document guid to the corresponding context that handles that document on the engine side
  static ezHashTable<ezUuid, ezEngineProcessDocumentContext*> s_DocumentContexts;

  /// Removes all sync objects that are tied to this context
  void CleanUpContextSyncObjects();

  ezUuid m_DocumentGuid;

  ezEngineProcessCommunicationChannel* m_pIPC;
  ezHybridArray<ezEngineProcessViewContext*, 4> m_ViewContexts;

  ezMap<ezUuid, ezEditorEngineSyncObject*> m_SyncObjects;

private:
  enum Constants
  {
    ThumbnailSuperscaleFactor = 4, ///< Thumbnail render target size is multiplied by this and then the final image is downscaled again. Needs to be pot.
    ThumbnailConvergenceFramesTarget = 2 ///< Due to multi-threaded rendering, this must be at least 2
  };

  ezUInt8 m_uiThumbnailConvergenceFrames;
  ezUInt16 m_uiThumbnailWidth;
  ezUInt16 m_uiThumbnailHeight;
  ezEngineProcessViewContext* m_pThumbnailViewContext;
  ezGALRenderTagetSetup m_ThumbnailRenderTargetSetup;
  ezGALTextureHandle m_hThumbnailColorRT;
  ezGALTextureHandle m_hThumbnailDepthRT;
  bool m_bWorldSimStateBeforeThumbnail;
};

