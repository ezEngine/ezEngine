#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

using ezColorAnimationComponentManager = ezComponentManagerSimple<class ezColorAnimationComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Samples a color gradient and sends an ezMsgSetColor to the object it is attached to
///
/// The color gradient is sampled linearly over time.
/// This can be used to animate the color of a light source or mesh.
class EZ_GAMEENGINE_DLL ezColorAnimationComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezColorAnimationComponent, ezComponent, ezColorAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent
public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezColorAnimationComponent
public:
  ezColorAnimationComponent();

  /// \brief How long it takes to sample the entire color gradient.
  ezTime m_Duration;                                                                                     // [ property ]

  void SetColorGradient(const ezColorGradientResourceHandle& hResource);                                 // [ property ]
  EZ_ALWAYS_INLINE const ezColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; } // [ property ]

  /// \brief How the animation should be played and looped.
  ezEnum<ezPropertyAnimMode> m_AnimationMode; // [ property ]

  /// \brief How the color should be applied to the target.
  ezEnum<ezSetColorMode> m_SetColorMode; // [ property ]

  bool GetApplyRecursive() const;        // [ property ]
  void SetApplyRecursive(bool value);    // [ property ]

  bool GetRandomStartOffset() const;     // [ property ]
  void SetRandomStartOffset(bool value); // [ property ]

protected:
  void Update();

  ezTime m_CurAnimTime;
  ezColorGradientResourceHandle m_hGradient;
};
