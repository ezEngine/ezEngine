#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentInfo final : public ezDocumentInfo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentInfo, ezDocumentInfo);

public:
  ezAssetDocumentInfo();
  virtual ~ezAssetDocumentInfo();
  ezAssetDocumentInfo(ezAssetDocumentInfo&& rhs);
  void operator=(ezAssetDocumentInfo&& rhs);
  /// \brief Creates a clone without meta data.
  void CreateShallowClone(ezAssetDocumentInfo& out_docInfo) const;
  void ClearMetaData();

  ezUInt64 m_uiSettingsHash;                    ///< Current hash over all settings in the document, used to check resulting resource for being up-to-date in combination with dependency hashes.

  ezSet<ezString> m_TransformDependencies;      ///< [Data dir relative path or GUID] Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  ezSet<ezString> m_ThumbnailDependencies;      ///< [Data dir relative path or GUID] Files that are used to generate the thumbnail.
  ezSet<ezString> m_PackageDependencies;        ///< [Data dir relative path or GUID] Files that are needed at runtime and should be packaged with the game.

  ezSet<ezString> m_Outputs;                    ///< Additional output this asset produces besides the default one. These are tags like VISUAL_SHADER that are resolved
                                                ///< by the ezAssetDocumentManager into paths.
  ezHashedString m_sAssetsDocumentTypeName;
  ezString m_sAssetsDocumentTags;
  ezDynamicArray<ezReflectedClass*> m_MetaInfo; ///< Holds arbitrary objects that store meta-data for the asset document. Mainly used for exposed parameters, but can be any reflected
                                                ///< type. This array takes ownership of all objects and deallocates them on shutdown.

  const char* GetAssetsDocumentTypeName() const;
  void SetAssetsDocumentTypeName(const char* szSz);

  const ezString& GetAssetsDocumentTags() const;
  void SetAssetsDocumentTags(const ezString& sTags);

  /// \brief Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  const ezReflectedClass* GetMetaInfo(const ezRTTI* pType) const;

  /// \brief Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  template <typename T>
  const T* GetMetaInfo() const
  {
    return static_cast<const T*>(GetMetaInfo(ezGetStaticRTTI<T>()));
  }

private:
  ezAssetDocumentInfo(const ezAssetDocumentInfo&);
  void operator=(const ezAssetDocumentInfo&) = delete;
};
