#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/IPC/IPCObjectMirror.h>

class ezEditorEngineConnection;
class ezEditorEngineSyncObject;

class EZ_EDITORFRAMEWORK_DLL ezAssetDocument : public ezDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocument, ezDocument);

public:
  ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, bool bUseEngineConnection, bool bUseIPCObjectMirror);
  ~ezAssetDocument();

  /// \name Asset Functions
  ///@{

  /// \brief Returns one of the strings that ezAssetDocumentManager::QuerySupportedAssetTypes returned.
  ///
  /// This can be different for each instance of the same asset document type.
  /// E.g. one texture resource may return 'Texture 2D' and another 'Texture 3D'.
  /// Likewise completely different asset document types may use the same 'asset types'.
  ///
  /// This is mostly used for sorting and filtering in the asset browser.
  virtual const char* QueryAssetType() const = 0;

  ezStatus TransformAsset(const char* szPlatform = nullptr);

  /// \brief Will definitely try to transform the asset, ignoring whether the transform is disabled on an asset.
  ///
  /// Will also not try to save the document.
  /// If this function encounters flags that prevent transformation, it will return an error and not silently ignore them.
  ezStatus TransformAssetManually(const char* szPlatform = nullptr);

  ezStatus RetrieveAssetInfo(const char* szPlatform = nullptr);

  /// \brief Determines the path to the transformed asset file. May be overridden for special cases.
  ///
  /// The default implementation puts each asset into the AssetCache folder.
  virtual ezString GetFinalOutputFileName(const char* szPlatform = nullptr);

  /// \brief Called during certain operations, such as TransformAsset, to determine how to proceed with this asset.
  virtual ezBitflags<ezAssetDocumentFlags> GetAssetFlags() const;

  /// \brief Retruns the RTTI type version of this asset document type. E.g. when the algorithm to transform an asset changes,
  /// Increase the RTTI version. This will ensure that assets get re-transformed, even though their settings and dependencies might not have changed.
  ezUInt16 GetAssetTypeVersion() const;

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

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) = 0;

  /// \brief Override this and write the transformed file into the given stream.
  ///
  /// The stream already contains the ezAssetFileHeader. This is the function to prefer when the asset can be written
  /// directly from the editor process.
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) = 0;

  /// \brief Only override this function, if the transformed file must be written from another process.
  ///
  /// szTargetFile is where the transformed asset should be written to. The overriding function must ensure to first
  /// write \a AssetHeader to the file, to make it a valid asset file.
  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader);

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) = 0;

  /// \brief Returns the full path to the jpg file in which the thumbnail for this asset is supposed to be
  ezString GetThumbnailFilePath() const;

  /// \brief Should be called after manually changing the thumbnail, such that the system will reload it
  void InvalidateAssetThumbnail();

  /// \brief Saves the given image as the new thumbnail for the asset
  void SaveThumbnail(const ezImage& img);

  virtual ezString GetDocumentPathFromGuid(const ezUuid& documentGuid) const override;

  ///@}

private:
  virtual ezDocumentInfo* CreateDocumentInfo() override;

  static ezString DetermineFinalTargetPlatform(const char* szPlatform);

private:
  EngineStatus m_EngineStatus;
  bool m_bUseIPCObjectMirror;
  bool m_bUseEngineConnection;

  ezIPCObjectMirror m_Mirror;
  ezEditorEngineConnection* m_pEngineConnection;

  mutable ezHashTable<ezUuid, ezEditorEngineSyncObject*> m_AllSyncObjects;
  mutable ezDeque<ezEditorEngineSyncObject*> m_SyncObjects;

  mutable ezHybridArray<ezUuid, 32> m_DeletedObjects;
};

