#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct EZ_RENDERERCORE_DLL ezShaderStateResourceDescriptor
{
  ezGALBlendStateCreationDescription m_BlendDesc;
  ezGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  ezGALRasterizerStateCreationDescription m_RasterizerDesc;

  ezResult Parse(const char* szSource);
  void Load(ezStreamReader& inout_stream);
  void Save(ezStreamWriter& inout_stream) const;

  ezUInt32 CalculateHash() const;
};

class EZ_RENDERERCORE_DLL ezShaderPermutationBinary
{
public:
  ezShaderPermutationBinary();

  ezResult Write(ezStreamWriter& inout_stream);
  ezResult Read(ezStreamReader& inout_stream, bool& out_bOldVersion);

  ezUInt32 m_uiShaderStageHashes[ezGALShaderStage::ENUM_COUNT];

  ezDependencyFile m_DependencyFile;

  ezShaderStateResourceDescriptor m_StateDescriptor;

  ezHybridArray<ezPermutationVar, 16> m_PermutationVars;
};
