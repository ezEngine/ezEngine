#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezEditorEngineConnection;
class ezEditorEngineSyncObject;
class ezAssetDocumentManager;
class ezPlatformProfile;
class QImage;

/// \brief Describes whether the asset document on the editor side also needs a rendering context on the engine side
enum class ezAssetDocEngineConnection : ezUInt8
{
  None,               ///< Use this when the document is fully self-contained and any UI is handled by Qt only. This is very common for 'data only' assets and everything that can't be visualized in 3D.
  Simple,             ///< Use this when the asset should be visualized in 3D. This requires a 'context' to be set up on the engine side that implements custom rendering. This is the most common type for anything that can be visualized in 3D, though can also be used for 2D data.
  FullObjectMirroring ///< In this mode the entire object hierarchy on the editor side is automatically synchronized over to an engine context. This is only needed for complex documents, such as scenes and prefabs.
};

/// \brief Frequently needed asset document states, to prevent code duplication
struct ezCommonAssetUiState
{
  enum Enum : ezUInt32
  {
    Pause = EZ_BIT(0),
    Restart = EZ_BIT(1),
    Loop = EZ_BIT(2),
    SimulationSpeed = EZ_BIT(3),
    Grid = EZ_BIT(4),
    Visualizers = EZ_BIT(5),
  };

  Enum m_State;
  double m_fValue = 0;
};

class EZ_EDITORFRAMEWORK_DLL ezAssetDocument : public ezDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocument, ezDocument);

