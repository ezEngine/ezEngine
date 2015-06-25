#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezImage;

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentInfo : public ezDocumentInfo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentInfo);

public:
  ezAssetDocumentInfo();

  ezString GetDependencies() const;
  void SetDependencies(ezString s);

  ezString GetReferences() const;
  void SetReferences(ezString s);

  ezUInt64 m_uiSettingsHash; ///< Current hash over all settings in the document, used to check resulting resource for being up-to-date in combination with dependency hashes.
  ezHybridArray<ezString, 16> m_FileDependencies;   ///< Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  ezHybridArray<ezString, 16> m_FileReferences;     ///< Other files that are used at runtime together with this asset, e.g. materials for a mesh
  ezString m_sAssetTypeName;
};

class EZ_EDITORFRAMEWORK_DLL ezAssetDocument : public ezDocumentBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocument);

public:
  ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager);
  ~ezAssetDocument();

  /// \brief Returns one of the strings that ezAssetDocumentManager::QuerySupportedAssetTypes returned.
  ///
  /// This can be different for each instance of the same asset document type.
  /// E.g. one texture resource may return 'Texture 2D' and another 'Texture 3D'.
  /// Likewise completely different asset document types may use the same 'asset types'.
  ///
  /// This is mostly used for sorting and filtering in the asset browser.
  virtual const char* QueryAssetType() const = 0;

  ezStatus TransformAsset(const char* szPlatform = "");

  ezStatus RetrieveAssetInfo(const char* szPlatform = "");

  struct AssetEvent
  {
    enum class Type
    {
      AssetInfoChanged,
    };

    Type m_Type;
  };

  ezEvent<const AssetEvent&> m_AssetEvents;

protected:
  ezUInt64 GetDocumentHash() const;
  void GetChildHash(const ezDocumentObjectBase* pObject, ezUInt64& uiHash) const;
  virtual ezStatus InternalSaveDocument() override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) = 0;
  virtual ezStatus InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform) = 0;
  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) = 0;

  void SaveThumbnail(const ezImage& img);

private:
  virtual ezDocumentInfo* CreateDocumentInfo() override;

};

