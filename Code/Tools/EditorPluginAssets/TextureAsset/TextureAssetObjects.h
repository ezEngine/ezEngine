#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>

struct ezPropertyMetaStateEvent;

struct ezTexture2DChannelMappingEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    R1,

    RG1,
    R1_G2,

    RGB1,
    R1_G2_B3,

    RGBA1,
    RGB1_A2,
    RGB1_ABLACK,
    R1_G2_B3_A4,

    Default = RGB1,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTexture2DChannelMappingEnum);

struct ezTexture2DResolution
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Fixed64x64,
    Fixed128x128,
    Fixed256x256,
    Fixed512x512,
    Fixed1024x1024,
    Fixed2048x2048,
    CVarRtResolution1,
    CVarRtResolution2,

    Default = Fixed256x256
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTexture2DResolution);

struct ezRenderTargetFormat
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    RGBA8sRgb,
    RGBA8,
    RGB10,
    RGBA16,

    Default = RGBA8sRgb
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezRenderTargetFormat);

class ezTextureAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetProperties, ezReflectedClass);

public:
  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  const char* GetInputFile(ezInt32 iInput) const { return m_Input[iInput]; }

  void SetInputFile0(const char* szFile) { m_Input[0] = szFile; }
  const char* GetInputFile0() const { return m_Input[0]; }
  void SetInputFile1(const char* szFile) { m_Input[1] = szFile; }
  const char* GetInputFile1() const { return m_Input[1]; }
  void SetInputFile2(const char* szFile) { m_Input[2] = szFile; }
  const char* GetInputFile2() const { return m_Input[2]; }
  void SetInputFile3(const char* szFile) { m_Input[3] = szFile; }
  const char* GetInputFile3() const { return m_Input[3]; }

  ezString GetAbsoluteInputFilePath(ezInt32 iInput) const;

  ezTexture2DChannelMappingEnum::Enum GetChannelMapping() const { return m_ChannelMapping; }

  ezInt32 GetNumInputFiles() const;

  bool m_bIsRenderTarget = false;
  bool m_bPremultipliedAlpha = false;
  bool m_bFlipHorizontal = false;
  float m_fCVarResolutionScale = 1.0f;

  ezEnum<ezTextureFilterSetting> m_TextureFilter;
  ezEnum<ezImageAddressMode> m_AddressModeU;
  ezEnum<ezImageAddressMode> m_AddressModeV;
  ezEnum<ezImageAddressMode> m_AddressModeW;
  ezEnum<ezTexture2DResolution> m_Resolution;
  ezEnum<ezTexConvUsage> m_TextureUsage;
  ezEnum<ezRenderTargetFormat> m_RtFormat;

  ezEnum<ezTexConvCompressionMode> m_CompressionMode;
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;

private:
  ezEnum<ezTexture2DChannelMappingEnum> m_ChannelMapping;
  ezString m_Input[4];
};
