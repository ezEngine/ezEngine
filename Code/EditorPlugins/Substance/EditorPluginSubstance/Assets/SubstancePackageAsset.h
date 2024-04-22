#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Texture/TexConv/TexConvEnums.h>

struct ezSubstanceUsage
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Unknown,

    BaseColor,
    Emissive,
    Height,
    Metallic,
    Mask,
    Normal,
    Occlusion,
    Opacity,
    Roughness,

    Count,

    Default = Unknown
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINSUBSTANCE_DLL, ezSubstanceUsage);


struct ezSubstanceGraphOutput
{
  bool m_bEnabled = true;
  ezEnum<ezTexConvCompressionMode> m_CompressionMode = ezTexConvCompressionMode::High;
  ezEnum<ezSubstanceUsage> m_Usage;
  ezUInt8 m_uiNumChannels = 1;
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;
  bool m_bPreserveAlphaCoverage = false;
  ezString m_sName;
  ezString m_sLabel;
  ezUuid m_Uuid;

  bool operator==(const ezSubstanceGraphOutput& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_CompressionMode == other.m_CompressionMode &&
           m_Usage == other.m_Usage &&
           m_uiNumChannels == other.m_uiNumChannels &&
           m_MipmapMode == other.m_MipmapMode &&
           m_bPreserveAlphaCoverage == other.m_bPreserveAlphaCoverage &&
           m_sName == other.m_sName &&
           m_sLabel == other.m_sLabel &&
           m_Uuid == other.m_Uuid;
  }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINSUBSTANCE_DLL, ezSubstanceGraphOutput);


struct ezSubstanceGraph
{
  bool m_bEnabled = true;

  ezString m_sName;

  ezUInt8 m_uiOutputWidth = 0;  ///< In base 2, e.g. 8 = 2^8 = 256
  ezUInt8 m_uiOutputHeight = 0; ///< In base 2

  ezHybridArray<ezSubstanceGraphOutput, 8> m_Outputs;

  bool operator==(const ezSubstanceGraph& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_sName == other.m_sName &&
           m_uiOutputWidth == other.m_uiOutputWidth &&
           m_uiOutputHeight == other.m_uiOutputHeight &&
           m_Outputs == other.m_Outputs;
  }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINSUBSTANCE_DLL, ezSubstanceGraph);


class ezSubstancePackageAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubstancePackageAssetProperties, ezReflectedClass);

public:
  ezSubstancePackageAssetProperties() = default;

  ezString m_sSubstancePackage;
  ezString m_sOutputPattern;

  ezHybridArray<ezSubstanceGraph, 2> m_Graphs;
};


class ezSubstancePackageAssetMetaData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubstancePackageAssetMetaData, ezReflectedClass);

public:
  ezDynamicArray<ezUuid> m_OutputUuids;
  ezDynamicArray<ezString> m_OutputNames;
};

class ezTextureAssetProfileConfig;

class ezSubstancePackageAssetDocument : public ezSimpleAssetDocument<ezSubstancePackageAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubstancePackageAssetDocument, ezSimpleAssetDocument<ezSubstancePackageAssetProperties>);

public:
  ezSubstancePackageAssetDocument(ezStringView sDocumentPath);
  ~ezSubstancePackageAssetDocument();

private:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override { return ezStatus(EZ_SUCCESS); }
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  void OnPropertyChanged(const ezDocumentObjectPropertyEvent& e);

private:
  ezResult GetTempDir(ezStringBuilder& out_sTempDir) const;
  void GenerateOutputName(const ezSubstanceGraph& graph, const ezSubstanceGraphOutput& graphOutput, ezStringBuilder& out_sOutputName) const;
  ezTransformStatus UpdateGraphOutputs(ezStringView sAbsolutePath, bool bAllowPropertyModifications);
  ezStatus RunTexConv(const char* szInputFile, const char* szTargetFile, const ezAssetFileHeader& assetHeader, const ezSubstanceGraphOutput& graphOutput, ezStringView sThumbnailFile, const ezTextureAssetProfileConfig* pAssetConfig);
};
