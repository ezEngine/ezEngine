#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezMaterialAssetDocument;

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

  void SetDocument(ezMaterialAssetDocument* pDocument);
  void UpdateShader(bool bForce = false);

  void DeleteProperties();
  void CreateProperties(const char* szShaderPath, bool bForce = false);

  void SaveOldValues();
  void LoadOldValues();
  const ezRTTI* UpdateShaderType(const char* szShaderPath);

public:
  ezString m_sBaseMaterial;
  ezString m_sShader;
  ezString m_sPermutationVarValues;
  ezString m_sTextureDiffuse;
  ezString m_sTextureMask;
  ezString m_sTextureNormal;

  ezMap<ezString, ezVariant> m_CachedProperties;
  ezMaterialAssetDocument* m_pDocument;
};

class ezMaterialAssetDocument : public ezSimpleAssetDocument<ezMaterialAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetDocument, ezSimpleAssetDocument<ezMaterialAssetProperties>);

public:
  ezMaterialAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Material Asset"; }

  virtual const char* QueryAssetType() const override { return "Material"; }

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetFlags() const;

  ezDocumentObject* GetShaderPropertyObject();

  void SetBaseMaterial(const char* szBaseMaterial);

protected:
  ezUuid GetSeedFromBaseMaterial(const char* szBaseGraph);
  static ezUuid GetMaterialNodeGuid(const ezAbstractObjectGraph& graph);
  virtual void UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab) override;
  virtual void InitializeAfterLoading() override;

  virtual ezUInt16 GetAssetTypeVersion() const override { return 1; }
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) override;
  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override { return ezStatus(EZ_SUCCESS); }
};
