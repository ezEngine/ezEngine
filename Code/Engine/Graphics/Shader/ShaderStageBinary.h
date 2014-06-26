#pragma once

#include <Graphics/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>

class EZ_GRAPHICS_DLL ezShaderStageBinary
{
public:
  ezShaderStageBinary();

  ezResult Write(ezStreamWriterBase& Stream) const;
  ezResult Read(ezStreamReaderBase& Stream);

//private:
  ezUInt32 m_uiSourceHash;
  ezGALShaderStage::Enum m_Stage;
  ezDynamicArray<ezUInt8> m_ByteCode;


  static ezMap<ezUInt32, ezShaderStageBinary> s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
};

