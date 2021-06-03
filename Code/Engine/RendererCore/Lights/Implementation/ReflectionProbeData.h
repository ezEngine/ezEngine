#pragma once

#include <Foundation/Types/TagSet.h>
#include <RendererCore/Declarations.h>

struct ezReflectionProbeMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezReflectionProbeMode);

typedef ezGenericId<24, 8> ezReflectionProbeId;

struct EZ_RENDERERCORE_DLL ezReflectionProbeData
{
  ezReflectionProbeId m_Id;

  ezTagSet m_IncludeTags;
  ezTagSet m_ExcludeTags;

  ezEnum<ezReflectionProbeMode> m_Mode;
  ezTextureCubeResourceHandle m_hCubeMap;

  bool m_bShowDebugInfo = false;
  float m_fIntensity = 1.0f;
  float m_fSaturation = 1.0f;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream, const ezUInt32 uiVersion);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezReflectionProbeData);
