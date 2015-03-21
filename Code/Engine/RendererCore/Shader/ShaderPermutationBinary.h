#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <Foundation/IO/DependencyFile.h>

class EZ_RENDERERCORE_DLL ezShaderPermutationBinary
{
public:
  ezShaderPermutationBinary();

  ezResult Write(ezStreamWriterBase& Stream);
  ezResult Read(ezStreamReaderBase& Stream);

  ezUInt32 m_uiShaderStateHash;
  ezUInt32 m_uiShaderStageHashes[ezGALShaderStage::ENUM_COUNT];

  ezDependencyFile m_DependencyFile;
};

