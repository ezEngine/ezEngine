#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

/// \brief Usage hint of a camera component.
struct EZ_RENDERERCORE_DLL ezCameraComponentUsageHint
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    None,
    MainView,
    EditorView,
    Thumbnail,

    ENUM_COUNT,

    Default = None,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezCameraComponentUsageHint);



