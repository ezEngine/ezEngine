
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>

#include <RendererFoundation/Descriptors/Descriptors.h>

#include <Helper/Shader.h>
#include <Helper/Misc.h>

#include <d3dcompiler.h>

namespace DontUse
{
  ezGALShaderByteCode* CompileDXShader(const char* source, const char* profile, const char* entryPoint)
  {
    ID3DBlob* ResultBlob = NULL;
    ID3DBlob* ErrorBlob = NULL;

    if(FAILED(D3DCompile(source, strlen(source), "<file>", NULL, NULL, entryPoint, profile, 0, 0, &ResultBlob, &ErrorBlob)))
    {
      const char* szError = (const char*)ErrorBlob->GetBufferPointer();

      EZ_ASSERT(false, "Shader compilation failed for profile %s: %s", profile, szError);

      return NULL;
    }


    ezGALShaderByteCode* pByteCode = new ezGALShaderByteCode(ResultBlob->GetBufferPointer(), (ezUInt32)ResultBlob->GetBufferSize());

    return pByteCode;
  }


  bool ShaderCompiler::Compile(const char* szPath, ezGALShaderCreationDescription& Result)
  {
    ezDynamicArray<ezUInt8> FileContent;
    FileContent.Reserve(64 * 1024);

    if(!ReadCompleteFile(szPath, FileContent).IsSuccess())
      return false;

    FileContent.PushBack('\0');

    ezStringBuilder sContent = (const char*) &FileContent[0];

    Result.m_ByteCodes[ezGALShaderStage::VertexShader] = CompileDXShader(sContent.GetData(), "vs_5_0", "vs_main");
    Result.m_ByteCodes[ezGALShaderStage::PixelShader] = CompileDXShader(sContent.GetData(), "ps_5_0", "ps_main");

    return true;
  }

}