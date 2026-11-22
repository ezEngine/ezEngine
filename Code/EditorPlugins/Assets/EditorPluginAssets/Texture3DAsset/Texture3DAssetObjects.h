#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct ezPropertyMetaStateEvent;

class ezTexture3DAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DAssetProperties, ezReflectedClass);

public:
  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  const char* GetInputFile() const { return m_sInput; }
  void SetInputFile(const char* szFile) { m_sInput = szFile; }

  ezString GetAbsoluteInputFilePath() const;

  ezEnum<ezTextureFilterSetting> m_TextureFilter;
  ezEnum<ezImageAddressMode> m_AddressModeU;
  ezEnum<ezImageAddressMode> m_AddressModeV;
  ezEnum<ezImageAddressMode> m_AddressModeW;
  ezEnum<ezTexConvUsage> m_TextureUsage;
  ezEnum<ezTexConvCompressionMode> m_CompressionMode;
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;
  float m_fHdrExposureBias = 0;

private:
  ezString m_sInput;
};
