#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/IPC/IPCObjectMirror.h>

class ezEditorEngineConnection;
class ezEditorEngineSyncObject;
class ezAssetDocumentManager;

class EZ_EDITORFRAMEWORK_DLL ezAssetDocument : public ezDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocument, ezDocument);

public:
  ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, bool bUseEngineConnection, bool bUseIPCObjectMirror);
  ~ezAssetDocument();

  /// \name Asset Functions
  ///@{

  ezAssetDocumentManager* GetAssetDocumentManager() const;

  ezBitflags<ezAssetDocumentFlags> GetAssetFlags() const;

  /// \brief Returns one of the strings that ezAssetDocumentManager::QuerySupportedAssetTypes returned.
  ///
  /// This can be different for each instance of the same asset document type.
  /// E.g. one texture resource may return 'Texture 2D' and another 'Texture 3D'.
  /// Likewise completely different asset document types may use the same 'asset types'.
  ///
  /// This is mostly used for sorting and filtering in the asset browser.
  virtual const char* QueryAssetType() const = 0;

  /// \brief Transforms an asset.
  ///   Should never be called manually. Called only by the curator which takes care of dependencies first.
  ezStatus TransformAsset(const char* szPlatform = nullptr);

  /// \brief Updates the thumbnail of the asset.
  ///   Should never be called manually. Called only by the curator which takes care of dependencies first.
  ezStatus CreateThumbnail();

  /// \brief Will definitely try to transform the asset, ignoring whether the transform is disabled on an asset.
  ///
  /// Will also not try to save the document.
  /// If this function encounters flags that prevent transformation, it will return an error and not silently ignore them.
  ezStatus TransformAssetManually(const char* szPlatform = nullptr);

  ezStatus RetrieveAssetInfo(const char* szPlatform = nullptr);

  /// \brief Returns the RTTI type version of this asset document type. E.g. when the algorithm to transform an asset changes,
  /// Increase the RTTI version. This will ensure that assets get re-transformed, even though their settings and dependencies might not have changed.
  ezUInt16 GetAssetTypeVersion() const;

  ezString GetFinalOutputFileName(const char* szPlatform = nullptr) const;

  ///@}
  /// \name IPC Functions
  ///@{
  
  enum class EngineStatus
  {
    Unsupported, ///< This document does not have engine IPC.
    Disconnected, ///< Engine process crashed or not started yet.
    Initializing, ///< Document is being initialized on the engine process side.
    Loaded, ///< Any message sent after this state is reached will work on a fully loaded document.
  };

  /// \brief Returns the current state of the engine process side of this document.
  EngineStatus GetEngineStatus() const { return m_EngineStatus; }

  /// \brief Sends a message to the corresponding ezEngineProcessDocumentContext on the engine process.
  void SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage) const;

  /// \brief Handles all messages received from the corresponding ezEngineProcessDocumentContext on the engine process.
  virtual void HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg);

  /// \brief Returns the ezEditorEngineConnection for this document.
  ezEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineConnection; }

  /// \brief Registers a sync object for this document. It will be mirrored to the ezEngineProcessDocumentContext on the engine process.
  void AddSyncObject(ezEditorEngineSyncObject* pSync) const;

  /// \brief Removes a previously registered sync object. It will be removed on the engine process side.
  void RemoveSyncObject(ezEditorEngineSyncObject* pSync) const;

  /// \brief Returns the sync object registered under the given guid.
  ezEditorEngineSyncObject* FindSyncObject(const ezUuid& guid) const;

  /// \brief Sends messages to sync all sync objects to the engine process side.
  void SyncObjectsToEngine();

  ///@}

  struct AssetEvent
  {
    enum class Type
    {
      AssetInfoChanged,
    };

    Type m_Type;
  };

  ezEvent<const AssetEvent&> m_AssetEvents;

  ezEvent<const ezEditorEngineDocumentMsg*> m_ProcessMessageEvent;

protected:
  void EngineConnectionEventHandler(const ezEditorEngineProcessConnection::Event& e);

  /// \name Hash Functions
  ///@{

  /// \brief Computes the hash from all document objects
  ezUInt64 GetDocumentHash() const;

  /// \brief Computes the hash for one document object and combines it with the given hash
  void GetChildHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const;

  /// \brief Computes the hash for transform relevant meta data of the given document object and combines it with the given hash.
  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const {}

  ///@}
  /// \name Reimplemented Base Functions
  ///@{

  /// \brief Overrides the base function to call UpdateAssetDocumentInfo() to update the settings hash
  virtual ezStatus InternalSaveDocument() override;

  /// \brief Implements auto transform on save
  virtual void InternalAfterSaveDocument() override;

  virtual void InitializeAfterLoading() override;

  ///@}
  /// \name Asset Functions
  ///@{

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const;

  /// \brief Override this and write the transformed file into the given stream.
  ///
  /// The stream already contains the ezAssetFileHeader. This is the function to prefer when the asset can be written
  /// directly from the editor process. AssetHeader is already written to the stream, but provided as reference.
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) = 0;

  /// \brief Only override this function, if the transformed file must be written from another process.
  ///
  /// szTargetFile is where the transformed asset should be written to. The overriding function must ensure to first
  /// write \a AssetHeader to the file, to make it a valid asset file.
  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader);

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) = 0;

  virtual ezString GetDocumentPathFromGuid(const ezUuid& documentGuid) const override;

  ezStatus RemoteExport(const ezAssetFileHeader& header, const char* szOutputTarget) const;

  ///@}
  /// \name Thumbnail Functions
  ///@{

  /// \brief Override this function to generate a thumbnail. Only called if GetAssetFlags returns ezAssetDocumentFlags::SupportsThumbnail.
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader);
  /// \brief Returns the full path to the jpg file in which the thumbnail for this asset is supposed to be
  ezString GetThumbnailFilePath() const;
  /// \brief Should be called after manually changing the thumbnail, such that the system will reload it
  void InvalidateAssetThumbnail() const;
  /// \brief Requests the engine side to render a thumbnail, will call SaveThumbnail on success.
  ezStatus RemoteCreateThumbnail(const ezAssetFileHeader& header) const;
  /// \brief Saves the given image as the new thumbnail for the asset
  ezStatus SaveThumbnail(const ezImage& img, const ezAssetFileHeader& header) const;
  /// \brief Appends an asset header containing the thumbnail hash to the file. Each thumbnail is appended by it to check up-to-date state.
  void AppendThumbnailInfo(const char* szThumbnailFile, const ezAssetFileHeader& header) const;

  ///@}

private:
  virtual ezDocumentInfo* CreateDocumentInfo() override;

private:
  void AddPrefabDependencies(const ezDocumentObject* pObject, ezAssetDocumentInfo* pInfo) const;
  void AddReferences(const ezDocumentObject* pObject, ezAssetDocumentInfo* pInfo, bool bInsidePrefab) const;

  EngineStatus m_EngineStatus;
  bool m_bUseIPCObjectMirror;
  bool m_bUseEngineConnection;

  ezIPCObjectMirror m_Mirror;
  ezEditorEngineConnection* m_pEngineConnection;

  mutable ezHashTable<ezUuid, ezEditorEngineSyncObject*> m_AllSyncObjects;
  mutable ezDeque<ezEditorEngineSyncObject*> m_SyncObjects;

  mutable ezHybridArray<ezUuid, 32> m_DeletedObjects;
};

