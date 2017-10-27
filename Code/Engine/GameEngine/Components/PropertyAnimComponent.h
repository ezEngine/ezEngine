#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <Foundation/Types/SharedPtr.h>

typedef ezComponentManagerSimple<class ezPropertyAnimComponent, ezComponentUpdateType::WhenSimulating> ezPropertyAnimComponentManager;

class EZ_GAMEENGINE_DLL ezPropertyAnimComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPropertyAnimComponent, ezComponent, ezPropertyAnimComponentManager);

public:
  ezPropertyAnimComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetPropertyAnimFile(const char* szFile);
  const char* GetPropertyAnimFile() const;

  void SetPropertyAnim(const ezPropertyAnimResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezPropertyAnimResourceHandle& GetPropertyAnim() const { return m_hPropertyAnim; }


protected:
  void CreatePropertyBindings();
  void CreateFloatPropertyBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pRtti, void* pObject, const ezComponentHandle& hComponent);
  void CreateColorPropertyBinding(const ezColorPropertyAnimEntry* pAnim, const ezRTTI* pRtti, void* pObject, const ezComponentHandle& hComponent);
  void ApplyAnimations(const ezTime& tDiff);
  void ApplyFloatAnimation(ezUInt32 idx, double lookupTime);
  void ApplyColorAnimation(ezUInt32 idx, double lookupTime);
  double ComputeAnimationLookup(ezTime& inout_tCur, ezPropertyAnimMode::Enum mode, ezTime duration) const;

  struct Binding
  {
    ezAbstractMemberProperty* m_pMemberProperty;
    ezComponentHandle m_hComponent;
    void* m_pObject;
  };

  struct FloatBinding : Binding
  {
    const ezFloatPropertyAnimEntry* m_pAnimation;
  };

  struct ColorBinding : Binding
  {
    const ezColorPropertyAnimEntry* m_pAnimation;
  };

  ezTime m_AnimationTime;
  ezHybridArray<FloatBinding, 4> m_FloatBindings;
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
