#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>

class ezShaderCompilerGLSL : public ezShaderProgramCompiler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerGLSL);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& Platforms) override
  {
    Platforms.PushBack("GL3");
    Platforms.PushBack("GL4");
  }

  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) override;


};




