#pragma once

#include <Graphics/Basics.h>
#include <Graphics/Shader/ShaderStageBinary.h>

class EZ_GRAPHICS_DLL ezShaderPermutationBinary
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

