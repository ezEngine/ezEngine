#include <RendererFoundationPCH.h>

#include <RendererFoundation/Device/DeviceFactory.h>

struct CreatorFuncInfo
{
  ezGALDeviceFactory::CreatorFunc m_Func;
  ezString m_sShaderModel;
  ezString m_sShaderCompiler;
};

static ezHashTable<ezString, CreatorFuncInfo> s_CreatorFuncs;

CreatorFuncInfo* GetCreatorFuncInfo(const char* szRendererName)
{
  ezStringBuilder sb = szRendererName;
  sb.ToLower();

  return s_CreatorFuncs.GetValue(sb);
}

ezGALDevice* ezGALDeviceFactory::CreateDevice(const char* szRendererName, ezAllocatorBase* pAllocator, const ezGALDeviceCreationDescription& desc)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(szRendererName))
  {
    return pFuncInfo->m_Func(pAllocator, desc);
  }

  return nullptr;
}

void ezGALDeviceFactory::GetShaderModelAndCompiler(const char* szRendererName, const char*& szShaderModel, const char*& szShaderCompiler)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(szRendererName))
  {
    szShaderModel = pFuncInfo->m_sShaderModel;
    szShaderCompiler = pFuncInfo->m_sShaderCompiler;
  }
}
