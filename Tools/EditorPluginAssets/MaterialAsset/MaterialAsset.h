#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <VisualShader/VisualShaderNodeManager.h>

class ezMaterialAssetDocument;
struct ezPropertyMetaStateEvent;

struct ezMaterialShaderMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    BaseMaterial,
    File,
    Custom,

    Default = File
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezMaterialShaderMode);

class ezMaterialAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetProperties, ezReflectedClass);

public:
  ezMaterialAssetProperties() : m_pDocument(nullptr) {}

  void SetBaseMaterial(const char* szBaseMaterial);
  const char* GetBaseMaterial() const;
  void SetShader(const char* szShader);
  const char* GetShader() const;
  void SetShaderProperties(ezReflectedClass* pProperties);
  ezReflectedClass* GetShaderProperties() const;
  void SetShaderMode(ezEnum<ezMaterialShaderMode> mode);
  ezEnum<ezMaterialShaderMode> GetShaderMode() const { return m_ShaderMode; }

  void SetDocument(ezMaterialAssetDocument* pDocument);
  void UpdateShader(bool bForce = false);

  void DeleteProperties();
  void CreateProperties(const char* szShaderPath);

  void SaveOldValues();
  void LoadOldValues();

  ezString GetFinalShader() const;
  ezString GetAutoGenShaderPathAbs() const;

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

public:
  ezString m_sBaseMaterial;
  ezString m_sShader;

  ezMap<ezString, ezVariant> m_CachedProperties;
  ezMaterialAssetDocument* m_pDocument;
  ezEnum<ezMaterialShaderMode> m_ShaderMode;
};

class ezMaterialAssetDocument : public ezSimpleAssetDocument<ezMaterialAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetDocument, ezSimpleAssetDocument<ezMaterialAssetProperties>);

public:
  ezMaterialAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Material"; }

  ezDocumentObject* GetShaderPropertyObject();
  const ezDocumentObject* GetShaderPropertyObject() const;

  void SetBaseMaterial(const char* szBaseMaterial);

  ezStatus WriteMaterialAsset(ezStreamWriter& stream, const char* szPlatform) const;

  /// \brief Will make sure that the visual shader is rebuilt.
  /// Typically called during asset transformation, but can be triggered manually to enforce getting visual shader node changes in.
  ezStatus RecreateVisualShaderFile(const char* szPlatform = nullptr);

  /// \brief Deletes all Visual Shader nodes that are not connected to the output
  void RemoveDisconnectedNodes();

  static ezUuid GetLitBaseMaterial();
  static ezUuid GetLitAlphaTextBaseMaterial();
  static ezUuid GetNeutralNormalMap();


  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition) override;

protected:
  ezUuid GetSeedFromBaseMaterial(const ezAbstractObjectGraph* pBaseGraph);
  static ezUuid GetMaterialNodeGuid(const ezAbstractObjectGraph& graph);
  virtual void UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab) override;
  virtual void InitializeAfterLoading() override;

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

  virtual void InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph) override;

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;


private:
  static ezUuid s_LitBaseMaterial;
  static ezUuid s_LitAlphaTextBaseMaterial;
  static ezUuid s_NeutralNormalMap;
};

class ezMaterialObjectManager : public ezVisualShaderNodeManager
{
};
