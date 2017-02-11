#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>
#include <EditorFramework/Assets/Declarations.h>

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentManager : public ezDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentManager, ezDocumentManager);

public:
  ezAssetDocumentManager() {};
  ~ezAssetDocumentManager() {};

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const;
  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const = 0;

  /// \name Thumbnail Functions
  ///@{

  /// \brief Returns the absolute path to the thumbnail that belongs to the given document.
  static ezString GenerateResourceThumbnailPath(const char* szDocumentPath);
  static bool IsThumbnailUpToDate(const char* szDocumentPath, ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion);
 
  ///@}
  /// \name Output Functions
  ///@{

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
