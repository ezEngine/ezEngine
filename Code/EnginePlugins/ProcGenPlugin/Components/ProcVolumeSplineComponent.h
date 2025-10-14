#pragma once

#include <ProcGenPlugin/Components/ProcVolumeComponent.h>

struct ezMsgSplineChanged;
struct ezMsgExtractRenderData;
struct ezDebugRendererLine;

using ezProcVolumeSplineComponentManager = ezComponentManager<class ezProcVolumeSplineComponent, ezBlockStorageType::Compact>;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeSplineComponent : public ezProcVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVolumeSplineComponent, ezProcVolumeComponent, ezProcVolumeSplineComponentManager);

public:
  ezProcVolumeSplineComponent();
  ~ezProcVolumeSplineComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void SetFalloff(float fFalloff);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnMsgSplineChanged(ezMsgSplineChanged& ref_msg);
  void OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;
  void OnMsgExtractVolumes(ezMsgExtractVolumes& ref_msg) const;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& ref_msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;

  ezUInt32 m_uiLastChangeCounter = 0;

  mutable ezDynamicArray<ezDebugRendererLine> m_DebugLines;
};
