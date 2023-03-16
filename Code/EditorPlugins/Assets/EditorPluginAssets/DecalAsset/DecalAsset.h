#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezAssetFileHeader;
struct ezPropertyMetaStateEvent;

struct ezDecalMode
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    BaseColor,
    BaseColorNormal,
    BaseColorORM,
    BaseColorNormalORM,
    BaseColorEmissive,

    Default = BaseColor
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
  bool m_bBlendModeColorize = false;

  ezString m_sAlphaMask;
  ezString m_sBaseColor;
  ezString m_sNormal;
  ezString m_sORM;
  ezString m_sEmissive;

  bool NeedsBaseColor() const { return true; }
  bool NeedsNormal() const { return m_Mode == ezDecalMode::BaseColorNormal || m_Mode == ezDecalMode::BaseColorNormalORM; }
  bool NeedsORM() const { return m_Mode == ezDecalMode::BaseColorORM || m_Mode == ezDecalMode::BaseColorNormalORM; }
  bool NeedsEmissive() const { return m_Mode == ezDecalMode::BaseColorEmissive; }
};


class ezDecalAssetDocument : public ezSimpleAssetDocument<ezDecalAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocument, ezSimpleAssetDocument<ezDecalAssetProperties>);

public:
  ezDecalAssetDocument(const char* szDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class ezDecalAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezDecalAssetDocumentGenerator();
  ~ezDecalAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual ezStringView GetDocumentExtension() const override { return "ezDecalAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Images"; }
};
