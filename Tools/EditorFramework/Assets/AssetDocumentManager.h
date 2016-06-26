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

  virtual ezString GetResourceTypeExtension() const = 0;

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const = 0;

  virtual bool GeneratesPlatformSpecificAssets() const = 0;

  static bool IsResourceUpToDate(ezUInt64 uiHash, ezUInt16 uiTypeVersion, const char* szResourceFile);
  static bool IsThumbnailUpToDate(ezUInt64 uiThumbnailHash, ezUInt32 uiTypeVersion, const char* szDocumentPath);

  ezString GenerateResourceFileName(const char* szDocumentPath, const char* szPlatform) const;
  static ezString GenerateResourceThumbnailPath(const char* szDocumentPath);

  /// \brief Determines the path to the transformed asset file. May be overridden for special cases.
  ///
  /// The default implementation puts each asset into the AssetCache folder.
  virtual ezString GetFinalOutputFileName(const ezDocumentTypeDescriptor* pDescriptor, const char* szDocumentPath, const char* szPlatform) const;

  ezString GenerateRelativeResourceFileName(const char* szDataDirectory, const char* szDocumentPath, const char* szPlatform) const;
  
  static ezString DetermineFinalTargetPlatform(const char* szPlatform);
};
