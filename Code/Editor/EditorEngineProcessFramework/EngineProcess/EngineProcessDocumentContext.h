#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>
#include <Foundation/Types/Uuid.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/Readback.h>

class ezEditorEngineSyncObjectMsg;
class ezEditorEngineSyncObject;
class ezEditorEngineDocumentMsg;
class ezEngineProcessViewContext;
class ezEngineProcessCommunicationChannel;
class ezProcessMessage;
class ezExportDocumentMsgToEngine;
class ezCreateThumbnailMsgToEngine;
struct ezResourceEvent;

struct ezEngineProcessDocumentContextFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None = 0,
    CreateWorld = EZ_BIT(0),
    Default = None
  };

  struct Bits
  {
    StorageType CreateWorld : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezEngineProcessDocumentContextFlags);

/// \brief A document context is the counter part to an editor document on the engine side.
///
/// For every document in the editor that requires engine output (rendering, picking, etc.), there is a ezEngineProcessDocumentContext
/// created in the engine process.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineProcessDocumentContext : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineProcessDocumentContext, ezReflectedClass);

public:
  ezEngineProcessDocumentContext(ezBitflags<ezEngineProcessDocumentContextFlags> flags);
  virtual ~ezEngineProcessDocumentContext();

  virtual void Initialize(const ezUuid& documentGuid, const ezVariant& metaData, ezEngineProcessCommunicationChannel* pIPC, ezStringView sDocumentType);
  void Deinitialize();

  /// \brief Returns the document type for which this context was created. Useful in case a context may be used for multiple document types.
  ezStringView GetDocumentType() const { return m_sDocumentType; }

  void SendProcessMessage(ezProcessMessage* pMsg = nullptr);
  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg);

  static ezEngineProcessDocumentContext* GetDocumentContext(ezUuid guid);
  static void AddDocumentContext(ezUuid guid, const ezVariant& metaData, ezEngineProcessDocumentContext* pView, ezEngineProcessCommunicationChannel* pIPC, ezStringView sDocumentType);
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
  ezWorldRttiConverterContext m_Context; // TODO: Move actual context into the EngineProcessDocumentContext
  virtual ezWorldRttiConverterContext& GetContext() { return m_Context; }
  virtual const ezWorldRttiConverterContext& GetContext() const { return m_Context; }

  ezWorld* GetWorld() const { return m_pWorld; }

  /// \brief Tries to resolve a 'reference' (given in pData) to an ezGameObject.
  virtual ezGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, ezComponentHandle hThis, ezStringView sProperty) const;

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
  virtual ezStatus ExportDocument(const ezExportDocumentMsgToEngine* pMsg);
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

  /// \brief Called before a thumbnail context is created.
  virtual void OnThumbnailViewContextRequested() {}
  /// \brief Called after a thumbnail context was created. Allows to insert code before the thumbnail is generated.
  virtual void OnThumbnailViewContextCreated();
  /// \brief Called before a thumbnail context is destroyed. Used for cleanup of what was done in OnThumbnailViewContextCreated()
  virtual void OnDestroyThumbnailViewContext();

  ezWorld* m_pWorld = nullptr;

  /// \brief Sets or removes the given tag on the object and optionally all children
  void SetTagOnObject(const ezUuid& object, const char* szTag, bool bSet, bool recursive);

  /// \brief Sets the given tag on the object and all children.
  void SetTagRecursive(ezGameObject* pObject, const ezTag& tag);
  /// \brief Clears the given tag on the object and all children.
  void ClearTagRecursive(ezGameObject* pObject, const ezTag& tag);

protected:
  const ezEngineProcessViewContext* GetViewContext(ezUInt32 uiView) const
  {
    return uiView >= m_ViewContexts.GetCount() ? nullptr : m_ViewContexts[uiView];
  }

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

protected:
  ezBitflags<ezEngineProcessDocumentContextFlags> m_Flags;
  ezUuid m_DocumentGuid;
  ezVariant m_MetaData;

  ezEngineProcessCommunicationChannel* m_pIPC = nullptr;
  ezHybridArray<ezEngineProcessViewContext*, 4> m_ViewContexts;

  ezMap<ezUuid, ezEditorEngineSyncObject*> m_SyncObjects;

private:
  enum Constants
  {
    ThumbnailSuperscaleFactor =
      2,                                 ///< Thumbnail render target size is multiplied by this and then the final image is downscaled again. Needs to be power-of-two.
    ThumbnailConvergenceFramesTarget = 4 ///< Due to multi-threaded rendering, this must be at least 4
  };

  ezUInt8 m_uiThumbnailConvergenceFrames = 0;
  ezUInt16 m_uiThumbnailWidth = 0;
  ezUInt16 m_uiThumbnailHeight = 0;
  ezEngineProcessViewContext* m_pThumbnailViewContext = nullptr;
  ezGALRenderTargets m_ThumbnailRenderTargets;
  ezGALTextureHandle m_hThumbnailColorRT;
  ezGALTextureHandle m_hThumbnailDepthRT;
  ezGALReadbackTexture m_ThumbnailReadback;

  bool m_bWorldSimStateBeforeThumbnail = false;
  ezString m_sDocumentType;

  //////////////////////////////////////////////////////////////////////////
  // GameObject reference resolution
private:
  struct GoReferenceTo
  {
    ezStringView m_sComponentProperty;
    ezUuid m_ReferenceToGameObject;
  };

  struct GoReferencedBy
  {
    ezStringView m_sComponentProperty;
    ezUuid m_ReferencedByComponent;
  };

  // Components reference GameObjects
  mutable ezMap<ezUuid, ezHybridArray<GoReferenceTo, 4>> m_GoRef_ReferencesTo;

  // GameObjects referenced by Components
  mutable ezMap<ezUuid, ezHybridArray<GoReferencedBy, 4>> m_GoRef_ReferencedBy;

  void WorldRttiConverterContextEventHandler(const ezWorldRttiConverterContext::Event& e);
};