public:
  /// \brief The thumbnail info containing the hash of the file is appended to assets.
  /// The serialized size of this class can't change since it is found by seeking to the end of the file.
  class EZ_EDITORFRAMEWORK_DLL ThumbnailInfo
  {
  public:
    ezResult Deserialize(ezStreamReader& inout_reader);
    ezResult Serialize(ezStreamWriter& inout_writer) const;

    /// \brief Checks whether the stored file contains the same hash.
    bool IsThumbnailUpToDate(ezUInt64 uiExpectedHash, ezUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

    /// \brief Sets the asset file hash
    void SetFileHashAndVersion(ezUInt64 uiHash, ezUInt16 v)
    {
      m_uiHash = uiHash;
      m_uiVersion = v;
    }

    /// \brief Returns the serialized size of the thumbnail info.
    /// Used to seek to the end of the file and find the thumbnail info struct.
    constexpr ezUInt32 GetSerializedSize() const { return 19; }

  private:
    ezUInt64 m_uiHash = 0;
    ezUInt16 m_uiVersion = 0;
    ezUInt16 m_uiReserved = 0;
  };

  ezAssetDocument(ezStringView sDocumentPath, ezDocumentObjectManager* pObjectManager, ezAssetDocEngineConnection engineConnectionType);
  ~ezAssetDocument();

  /// \name Asset Functions
  ///@{

  ezAssetDocumentManager* GetAssetDocumentManager() const;
  const ezAssetDocumentInfo* GetAssetDocumentInfo() const;

  ezBitflags<ezAssetDocumentFlags> GetAssetFlags() const;

  const ezAssetDocumentTypeDescriptor* GetAssetDocumentTypeDescriptor() const
  {
    return static_cast<const ezAssetDocumentTypeDescriptor*>(GetDocumentTypeDescriptor());
  }

  /// \brief Transforms an asset.
  ///   Typically not called manually but by the curator which takes care of dependencies first.
  ///
  /// If ezTransformFlags::ForceTransform is set, it will try to transform the asset, ignoring whether the transform is up to date.
  /// If ezTransformFlags::TriggeredManually is set, transform produced changes will be saved back to the document.
  /// If ezTransformFlags::BackgroundProcessing is set and transforming the asset would require re-saving it, nothing is done.
  ezTransformStatus TransformAsset(ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile = nullptr);

  /// \brief Updates the thumbnail of the asset.
  ///   Should never be called manually. Called only by the curator which takes care of dependencies first.
  ezTransformStatus CreateThumbnail();

  /// \brief Returns the RTTI type version of this asset document type. E.g. when the algorithm to transform an asset changes,
  /// Increase the RTTI version. This will ensure that assets get re-transformed, even though their settings and dependencies might not have changed.
  ezUInt16 GetAssetTypeVersion() const;

  ///@}
  /// \name IPC Functions
  ///@{

  enum class EngineStatus
  {
    Unsupported,  ///< This document does not have engine IPC.
    Disconnected, ///< Engine process crashed or not started yet.
    Initializing, ///< Document is being initialized on the engine process side.
    Loaded,       ///< Any message sent after this state is reached will work on a fully loaded document.
  };

  /// \brief Returns the current state of the engine process side of this document.
  EngineStatus GetEngineStatus() const { return m_EngineStatus; }
  /// \brief Waits for GetEngineStatus to return Loaded or returns a failure reason.
  ezStatus WaitForEngineStatusLoaded() const;

  /// \brief Passed into ezEngineProcessDocumentContext::Initialize on the engine process side. Allows the document to provide additional data to the engine process during context creation.
  virtual ezVariant GetCreateEngineMetaData() const { return ezVariant(); }

  /// \brief Sends a message to the corresponding ezEngineProcessDocumentContext on the engine process.
  bool SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage) const;

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

  /// \brief Returns the first sync object registered with the given type.
  ezEditorEngineSyncObject* FindSyncObject(const ezRTTI* pType) const;

  /// \brief Sends messages to sync all sync objects to the engine process side.
  void SyncObjectsToEngine() const;

  /// /brief Sends a message that the document has been opened or closed. Resends all document data.
  ///
  /// Calling this will always clear the existing document on the engine side and reset the state to the editor state.
  void SendDocumentOpenMessage(bool bOpen);


  ///@}

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
  virtual ezTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override;

  /// \brief Implements auto transform on save
  virtual void InternalAfterSaveDocument() override;

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  ///@}
  /// \name Asset Functions
  ///@{

  /// \brief Override this to add custom data (e.g. additional file dependencies) to the info struct.
  ///
  /// \note ALWAYS call the base function! It automatically fills out references that it can determine.
  ///       In most cases that is already sufficient.
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const;

  /// \brief Override this and write the transformed file for the given szOutputTag into the given stream.
  ///
  /// The stream already contains the ezAssetFileHeader. This is the function to prefer when the asset can be written
  /// directly from the editor process. AssetHeader is already written to the stream, but provided as reference.
  ///
  /// \param stream Data stream to write the asset to.
  /// \param szOutputTag Either empty for the default output or matches one of the tags defined in ezAssetDocumentInfo::m_Outputs.
  /// \param szPlatform Platform for which is the output is to be created. Default is 'Default'.
  /// \param AssetHeader Header already written to the stream, provided for reference.
  /// \param transformFlags flags that affect the transform process, see ezTransformFlags.
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) = 0;

  /// \brief Only override this function, if the transformed file for the given szOutputTag must be written from another process.
  ///
  /// szTargetFile is where the transformed asset should be written to. The overriding function must ensure to first
  /// write \a AssetHeader to the file, to make it a valid asset file or provide a custom ezAssetDocumentManager::IsOutputUpToDate function.
  /// See ezTransformFlags for definition of transform flags.
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags);

  ezStatus RemoteExport(const ezAssetFileHeader& header, const char* szOutputTarget) const;

  ///@}
  /// \name Thumbnail Functions
  ///@{

  /// \brief Override this function to generate a thumbnail. Only called if GetAssetFlags returns ezAssetDocumentFlags::SupportsThumbnail.
  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo);

  /// \brief Returns the full path to the jpg file in which the thumbnail for this asset is supposed to be
  ezString GetThumbnailFilePath(ezStringView sSubAssetName = ezStringView()) const;

  /// \brief Should be called after manually changing the thumbnail, such that the system will reload it
  void InvalidateAssetThumbnail(ezStringView sSubAssetName = ezStringView()) const;

  /// \brief Requests the engine side to render a thumbnail, will call SaveThumbnail on success.
  ezStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, ezArrayPtr<ezStringView> viewExclusionTags /*= ezStringView("SkyLight")*/) const;
  ezStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo) const
  {
    ezStringView defVal("SkyLight");
    return RemoteCreateThumbnail(thumbnailInfo, {&defVal, 1});
  }

  /// \brief Saves the given image as the new thumbnail for the asset
  ezStatus SaveThumbnail(const ezImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// \brief Saves the given image as the new thumbnail for the asset
  ezStatus SaveThumbnail(const QImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// \brief Appends an asset header containing the thumbnail hash to the file. Each thumbnail is appended by it to check up-to-date state.
  void AppendThumbnailInfo(ezStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const;

  ///@}
  /// \name Common Asset States
  ///@{

public:
  /// \brief Override this to handle a change to a common asset state differently.
  ///
  /// By default an on-off flag for every state is tracked, but nothing else.
  /// Also this automatically broadcasts the m_CommonAssetUiChangeEvent event.
  virtual void SetCommonAssetUiState(ezCommonAssetUiState::Enum state, double value);

  /// \brief Override this to return custom values for a common asset state.
  virtual double GetCommonAssetUiState(ezCommonAssetUiState::Enum state) const;

  /// \brief Used to broadcast state change events for common asset states.
  ezEvent<const ezCommonAssetUiState&> m_CommonAssetUiChangeEvent;

protected:
  ezUInt32 m_uiCommonAssetStateFlags = 0;

  ///@}

protected:
  /// \brief Adds all prefab dependencies to the ezAssetDocumentInfo object. Called automatically by UpdateAssetDocumentInfo()
  void AddPrefabDependencies(const ezDocumentObject* pObject, ezAssetDocumentInfo* pInfo) const;

  /// \brief Crawls through all asset properties of pObject and adds all string properties that have a ezAssetBrowserAttribute as a dependency to
  /// pInfo. Automatically called by UpdateAssetDocumentInfo()
  void AddReferences(const ezDocumentObject* pObject, ezAssetDocumentInfo* pInfo, bool bInsidePrefab) const;

protected:
  ezUniquePtr<ezIPCObjectMirrorEditor> m_pMirror;

  virtual ezDocumentInfo* CreateDocumentInfo() override;

  ezTransformStatus DoTransformAsset(const ezPlatformProfile* pAssetProfile, ezBitflags<ezTransformFlags> transformFlags);

  EngineStatus m_EngineStatus;
  ezAssetDocEngineConnection m_EngineConnectionType = ezAssetDocEngineConnection::None;

  ezEditorEngineConnection* m_pEngineConnection;

  mutable ezHashTable<ezUuid, ezEditorEngineSyncObject*> m_AllSyncObjects;
  mutable ezDeque<ezEditorEngineSyncObject*> m_SyncObjects;

  mutable ezHybridArray<ezUuid, 32> m_DeletedObjects;
};
