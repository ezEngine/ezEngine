#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Types/Uuid.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>

class ezEditorEngineSyncObjectMsg;
class ezEditorEngineSyncObject;
class ezEditorEngineDocumentMsg;
class ezEngineProcessViewContext;
class ezEngineProcessCommunicationChannel;
class ezProcessMessage;
class ezExportDocumentMsgToEngine;
class ezCreateThumbnailMsgToEngine;
struct ezResourceEvent;

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

  void Initialize(const ezUuid& DocumentGuid, ezEngineProcessCommunicationChannel* pIPC);
  void Deinitialize();

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
  void ClearExistingObjects();

  ezIPCObjectMirrorEngine m_Mirror;
  ezWorldRttiConverterContext m_Context; //TODO: Move actual context into the EngineProcessDocumentContext

  ezWorld* GetWorld() const { return m_pWorld.Borrow(); }

protected:
  virtual void OnInitialize();
  virtual void OnDeinitialize();

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
  virtual void OnThumbnailViewContextCreated();
  /// \brief Called before a thumbnail context is destroyed. Used for cleanup of what was done in OnThumbnailViewContextCreated()
  virtual void OnDestroyThumbnailViewContext();

  ezUniquePtr<ezWorld> m_pWorld;

  /// \brief Sets or removes the given tag on the object and optionally all children
  void SetTagOnObject(const ezUuid& object, const char* szTag, bool bSet, bool recursive);

  /// \brief Sets the given tag on the object and all children.
  void SetTagRecursive(ezGameObject* pObject, const ezTag& tag);
  /// \brief Clears the given tag on the object and all children.
  void ClearTagRecursive(ezGameObject* pObject, const ezTag& tag);

protected:
  const ezEngineProcessViewContext* GetViewContext(ezUInt32 uiView) const { return uiView >= m_ViewContexts.GetCount() ? nullptr : m_ViewContexts[uiView]; }

private:
  friend class ezEditorEngineSyncObject;

  void AddSyncObject(ezEditorEngineSyncObject* pSync);
  void RemoveSyncObject(ezEditorEngineSyncObject* pSync);
  ezEditorEngineSyncObject* FindSyncObject(const ezUuid& guid);


private:
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
    ThumbnailSuperscaleFactor = 2, ///< Thumbnail render target size is multiplied by this and then the final image is downscaled again. Needs to be power-of-two.
    ThumbnailConvergenceFramesTarget = 4 ///< Due to multi-threaded rendering, this must be at least 2
  };

  ezUInt8 m_uiThumbnailConvergenceFrames;
  ezUInt16 m_uiThumbnailWidth;
  ezUInt16 m_uiThumbnailHeight;
  ezEngineProcessViewContext* m_pThumbnailViewContext;
  ezGALRenderTargetSetup m_ThumbnailRenderTargetSetup;
  ezGALTextureHandle m_hThumbnailColorRT;
  ezGALTextureHandle m_hThumbnailDepthRT;
  bool m_bWorldSimStateBeforeThumbnail;
};

