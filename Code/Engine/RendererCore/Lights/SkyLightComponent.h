#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Textures/TextureCubeResource.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;
struct ezMsgTransformChanged;

using ezSkyLightComponentManager = ezSettingsComponentManager<class ezSkyLightComponent>;

class EZ_RENDERERCORE_DLL ezSkyLightComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkyLightComponent, ezSettingsComponent, ezSkyLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSkyLightComponent

public:
  ezSkyLightComponent();
  ~ezSkyLightComponent();

  void SetReflectionProbeMode(ezEnum<ezReflectionProbeMode> mode); // [ property ]
  ezEnum<ezReflectionProbeMode> GetReflectionProbeMode() const;    // [ property ]

  void SetIntensity(float fIntensity);                             // [ property ]
  float GetIntensity() const;                                      // [ property ]

  void SetSaturation(float fSaturation);                           // [ property ]
  float GetSaturation() const;                                     // [ property ]

  const ezTagSet& GetIncludeTags() const;                          // [ property ]
  void InsertIncludeTag(const char* szTag);                        // [ property ]
  void RemoveIncludeTag(const char* szTag);                        // [ property ]

  const ezTagSet& GetExcludeTags() const;                          // [ property ]
  void InsertExcludeTag(const char* szTag);                        // [ property ]
  void RemoveExcludeTag(const char* szTag);                        // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo);                      // [ property ]
  bool GetShowDebugInfo() const;                                   // [ property ]

  void SetShowMipMaps(bool bShowMipMaps);                          // [ property ]
  bool GetShowMipMaps() const;                                     // [ property ]

  void SetCubeMapFile(ezStringView sFile);                         // [ property ]
  ezStringView GetCubeMapFile() const;                             // [ property ]

  ezTextureCubeResourceHandle GetCubeMap() const
  {
    return m_hCubeMap;
  }

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void SetNearPlane(float fNearPlane);                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; }   // [ property ]
  void SetFarPlane(float fFarPlane);                         // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnTransformChanged(ezMsgTransformChanged& msg);

  ezReflectionProbeDesc m_Desc;
  ezTextureCubeResourceHandle m_hCubeMap;

  ezReflectionProbeId m_Id;

  mutable bool m_bStatesDirty = true;
};
