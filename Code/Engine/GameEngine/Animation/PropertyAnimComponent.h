#pragma once

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/GameEngineDLL.h>
struct ezMsgSetPlaying;

using ezPropertyAnimComponentManager = ezComponentManagerSimple<class ezPropertyAnimComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Animates properties on other objects and components according to the property animation resource
///
/// Notes:
///  - There is no messages to change speed, simply modify the speed property.
class EZ_GAMEENGINE_DLL ezPropertyAnimComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPropertyAnimComponent, ezComponent, ezPropertyAnimComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPropertyAnimComponent

public:
  ezPropertyAnimComponent();
  ~ezPropertyAnimComponent();

  void SetPropertyAnim(const ezPropertyAnimResourceHandle& hResource);                                     // [ property ]
  EZ_ALWAYS_INLINE const ezPropertyAnimResourceHandle& GetPropertyAnim() const { return m_hPropertyAnim; } // [ property ]

  /// \brief Sets the animation playback range and resets the playing position to the range start position. Also activates the component if it isn't.
  void PlayAnimationRange(ezTime rangeLow, ezTime rangeHigh); // [ scriptable ]

  /// \brief Pauses or resumes animation playback. Does not reset any state.
  void OnMsgSetPlaying(ezMsgSetPlaying& ref_msg);                       // [ msg handler ]

  ezEnum<ezPropertyAnimMode> m_AnimationMode;                           // [ property ]
  ezTime m_RandomOffset;                                                // [ property ]
  float m_fSpeed = 1.0f;                                                // [ property ]
  ezTime m_AnimationRangeLow;                                           // [ property ]
  ezTime m_AnimationRangeHigh;                                          // [ property ]
  bool m_bPlaying = true;                                               // [ property ]

protected:
  ezEventMessageSender<ezMsgAnimationReachedEnd> m_ReachedEndMsgSender; // [ event ]
  ezEventMessageSender<ezMsgGenericEvent> m_EventTrackMsgSender;        // [ event ]

  struct Binding
  {
    const ezAbstractMemberProperty* m_pMemberProperty = nullptr;
    mutable void* m_pObject = nullptr; // needs to be updated in case components / objects get relocated in memory
  };

  struct FloatBinding : public Binding
  {
    const ezFloatPropertyAnimEntry* m_pAnimation[4] = {nullptr, nullptr, nullptr, nullptr};
  };

  struct ComponentFloatBinding : public FloatBinding
  {
    ezComponentHandle m_hComponent;
  };

  struct GameObjectBinding : public FloatBinding
  {
    ezGameObjectHandle m_hObject;
  };

  struct ColorBinding : public Binding
  {
    ezComponentHandle m_hComponent;
    const ezColorPropertyAnimEntry* m_pAnimation = nullptr;
  };

  void Update();
  void CreatePropertyBindings();
  void CreateGameObjectBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pRtti, void* pObject, const ezGameObjectHandle& hGameObject);
  void CreateFloatPropertyBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pRtti, void* pObject, const ezComponentHandle& hComponent);
  void CreateColorPropertyBinding(const ezColorPropertyAnimEntry* pAnim, const ezRTTI* pRtti, void* pObject, const ezComponentHandle& hComponent);
  void ApplyAnimations(const ezTime& tDiff);
  void ApplyFloatAnimation(const FloatBinding& binding, ezTime lookupTime);
  void ApplySingleFloatAnimation(const FloatBinding& binding, ezTime lookupTime);
  void ApplyColorAnimation(const ColorBinding& binding, ezTime lookupTime);
  ezTime ComputeAnimationLookup(ezTime tDiff);
  void EvaluateEventTrack(ezTime startTime, ezTime endTime);
  void StartPlayback();

  bool m_bReverse = false;

  ezTime m_AnimationTime;
  ezHybridArray<GameObjectBinding, 4> m_GoFloatBindings;
  ezHybridArray<ComponentFloatBinding, 4> m_ComponentFloatBindings;
  ezHybridArray<ColorBinding, 4> m_ColorBindings;
  ezPropertyAnimResourceHandle m_hPropertyAnim;

  // we do not want to recreate the binding when the resource changes at runtime
  // therefore we use a sharedptr to keep the data around as long as necessary
  // otherwise that would lead to weird state, because the animation would be interrupted at some point
  // and then the new animation would start from there
  // e.g. when the position is animated, objects could jump around the level
  // when the animation resource is reloaded
  // instead we go with one animation state until this component is reset entirely
  // that means you need to restart a level to see the updated animation
  ezSharedPtr<ezPropertyAnimResourceDescriptor> m_pAnimDesc;
};
