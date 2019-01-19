#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/Plugin.h>
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

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const;
  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const = 0;
  /// \brief Opens the asset file and reads the "Header" into the given ezAssetDocumentInfo.
  virtual ezStatus ReadAssetDocumentInfo(ezUniquePtr<ezAssetDocumentInfo>& out_pInfo, ezStreamReader& stream) const;
  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezHybridArray<ezSubAssetData, 4>& out_SubAssets) const {}

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

  virtual void AddEntriesToAssetTable(const char* szDataDirectory, const ezPlatformProfile* pAssetProfile,
                                      ezMap<ezString, ezString>& inout_GuidToPath) const;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezPlatformProfile* pAssetProfile) const;

  /// \brief Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  ezString GetAbsoluteOutputFileName(const char* szDocumentPath, const char* szOutputTag,
                                     const ezPlatformProfile* pAssetProfile = nullptr) const;

  /// \brief Relative to 'AssetCache' folder.
  virtual ezString GetRelativeOutputFileName(const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag,
                                             const ezPlatformProfile* pAssetProfile = nullptr) const;
  virtual ezString GetResourceTypeExtension(const char* szDocumentPath) const = 0;
  virtual bool GeneratesProfileSpecificAssets() const = 0;

  bool IsOutputUpToDate(const char* szDocumentPath, const ezSet<ezString>& outputs, ezUInt64 uiHash, ezUInt16 uiTypeVersion);
  virtual bool IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, ezUInt16 uiTypeVersion);

  ///@}


  /// \brief Called by the editor to try to open a document for the matching picking result
  virtual ezResult OpenPickedDocument(const ezDocumentObject* pPickedComponent, ezUInt32 uiPartIndex) { return EZ_FAILURE; }

  ezResult TryOpenAssetDocument(const char* szPathOrGuid);

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, ezUInt64 uiHash, ezUInt16 uiTypeVersion);
  static void GenerateOutputFilename(ezStringBuilder& inout_sRelativeDocumentPath, const ezPlatformProfile* pAssetProfile,
                                     const char* szExtension, bool bPlatformSpecific);
};
