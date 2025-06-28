#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class ezRemoteMessage;

/// \brief Shader compiler interface.
/// Custom shader compiles need to derive from this class and implement the pure virtual interface functions. Instances are created via reflection so each implementation must be properly reflected.
class EZ_RENDERERCORE_DLL ezShaderProgramCompiler : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderProgramCompiler, ezReflectedClass);

public:
  /// \brief Returns the platforms that this shader compiler supports.
  /// \param out_platforms Filled with the platforms this compiler supports.
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& out_platforms) = 0;

  /// \brief Returns the layout used for material buffers on the given platform
  /// \param sPlatform The platform for which the layout is to be retrieved
  /// \return an ezGALBufferLayout value.
  virtual ezEnum<ezGALBufferLayout> GetMaterialBufferLayout(ezStringView sPlatform) const = 0;

  /// Allows the shader compiler to modify the shader source before hashing and compiling. This allows it to implement custom features by injecting code before the compile process. Mostly used to define resource bindings that do not cause conflicts across shader stages.
  /// \param inout_data The state of the shader compiler. Only m_sShaderSource should be modified by the implementation.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader could be modified. On failure, the shader won't be compiled.
  virtual ezResult ModifyShaderSource(ezShaderProgramData& inout_data, ezLogInterface* pLog) = 0;

  /// Compiles the shader comprised of multiple stages defined in inout_data.
  /// \param inout_data The state of the shader compiler. m_Resources and m_ByteCode should be written to on successful return code.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader was compiled successfully. On failure, errors should be written to pLog.
  virtual ezResult Compile(ezShaderProgramData& inout_data, ezLogInterface* pLog) = 0;
};

class EZ_RENDERERCORE_DLL ezShaderCompiler
{
public:
  ezResult CompileShaderPermutationForPlatforms(ezStringView sFile, const ezArrayPtr<const ezPermutationVar>& permutationVars, ezLogInterface* pLog, ezStringView sPlatform = "ALL");

private:
  ezResult RunShaderCompiler(ezStringView sFile, ezStringView sPlatform, ezShaderProgramCompiler* pCompiler, ezLogInterface* pLog);

  void WriteFailedShaderSource(ezShaderProgramData& spd, ezLogInterface* pLog);

  bool PassThroughUnknownCommandCB(ezStringView sCmd) { return sCmd == "version"; }

  void ShaderCompileMsg(ezRemoteMessage& msg);

  struct ezShaderData
  {
    ezString m_Platforms;
    ezHybridArray<ezPermutationVar, 16> m_Permutations;
    ezHybridArray<ezPermutationVar, 16> m_FixedPermVars;
    ezString m_StateSource;
    ezString m_ShaderStageSource[ezGALShaderStage::ENUM_COUNT];
  };

  ezResult FileOpen(ezStringView sAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification);

  ezSharedPtr<ezShaderConstantBufferLayout> m_pMaterialBufferLayout;
  ezSet<ezString> m_MaterialParameters;
  ezStringBuilder m_StageSourceFile[ezGALShaderStage::ENUM_COUNT];

  ezTokenizedFileCache m_FileCache;
  ezShaderData m_ShaderData;

  ezSet<ezString> m_IncludeFiles;
  bool m_bCompilingShaderRemote = false;
  ezResult m_RemoteShaderCompileResult = EZ_FAILURE;
};
