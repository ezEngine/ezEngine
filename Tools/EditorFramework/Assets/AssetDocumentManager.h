#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/Types/Status.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>

struct ezSubAsset;

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentManager : public ezDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentManager, ezDocumentManager);

public:
  ezAssetDocumentManager() {};
  ~ezAssetDocumentManager() {};

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const;
  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const = 0;
  /// \brief Opens the asset file and reads the "Header" into the given ezAssetDocumentInfo.
  virtual ezStatus ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReader& stream) const;
  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezHybridArray<ezSubAssetData, 4>& out_SubAssets) const { }

  /// \name Thumbnail Functions
  ///@{

  /// \brief Returns the absolute path to the thumbnail that belongs to the given document.
  static ezString GenerateResourceThumbnailPath(const char* szDocumentPath);
  static bool IsThumbnailUpToDate(const char* szDocumentPath, ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion);

  ///@}
  /// \name Output Functions
  ///@{

  virtual void AddEntriesToAssetTable(const char* szDataDirectory, const char* szPlatform, ezStreamWriter& file) const;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const char* szPlatform) const;

  /// \brief Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  ezString GetAbsoluteOutputFileName(const char* szDocumentPath, const char* szOutputTag, const char* szPlatform = nullptr) const;

  /// \brief Relative to 'AssetCache' folder.
  virtual ezString GetRelativeOutputFileName(const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const char* szPlatform = nullptr) const;
  virtual ezString GetResourceTypeExtension() const = 0;
  virtual bool GeneratesPlatformSpecificAssets() const = 0;

  bool IsOutputUpToDate(const char* szDocumentPath, const ezSet<ezString>& outputs, ezUInt64 uiHash, ezUInt16 uiTypeVersion);
  virtual bool IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, ezUInt16 uiTypeVersion);

  ///@}

  static ezString DetermineFinalTargetPlatform(const char* szPlatform);

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, ezUInt64 uiHash, ezUInt16 uiTypeVersion);
  static void GenerateOutputFilename(ezStringBuilder& inout_sRelativeDocumentPath, const char* szPlatform, const char* szExtension, bool bPlatformSpecific);
};
