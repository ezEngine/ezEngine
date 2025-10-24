#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// Descriptor for GPU pipeline state associated with a shader.
///
/// Contains blend, depth-stencil, and rasterizer state descriptions.
/// Parsed from shader source and serialized with shader permutations.
struct EZ_RENDERERCORE_DLL ezShaderStateResourceDescriptor
{
  ezGALBlendStateCreationDescription m_BlendDesc;
  ezGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  ezGALRasterizerStateCreationDescription m_RasterizerDesc;

  /// Parses state descriptions from shader source text.
  ezResult Parse(const char* szSource);

  void Load(ezStreamReader& inout_stream);
  void Save(ezStreamWriter& inout_stream) const;

  /// Calculates a hash of all state descriptions for comparison.
  ezUInt32 CalculateHash() const;
};

/// Serialized state of a shader permutation.
///
/// Used by ezShaderPermutationResourceLoader to convert into an ezShaderPermutationResource.
/// Contains hashes to shader stage binaries, pipeline state, dependencies, and permutation variable values.
class EZ_RENDERERCORE_DLL ezShaderPermutationBinary
{
public:
  ezShaderPermutationBinary();

  ezResult Write(ezStreamWriter& inout_stream);
  ezResult Read(ezStreamReader& inout_stream, bool& out_bOldVersion);

  /// Hashes of compiled shader stage binaries for each shader stage.
  ///
  /// The actual binary will be loaded from the hash via ezShaderStageBinary::LoadStageBinary
  /// to produce ezShaderStageBinary objects.
  ezUInt32 m_uiShaderStageHashes[ezGALShaderStage::ENUM_COUNT];

  ezDependencyFile m_DependencyFile;                     ///< File dependencies for hot-reloading.

  ezShaderStateResourceDescriptor m_StateDescriptor;     ///< Pipeline state (blend, depth-stencil, rasterizer).

  ezHybridArray<ezPermutationVar, 16> m_PermutationVars; ///< Values of permutation variables for this permutation.
};
