#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>

struct ezPropertyMetaStateEvent;

struct ezTextureCubeChannelMappingEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    RGB1,
    RGBA1,

    RGB1TO6,
    RGBA1TO6,

    Default = RGB1,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTextureCubeChannelMappingEnum);


class ezTextureCubeAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeAssetProperties, ezReflectedClass);

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
  ezInt32 GetNumInputFiles() const;

  ezEnum<ezTexConvCompressionMode> m_CompressionMode;
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;

  ezEnum<ezTextureFilterSetting> m_TextureFilter;
  ezEnum<ezTexConvUsage> m_TextureUsage;
  ezEnum<ezTextureCubeChannelMappingEnum> m_ChannelMapping;

private:
  ezString m_Input[6];
};
