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
/// The color gradient is samples linearly over time. This can be used to animate the color of a light source or mesh.
/// \todo Expose the ezSetColorMode of the ezMsgSetColor
/// \todo Add speed parameter
/// \todo Add loop mode (once, back-and-forth, loop)
/// \todo Add option to send message to whole sub-tree (SendMessageRecursive)
/// \todo Add on-finished (loop point) event
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

  ezTime m_Duration; // [ property ]

  void SetColorGradientFile(const char* szFile); // [ property ]
  const char* GetColorGradientFile() const;      // [ property ]

  void SetColorGradient(const ezColorGradientResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  ezEnum<ezPropertyAnimMode> m_AnimationMode; // [ property ]
  ezEnum<ezSetColorMode> m_SetColorMode;      // [ property ]

  bool GetApplyRecursive() const;     // [ property ]
  void SetApplyRecursive(bool value); // [ property ]

  bool GetRandomStartOffset() const;     // [ property ]
  void SetRandomStartOffset(bool value); // [ property ]

protected:
  void Update();

  ezTime m_CurAnimTime;
  ezColorGradientResourceHandle m_hGradient;
};
