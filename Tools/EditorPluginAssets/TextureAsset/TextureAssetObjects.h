#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>

struct ezPropertyMetaStateEvent;

struct ezTexture2DUsageEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Unknown,
    Other_sRGB,
    Other_Linear,
    //Other_sRGB_Auto,
    //Other_Linear_Auto,
    Diffuse,
    NormalMap,
    EmissiveMask,
    EmissiveColor,
    Height,
    Mask,
    LookupTable,
    HDR,
    RenderTarget,

    Default = Unknown,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTexture2DUsageEnum);



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


struct ezTexture2DAddressMode
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Wrap = 0,
    Mirror,
    Clamp,

    Default = Wrap
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTexture2DAddressMode);

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
    ScreenSize,
    HalfScreenSize,

    Default = Fixed256x256
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTexture2DResolution);

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
  ezInt32 GetNumChannels() const;

  bool IsSRGB() const;
  bool IsHDR() const;

  bool m_bMipmaps;
  bool m_bCompression;
  bool m_bPremultipliedAlpha;
  bool m_bFlipHorizontal;

  ezEnum<ezTextureFilterSetting> m_TextureFilter;
  ezEnum<ezTexture2DAddressMode> m_AddressModeU;
  ezEnum<ezTexture2DAddressMode> m_AddressModeV;
  ezEnum<ezTexture2DAddressMode> m_AddressModeW;
  ezEnum<ezTexture2DResolution> m_Resolution;
  ezEnum<ezTexture2DUsageEnum> m_TextureUsage;

private:
  ezEnum<ezTexture2DChannelMappingEnum> m_ChannelMapping;
  ezString m_Input[4];
};
