#pragma once

#include <Foundation/Types/TagSet.h>
#include <RendererCore/Declarations.h>

struct ezReflectionProbeMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Static,
    DynamicLit,
    FullDynamic,

    Default = Static
  };
};

typedef ezGenericId<24, 8> ezReflectionProbeId;

struct EZ_RENDERERCORE_DLL ezReflectionProbeData
{
  ezReflectionProbeId m_Id;

  ezTagSet m_IncludeTags;
  ezTagSet m_ExcludeTags;

  ezEnum<ezReflectionProbeMode> m_Mode;
  bool m_bShowDebugInfo = false;

  float m_fIntensity = 1.0f;
  float m_fSaturation = 1.0f;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezReflectionProbeData);
