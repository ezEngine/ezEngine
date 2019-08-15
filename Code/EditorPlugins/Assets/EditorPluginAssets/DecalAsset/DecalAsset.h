#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

class ezAssetFileHeader;
struct ezPropertyMetaStateEvent;

struct ezDecalMode
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    All,
    BaseColor,
    BaseColorRoughness,
    NormalRoughnessOcclusion,
    Emissive,

    Default = All
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezDecalMode);

class ezDecalAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetProperties, ezReflectedClass);

public:
  ezDecalAssetProperties();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezEnum<ezDecalMode> m_Mode;

  ezString m_sBaseColor;
  ezString m_sNormal;
  ezString m_sRoughness;
  float m_fRoughnessValue;
  ezString m_sMetallic;
  float m_fMetallicValue;
  ezString m_sEmissive;
  ezString m_sOcclusion;
  float m_fOcclusionValue;
};


class ezDecalAssetDocument : public ezSimpleAssetDocument<ezDecalAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocument, ezSimpleAssetDocument<ezDecalAssetProperties>);

public:
  ezDecalAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class ezDecalAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezDecalAssetDocumentGenerator();
  ~ezDecalAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezDecalAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "Images"; }
};
