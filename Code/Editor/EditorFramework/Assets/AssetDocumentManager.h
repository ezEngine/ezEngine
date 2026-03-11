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
  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezDynamicArray<ezSubAssetData>& out_subAssets) const {}

  /// If this asset type has additional output files that need to be generated (like a texture atlas that combines outputs from multiple assets)
  /// this function should make sure those files are all generated and return the list of relative file paths (from the data directory root).
  virtual ezStatus GetAdditionalOutputs(ezDynamicArray<ezString>& ref_files) { return ezStatus(EZ_SUCCESS); }

  // ezDocumentManager overrides:
public:
  virtual ezStatus CloneDocument(ezStringView sPath, ezStringView sClonePath, ezUuid& inout_cloneGuid) override;

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
  virtual ezString GenerateResourceThumbnailPath(ezStringView sDocumentPath, ezStringView sSubAssetName = ezStringView());
  virtual bool IsThumbnailUpToDate(ezStringView sDocumentPath, ezStringView sSubAssetName, ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion);

  ///@}
  /// \name Output Functions
  ///@{

  virtual void AddEntriesToAssetTable(ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile, ezDelegate<void(ezStringView sGuid, ezStringView sPath, ezStringView sType)> addEntry) const;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const;

  /// \brief Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  ezString GetAbsoluteOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, ezStringView sDocumentPath, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile = nullptr) const;

  /// \brief Relative to 'AssetCache' folder.
  virtual ezString GetRelativeOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDesc, ezStringView sDataDirectory, ezStringView sDocumentPath, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile = nullptr) const;

  /// \brief Should return the document type of the given sOutputTag.
  virtual ezStringView GetOutputDocumentType(const ezAssetDocumentTypeDescriptor* pTypeDesc, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile = nullptr) const { return pTypeDesc->m_sDocumentTypeName; }

  virtual bool GeneratesProfileSpecificAssets() const = 0;

  bool IsOutputUpToDate(ezStringView sDocumentPath, const ezDynamicArray<ezString>& outputs, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor);
  virtual bool IsOutputUpToDate(ezStringView sDocumentPath, ezStringView sOutputTag, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor);

  /// Describes how likely it is that a generated file is 'corrupted', due to dependency issues and such.
  /// For example a prefab may not work correctly, if it was written with a very different C++ plugin state, but this can't be detected later.
  /// Whereas a texture always produces exactly the same output and is thus perfectly reliable.
  /// This is used to clear asset caches selectively, and keep things that are unlikely to be in a broken state.
  enum OutputReliability : ezUInt8
  {
    Unknown = 0,
    Good = 1,
    Perfect = 2,
  };

  /// \see OutputReliability
  virtual OutputReliability GetAssetTypeOutputReliability() const { return OutputReliability::Unknown; }

  ///@}


  /// \brief Called by the editor to try to open a document for the matching picking result
  virtual ezResult OpenPickedDocument(const ezDocumentObject* pPickedComponent, ezUInt32 uiPartIndex) { return EZ_FAILURE; }

  ezResult TryOpenAssetDocument(const char* szPathOrGuid);

  /// In case this manager deals with types that need to be force transformed on scene export, it can add the asset type names to this list.
  /// This is only needed for assets that have such special dependencies for their transform step, that the regular dependency tracking doesn't work for them.
  /// Currently the only known case are Collection assets, because they have to manually go through the Package dependencies transitively, which means
  /// that the asset curator can't know when they need to be updated.
  virtual void GetAssetTypesRequiringTransformForSceneExport(ezSet<ezTempHashedString>& inout_assetTypes) {};

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, ezUInt64 uiHash, ezUInt16 uiTypeVersion);
  static void GenerateOutputFilename(ezStringBuilder& inout_sRelativeDocumentPath, const ezPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific);
};
