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
    BaseColorORM,
    Normal,
    NormalORM,
    Emissive,

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
  bool m_bBlendModeModulate = false;

  ezString m_sBaseColor;
  ezString m_sNormal;
  ezString m_sORM;
  ezString m_sEmissive;

  bool NeedsBaseColor() const { return m_Mode == ezDecalMode::All || m_Mode == ezDecalMode::BaseColor || m_Mode == ezDecalMode::BaseColorORM; }
  bool NeedsNormal() const { return m_Mode == ezDecalMode::All || m_Mode == ezDecalMode::Normal || m_Mode == ezDecalMode::NormalORM; }
  bool NeedsORM() const { return m_Mode == ezDecalMode::All || m_Mode == ezDecalMode::BaseColorORM || m_Mode == ezDecalMode::NormalORM; }
  bool NeedsEmissive() const { return m_Mode == ezDecalMode::Emissive; }
};


class ezDecalAssetDocument : public ezSimpleAssetDocument<ezDecalAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocument, ezSimpleAssetDocument<ezDecalAssetProperties>);

public:
  ezDecalAssetDocument(const char* szDocumentPath);

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
