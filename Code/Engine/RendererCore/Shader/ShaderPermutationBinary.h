#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Shader/ShaderStageBinary.h>

class EZ_RENDERERCORE_DLL ezShaderPermutationBinary
{
public:
  ezShaderPermutationBinary();

  ezResult Write(ezStreamWriterBase& Stream) const;
  ezResult Read(ezStreamReaderBase& Stream);

  ezUInt32 m_uiShaderStateHash;
  ezUInt32 m_uiShaderStageHashes[ezGALShaderStage::ENUM_COUNT];

  ezDynamicArray<ezString> m_IncludeFiles;
  ezInt64 m_iMaxTimeStamp;
};

