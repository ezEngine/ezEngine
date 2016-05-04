#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

struct ezPropertyMetaStateEvent;

struct ezTextureUsageEnum
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
    Skybox,

    Default = Unknown,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTextureUsageEnum);



struct ezChannelMappingEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    R1_2D,

    RG1_2D,
    R1_G2_2D,

    RGB1_2D,
    R1_G2_B3_2D,

    RGBA1_2D,
    RGB1_A2_2D,
    R1_G2_B3_A4_2D,

    RGB1_CUBE,
    RGBA1_CUBE,

    Default = RGB1_2D,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezChannelMappingEnum);

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
  void SetInputFile4(const char* szFile) { m_Input[4] = szFile; }
  const char* GetInputFile4() const { return m_Input[4]; }
  void SetInputFile5(const char* szFile) { m_Input[5] = szFile; }
  const char* GetInputFile5() const { return m_Input[5]; }

  ezString GetAbsoluteInputFilePath(ezInt32 iInput) const;

  ezChannelMappingEnum::Enum GetChannelMapping() const { return m_ChannelMapping; }

  ezInt32 GetNumInputFiles() const;
  ezInt32 GetNumChannels() const;

  bool IsSRGB() const;
  bool IsTexture2D() const;
  bool IsTextureCube() const;

  bool m_bMipmaps;
  bool m_bCompression;

private:
  ezEnum<ezTextureUsageEnum> m_TextureUsage;
  ezEnum<ezChannelMappingEnum> m_ChannelMapping;
  ezString m_Input[6];
};
