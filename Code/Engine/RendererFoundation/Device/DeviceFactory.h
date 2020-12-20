#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct EZ_RENDERERFOUNDATION_DLL ezGALDeviceFactory
{
  using CreatorFunc = ezDelegate<ezGALDevice*(ezAllocatorBase*, const ezGALDeviceCreationDescription&)>;

  static ezGALDevice* CreateDevice(const char* szRendererName, ezAllocatorBase* pAllocator, const ezGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(const char* szRendererName, const char*& szShaderModel, const char*& szShaderCompiler);

  static bool RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
};
