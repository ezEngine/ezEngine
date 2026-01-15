#pragma once

#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>

using ezRmlUiCanvas2DComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas2DComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact, ezWorldUpdatePhase::PostTransform>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas2DComponent : public ezRmlUiCanvasComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, ezRmlUiCanvasComponentBase, ezRmlUiCanvas2DComponentManager);

public:
  ezRmlUiCanvas2DComponent();
  ~ezRmlUiCanvas2DComponent();

  ezRmlUiCanvas2DComponent& operator=(ezRmlUiCanvas2DComponent&& rhs);

  virtual void Deinitialize() override;
  virtual void OnActivated() override;

  void Update() final override;

  void SetOffset(const ezVec2I32& vOffset);                                   // [ property ]
  const ezVec2I32& GetOffset() const { return m_vOffset; }                    // [ property ]

  void SetSize(const ezVec2U32& vSize);                                       // [ property ]
  const ezVec2U32& GetSize() const { return m_vSize; }                        // [ property ]

  void SetAnchorPoint(const ezVec2& vAnchorPoint);                            // [ property ]
  const ezVec2& GetAnchorPoint() const { return m_vAnchorPoint; }             // [ property ]

  void SetPassInput(bool bPassInput);                                         // [ property ]
  bool GetPassInput() const { return m_bPassInput; }                          // [ property ]

  void SetCustomScale(float fScale);                                          // [ property ]
  float GetCustomScale() const { return m_fCustomScale; }                     // [ property ]

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const override;
  bool UpdateSizeOffsetAndTexture(ezVec2& out_viewSize);

  float m_fCustomScale = 1.0f;
  ezVec2I32 m_vOffset = ezVec2I32::MakeZero();
  ezVec2 m_vAnchorPoint = ezVec2::MakeZero();
  bool m_bPassInput = true;

  ezVec2 m_vFinalOffset = ezVec2::MakeZero();
  ezGALTextureHandle m_hTexture;
};
