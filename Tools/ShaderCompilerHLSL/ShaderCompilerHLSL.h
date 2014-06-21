#pragma once

#include <Graphics/ShaderCompiler/ShaderCompiler.h>

class ezShaderCompilerHLSL : public ezShaderProgramCompiler
{
public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& Platforms) override
  {
    Platforms.PushBack("DX11_SM40_93");
    Platforms.PushBack("DX11_SM40");
    Platforms.PushBack("DX11_SM41");
    Platforms.PushBack("DX11_SM50");
  }

  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) override;


};




