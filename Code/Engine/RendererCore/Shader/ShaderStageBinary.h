#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// Serializes ezGALShaderByteCode and provides access to the shader cache.
///
/// Compiled shader stage binaries are cached on disk and accessed via LoadStageBinary using a hash.
/// This allows shader permutations to share compiled stage binaries when they use the same code.
class EZ_RENDERERCORE_DLL ezShaderStageBinary
{
public:
  /// Serialization version for shader stage binaries.
  enum Version
  {
    Version0,
    Version1,
    Version2,
    Version3, ///< Added Material Parameters
    Version4, ///< Constant buffer layouts
    Version5, ///< Debug flag
    Version6, ///< Rewrite, no backwards compatibility. Moves all data into ezGALShaderByteCode.
    Version7, ///< Added tessellation support (m_uiTessellationPatchControlPoints)

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  ezShaderStageBinary();
  ~ezShaderStageBinary();

  /// Returns the compiled shader bytecode.
  ezSharedPtr<const ezGALShaderByteCode> GetByteCode() const;

private:
  friend class ezRenderContext;
  friend class ezShaderCompiler;
  friend class ezShaderPermutationResource;
  friend class ezShaderPermutationResourceLoader;

  ezResult WriteStageBinary(ezLogInterface* pLog, ezStringView sPlatform) const;
  ezResult Write(ezStreamWriter& inout_stream) const;
  ezResult Read(ezStreamReader& inout_stream);
  ezResult Write(ezStreamWriter& inout_stream, const ezShaderConstantBufferLayout& layout) const;
  ezResult Read(ezStreamReader& inout_stream, ezShaderConstantBufferLayout& out_layout);

private:
  ezUInt32 m_uiSourceHash = 0;
  ezSharedPtr<ezGALShaderByteCode> m_pGALByteCode;

private: // statics
  /// Loads a shader stage binary from the cache by hash.
  ///
  /// Returns nullptr if the binary is not in the cache. Binaries are cached per stage and platform.
  static ezShaderStageBinary* LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash, ezStringView sPlatform);

  static void OnEngineShutdown();

  static ezMutex s_ShaderStageBinariesLock;
  static ezMap<ezUInt32, ezShaderStageBinary> s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
};
