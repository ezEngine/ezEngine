#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezImage;

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentInfo : public ezDocumentInfo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentInfo, ezDocumentInfo);

public:
  ezAssetDocumentInfo();

  ezString GetDependencies() const;
  void SetDependencies(ezString s);

  //ezString GetReferences() const;
  //void SetReferences(ezString s);

  ezUInt64 m_uiSettingsHash; ///< Current hash over all settings in the document, used to check resulting resource for being up-to-date in combination with dependency hashes.
  ezSet<ezString> m_FileDependencies;   ///< Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  //ezSet<ezString> m_FileReferences;     ///< Other files that are used at runtime together with this asset, e.g. materials for a mesh, currently not really implemented
  ezString m_sAssetTypeName;
};

class EZ_EDITORFRAMEWORK_DLL ezAssetDocument : public ezDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocument, ezDocument);

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
  void GetChildHash(const ezDocumentObject* pObject, ezUInt64& uiHash) const;
  virtual ezStatus InternalSaveDocument() override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) = 0;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) = 0;
  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) = 0;

  /// \brief Override this to change the version of the asset type. E.g. when the algorithm to transform an asset changes.
  /// This will ensure that assets get re-transformed, even though their settings and dependencies might not have changed.
  virtual ezUInt16 GetAssetTypeVersion() const = 0;

  void SaveThumbnail(const ezImage& img);

private:
  virtual ezDocumentInfo* CreateDocumentInfo() override;

};

