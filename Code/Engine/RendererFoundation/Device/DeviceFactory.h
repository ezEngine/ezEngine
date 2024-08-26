#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct EZ_RENDERERFOUNDATION_DLL ezGALDeviceFactory
{
  using CreatorFunc = ezDelegate<ezInternal::NewInstance<ezGALDevice>(ezAllocator*, const ezGALDeviceCreationDescription&)>;

  static ezInternal::NewInstance<ezGALDevice> CreateDevice(ezStringView sRendererName, ezAllocator* pAllocator, const ezGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(ezStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler);

  static void RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
  static void UnregisterCreatorFunc(const char* szRendererName);
};
