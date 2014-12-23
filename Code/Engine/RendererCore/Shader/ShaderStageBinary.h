#pragma once

#include <RendererCore/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>

struct ezShaderStageResource
{
  enum ResourceType
  {
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
    ConstantBuffer
  };

  ResourceType m_Type;
  ezInt32 m_iSlot;
  ezHashedString m_Name;
};

class EZ_RENDERERCORE_DLL ezShaderStageBinary
{
public:
  enum Version
  {
    Version0,
    Version1,
    Version2,

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  ezShaderStageBinary();
  ~ezShaderStageBinary();

  ezResult Write(ezStreamWriterBase& Stream) const;
  ezResult Read(ezStreamReaderBase& Stream);

//private:
  ezUInt32 m_uiSourceHash;
  ezGALShaderStage::Enum m_Stage;
  ezDynamicArray<ezUInt8> m_ByteCode;
  ezScopedRefPointer<ezGALShaderByteCode> m_pGALByteCode;
  ezHybridArray<ezShaderStageResource, 8> m_ShaderResourceBindings;

  ezResult WriteStageBinary() const;

  static ezDeque<ezGALShaderByteCode*> s_GALByteCodes;

  static ezShaderStageBinary* LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash);

  static ezMap<ezUInt32, ezShaderStageBinary> s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
};

