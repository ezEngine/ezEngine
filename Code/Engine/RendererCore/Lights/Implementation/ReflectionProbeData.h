#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezReflectionProbeMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezReflectionProbeMode);

/// \brief Describes how a cube map should be generated.
struct EZ_RENDERERCORE_DLL ezReflectionProbeDesc
{
  ezUuid m_uniqueID;

  ezTagSet m_IncludeTags;
  ezTagSet m_ExcludeTags;

  ezEnum<ezReflectionProbeMode> m_Mode;

  bool m_bShowDebugInfo = false;
  bool m_bShowMipMaps = false;

  float m_fIntensity = 1.0f;
  float m_fSaturation = 1.0f;
  float m_fNearPlane = 0.0f;
  float m_fFarPlane = 100.0f;
  ezVec3 m_vCaptureOffset = ezVec3::MakeZero();
};

using ezReflectionProbeId = ezGenericId<24, 8>;

template <>
struct ezHashHelper<ezReflectionProbeId>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezReflectionProbeId value) { return ezHashHelper<ezUInt32>::Hash(value.m_Data); }

  EZ_ALWAYS_INLINE static bool Equal(ezReflectionProbeId a, ezReflectionProbeId b) { return a == b; }
};

/// \brief Render data for a reflection probe.
class EZ_RENDERERCORE_DLL ezReflectionProbeRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReflectionProbeRenderData, ezRenderData);

public:
  ezReflectionProbeRenderData()
  {
    m_Id.Invalidate();
    m_vHalfExtents.SetZero();
  }

  ezReflectionProbeId m_Id;
  ezUInt32 m_uiIndex = 0;
  ezVec3 m_vProbePosition; ///< Probe position in world space.
  ezVec3 m_vHalfExtents;
  ezVec3 m_vPositiveFalloff;
  ezVec3 m_vNegativeFalloff;
  ezVec3 m_vInfluenceScale;
  ezVec3 m_vInfluenceShift;
};

/// \brief A unique reference to a reflection probe.
struct ezReflectionProbeRef
{
  bool operator==(const ezReflectionProbeRef& b) const
  {
    return m_Id == b.m_Id && m_uiWorldIndex == b.m_uiWorldIndex;
  }

  ezUInt32 m_uiWorldIndex = 0;
  ezReflectionProbeId m_Id;
};
static_assert(sizeof(ezReflectionProbeRef) == 8);

template <>
struct ezHashHelper<ezReflectionProbeRef>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezReflectionProbeRef value) { return ezHashHelper<ezUInt64>::Hash(reinterpret_cast<ezUInt64&>(value)); }

  EZ_ALWAYS_INLINE static bool Equal(ezReflectionProbeRef a, ezReflectionProbeRef b) { return a.m_Id == b.m_Id && a.m_uiWorldIndex == b.m_uiWorldIndex; }
};

/// \brief Flags that describe a reflection probe.
struct ezProbeFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    SkyLight = EZ_BIT(0),
    HasCustomCubeMap = EZ_BIT(1),
    Sphere = EZ_BIT(2),
    Box = EZ_BIT(3),
    Dynamic = EZ_BIT(4),
    Default = 0
  };

  struct Bits
  {
    StorageType SkyLight : 1;
    StorageType HasCustomCubeMap : 1;
    StorageType Sphere : 1;
    StorageType Box : 1;
    StorageType Dynamic : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezProbeFlags);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezProbeFlags);
