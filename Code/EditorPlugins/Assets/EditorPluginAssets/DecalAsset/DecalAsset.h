#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezAssetFileHeader;
struct ezPropertyMetaStateEvent;

struct ezDecalMode
{
  using StorageType = ezInt8;

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

  /// Into how many columns and rows the input textures are subdivided.
  ///
  /// Every cell must contain an independent variation of the decal. All input textures (base color, normal, ...)
  /// must use the same subdivision. The texture is packed into the decal atlas as a whole, an ezDecalComponent
  /// then displays only a single cell.
  ezUInt8 m_uiNumVariationsX = 1;
  ezUInt8 m_uiNumVariationsY = 1;

  /// If no base color texture is given, an opaque white one is generated, so that the alpha mask alone defines the decal.
  bool NeedsBaseColor() const { return !m_sBaseColor.IsEmpty(); }
  bool NeedsNormal() const { return m_Mode == ezDecalMode::BaseColorNormal || m_Mode == ezDecalMode::BaseColorNormalORM; }
  bool NeedsORM() const { return m_Mode == ezDecalMode::BaseColorORM || m_Mode == ezDecalMode::BaseColorNormalORM; }
  bool NeedsEmissive() const { return m_Mode == ezDecalMode::BaseColorEmissive; }
};


class ezDecalAssetDocument : public ezSimpleAssetDocument<ezDecalAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocument, ezSimpleAssetDocument<ezDecalAssetProperties>);

public:
  ezDecalAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
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

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezDecalAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Images"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
