#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgExtractRenderData;

typedef ezComponentManager<class ezDebugTextComponent, ezBlockStorageType::Compact> ezDebugTextComponentManager;

/// \brief This component prints debug text at the owner object's position.
class EZ_RENDERERCORE_DLL ezDebugTextComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDebugTextComponent, ezComponent, ezDebugTextComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent
public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezDebugTextComponent
public:
  ezDebugTextComponent();
  ~ezDebugTextComponent();

  ezString m_sText;       // [ property ]
  ezColorGammaUB m_Color; // [ property ]

  float m_fValue0; // [ property ]
  float m_fValue1; // [ property ]
  float m_fValue2; // [ property ]
  float m_fValue3; // [ property ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const; // [ msg handler ]
};
