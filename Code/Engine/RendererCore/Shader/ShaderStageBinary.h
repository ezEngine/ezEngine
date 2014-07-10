#pragma once

#include <RendererCore/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>

class EZ_RENDERERCORE_DLL ezShaderStageBinary
{
public:
  ezShaderStageBinary();
  ~ezShaderStageBinary();

  ezResult Write(ezStreamWriterBase& Stream) const;
  ezResult Read(ezStreamReaderBase& Stream);

//private:
  ezUInt32 m_uiSourceHash;
  ezGALShaderStage::Enum m_Stage;
  ezDynamicArray<ezUInt8> m_ByteCode;
  ezScopedRefPointer<ezGALShaderByteCode> m_pGALByteCode;

  ezResult WriteStageBinary() const;

  static ezDeque<ezGALShaderByteCode*> s_GALByteCodes;

  static ezShaderStageBinary* LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash);

  static ezMap<ezUInt32, ezShaderStageBinary> s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
};

