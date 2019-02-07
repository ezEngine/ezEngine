#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <Core/World/World.h>

struct ezMsgExtractRenderData;

typedef ezComponentManager<class ezDebugTextComponent, ezBlockStorageType::Compact> ezDebugTextComponentManager;

/// \brief This component prints debug text at the owner object's position.
class EZ_RENDERERCORE_DLL ezDebugTextComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDebugTextComponent, ezComponent, ezDebugTextComponentManager);

public:
  ezDebugTextComponent();
  ~ezDebugTextComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  // ************************************* PROPERTIES ***********************************

  ezString m_sText;

  float m_fValue0;
  float m_fValue1;
  float m_fValue2;
  float m_fValue3;

  ezColorGammaUB m_Color;
};

