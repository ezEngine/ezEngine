#pragma once

#include <Core/World/Component.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;
struct ezMsgTransformChanged;
class ezAbstractObjectNode;

/// \brief Base class for all reflection probes.
class EZ_RENDERERCORE_DLL ezReflectionProbeComponentBase : public ezComponent
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReflectionProbeComponentBase, ezComponent);

public:
  ezReflectionProbeComponentBase();
  ~ezReflectionProbeComponentBase();

  void SetReflectionProbeMode(ezEnum<ezReflectionProbeMode> mode);           // [ property ]
  ezEnum<ezReflectionProbeMode> GetReflectionProbeMode() const;              // [ property ]

  const ezTagSet& GetIncludeTags() const;                                    // [ property ]
  void InsertIncludeTag(const char* szTag);                                  // [ property ]
  void RemoveIncludeTag(const char* szTag);                                  // [ property ]

  const ezTagSet& GetExcludeTags() const;                                    // [ property ]
  void InsertExcludeTag(const char* szTag);                                  // [ property ]
  void RemoveExcludeTag(const char* szTag);                                  // [ property ]

  float GetNearPlane() const { return m_Desc.m_fNearPlane; }                 // [ property ]
  void SetNearPlane(float fNearPlane);                                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; }                   // [ property ]
  void SetFarPlane(float fFarPlane);                                         // [ property ]

  const ezVec3& GetCaptureOffset() const { return m_Desc.m_vCaptureOffset; } // [ property ]
  void SetCaptureOffset(const ezVec3& vOffset);                              // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo);                                // [ property ]
  bool GetShowDebugInfo() const;                                             // [ property ]

  void SetShowMipMaps(bool bShowMipMaps);                                    // [ property ]
  bool GetShowMipMaps() const;                                               // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  float ComputePriority(ezMsgExtractRenderData& msg, ezReflectionProbeRenderData* pRenderData, float fVolume, const ezVec3& vScale) const;

protected:
  ezReflectionProbeDesc m_Desc;

  ezReflectionProbeId m_Id;
  // Set to true if a change was made that requires recomputing the cube map.
  mutable bool m_bStatesDirty = true;
};
