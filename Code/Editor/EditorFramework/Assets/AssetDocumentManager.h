#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct ezSubAsset;
class ezPlatformProfile;

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentManager : public ezDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentManager, ezDocumentManager);

public:
  ezAssetDocumentManager();
  ~ezAssetDocumentManager();

  /// \brief Opens the asset file and reads the "Header" into the given ezAssetDocumentInfo.
  virtual ezStatus ReadAssetDocumentInfo(ezUniquePtr<ezAssetDocumentInfo>& out_pInfo, ezStreamReader& inout_stream) const;
  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezHybridArray<ezSubAssetData, 4>& out_subAssets) const {}

  /// If this asset type has additional output files that need to be generated (like a texture atlas that combines outputs from multiple assets)
  /// this function should make sure those files are all generated and return the list of relative file paths (from the data directory root).
  virtual ezStatus GetAdditionalOutputs(ezDynamicArray<ezString>& ref_files) { return ezStatus(EZ_SUCCESS); }

  // ezDocumentManager overrides:
public:
  virtual ezStatus CloneDocument(const char* szPath, const char* szClonePath, ezUuid& inout_cloneGuid) override;

  /// \name Asset Profile Functions
  ///@{
public:
  /// \brief Called by the ezAssetCurator when the active asset profile changes to re-compute m_uiAssetProfileHash.
  void ComputeAssetProfileHash(const ezPlatformProfile* pAssetProfile);

  /// \brief Returns the hash that was previously computed through ComputeAssetProfileHash().
  EZ_ALWAYS_INLINE ezUInt64 GetAssetProfileHash() const { return m_uiAssetProfileHash; }

  /// \brief Returns pAssetProfile, or if that is null, ezAssetCurator::GetSingleton()->GetActiveAssetProfile().
  static const ezPlatformProfile* DetermineFinalTargetProfile(const ezPlatformProfile* pAssetProfile);

private:
  virtual ezUInt64 ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const;

  // The hash that is combined with the asset document hash to determine whether the document output is up to date.
  // This hash needs to be computed in ComputeAssetProfileHash() and should reflect all important settings from the givne asset profile that
  // affect the asset output for this manager.
  // However, if GeneratesProfileSpecificAssets() return false, the hash must be zero, as then all outputs must be identical in all
  // profiles.
  ezUInt64 m_uiAssetProfileHash = 0;

  ///@}
  /// \name Thumbnail Functions
  ///@{
public:
  /// \brief Returns the absolute path to the thumbnail that belongs to the given document.
  static ezString GenerateResourceThumbnailPath(const char* szDocumentPath);
  static bool IsThumbnailUpToDate(const char* szDocumentPath, ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion);

  ///@}
  /// \name Output Functions
  ///@{

  virtual void AddEntriesToAssetTable(
    const char* szDataDirectory, const ezPlatformProfile* pAssetProfile, ezDelegate<void(ezStringView sGuid, ezStringView sPath, ezStringView sType)> addEntry) const;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezPlatformProfile* pAssetProfile) const;

  /// \brief Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  ezString GetAbsoluteOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, const char* szDocumentPath, const char* szOutputTag,
    const ezPlatformProfile* pAssetProfile = nullptr) const;

  /// \brief Relative to 'AssetCache' folder.
  virtual ezString GetRelativeOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, const char* szDataDirectory, const char* szDocumentPath,
    const char* szOutputTag, const ezPlatformProfile* pAssetProfile = nullptr) const;
  virtual bool GeneratesProfileSpecificAssets() const = 0;

  bool IsOutputUpToDate(
    const char* szDocumentPath, const ezDynamicArray<ezString>& outputs, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor);
  virtual bool IsOutputUpToDate(
    const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor);

  /// Describes how likely it is that a generated file is 'corrupted', due to dependency issues and such.
  /// For example a prefab may not work correctly, if it was written with a very different C++ plugin state, but this can't be detected later.
  /// Whereas a texture always produces exactly the same output and is thus perfectly reliable.
  /// This is used to clear asset caches selectively, and keep things that are unlikely to be in a broken state.
  enum OutputReliability
  {
    Perfect,
    Good,
    Unknown,
  };

  /// \see OutputReliability
  virtual OutputReliability GetAssetTypeOutputReliability() const { return OutputReliability::Unknown; }

  ///@}


  /// \brief Called by the editor to try to open a document for the matching picking result
  virtual ezResult OpenPickedDocument(const ezDocumentObject* pPickedComponent, ezUInt32 uiPartIndex) { return EZ_FAILURE; }

  ezResult TryOpenAssetDocument(const char* szPathOrGuid);

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, ezUInt64 uiHash, ezUInt16 uiTypeVersion);
  static void GenerateOutputFilename(
    ezStringBuilder& inout_sRelativeDocumentPath, const ezPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific);
};
