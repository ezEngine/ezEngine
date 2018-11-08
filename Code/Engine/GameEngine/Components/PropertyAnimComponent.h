#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <Foundation/Types/SharedPtr.h>
#include <Core/Messages/EventMessage.h>

struct ezMsgSetPlaying;

typedef ezComponentManagerSimple<class ezPropertyAnimComponent, ezComponentUpdateType::WhenSimulating> ezPropertyAnimComponentManager;

/// \brief Sent when a ezPropertyAnimComponent reaches the end of the property animation (either forwards or backwards playing)
///
/// This is sent regardless of whether the animation is played once, looped or back and forth.
struct EZ_GAMEENGINE_DLL ezMsgPropertyAnimationEndReached : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPropertyAnimationEndReached, ezEventMessage);

};

struct EZ_GAMEENGINE_DLL ezMsgPropertyAnimationPlayRange : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPropertyAnimationPlayRange, ezMessage);

  ezTime m_RangeLow;
  ezTime m_RangeHigh;
};

/// \brief Animates properties on other objects and components according to the property animation resource
///
/// Notes:
///  - There is no messages to change speed, simply modify the speed property.
class EZ_GAMEENGINE_DLL ezPropertyAnimComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPropertyAnimComponent, ezComponent, ezPropertyAnimComponentManager);

public:
  ezPropertyAnimComponent();


  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface
  //

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:

  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPropertyAnimComponent interface
  //

public:

  void SetPropertyAnimFile(const char* szFile);
  const char* GetPropertyAnimFile() const;

  void SetPropertyAnim(const ezPropertyAnimResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezPropertyAnimResourceHandle& GetPropertyAnim() const { return m_hPropertyAnim; }

  /// \brief Sets the animation playback range and resets the playing position to the range start position. Also activates the component if it isn't.
  void OnPlayAnimationRange(ezMsgPropertyAnimationPlayRange& msg);

  /// \brief Pauses or resumes animation playback. Does not reset any state.
  void OnSetPlaying(ezMsgSetPlaying& msg);

  // public properties:
  ezEnum<ezPropertyAnimMode> m_AnimationMode;
  ezTime m_RandomOffset;
  float m_fSpeed = 1.0f;
  // for only playing a sub-set of the animation
  ezTime m_AnimationRangeLow;
  ezTime m_AnimationRangeHigh;
  
protected:
  ezEventMessageSender<ezMsgPropertyAnimationEndReached> m_ReachedEndMsgSender;
  ezEventMessageSender<ezMsgGenericEvent> m_EventTrackMsgSender;


protected:
  struct Binding
  {
    ezAbstractMemberProperty* m_pMemberProperty = nullptr;
    mutable void* m_pObject = nullptr; // needs to be updated in case components / objects get relocated in memory
  };

  struct FloatBinding : public Binding
  {
    const ezFloatPropertyAnimEntry* m_pAnimation[4] = { nullptr, nullptr, nullptr, nullptr };
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
  void ApplyFloatAnimation(const FloatBinding& binding, double lookupTime);
  void ApplySingleFloatAnimation(const FloatBinding& binding, double lookupTime);
  void ApplyColorAnimation(const ColorBinding& binding, double lookupTime);
  double ComputeAnimationLookup(ezTime tDiff);
  void EvaluateEventTrack(ezTime startTime, ezTime endTime);
  void StartPlayback();

  bool m_bPlaying = true;
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
  ezSharedPtr<ezPropertyAnimResourceDescriptor> m_AnimDesc;
};

