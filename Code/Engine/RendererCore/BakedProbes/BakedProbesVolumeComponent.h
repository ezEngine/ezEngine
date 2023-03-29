#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgUpdateLocalBounds;

using ezBakedProbesVolumeComponentManager = ezComponentManager<class ezBakedProbesVolumeComponent, ezBlockStorageType::Compact>;

class EZ_RENDERERCORE_DLL ezBakedProbesVolumeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBakedProbesVolumeComponent, ezComponent, ezBakedProbesVolumeComponentManager);

public:
  ezBakedProbesVolumeComponent();
  ~ezBakedProbesVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  const ezVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const ezVec3& vExtents);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;

private:
  ezVec3 m_vExtents = ezVec3(10.0f);
};
