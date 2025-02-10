#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgExtractRenderData;

using ezDebugTextComponentManager = ezComponentManager<class ezDebugTextComponent, ezBlockStorageType::Compact>;

/// \brief This component prints debug text at the owner object's position.
class EZ_RENDERERCORE_DLL ezDebugTextComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDebugTextComponent, ezComponent, ezDebugTextComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent
public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezDebugTextComponent
public:
  ezDebugTextComponent();
  ~ezDebugTextComponent();

  ezString m_sText;                                               // [ property ]
  ezColorGammaUB m_Color;                                         // [ property ]

  float m_fValue0 = 0.0f;                                         // [ property ]
  float m_fValue1 = 0.0f;                                         // [ property ]
  float m_fValue2 = 0.0f;                                         // [ property ]
  float m_fValue3 = 0.0f;                                         // [ property ]

  float m_fMaxDistance = 10.0f;                                   // [ property ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const; // [ msg handler ]
};
