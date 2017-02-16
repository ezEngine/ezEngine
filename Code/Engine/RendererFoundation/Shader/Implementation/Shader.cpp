
#include <PCH.h>
#include <RendererFoundation/Shader/Shader.h>

ezGALShader::ezGALShader(const ezGALShaderCreationDescription& Description)
  : ezGALObject(Description)
{
}

ezGALShader::~ezGALShader()
{
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_Shader);

