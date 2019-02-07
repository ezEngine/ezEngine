#pragma once

#include <SampleGamePlugin/SampleGamePluginDLL.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>

typedef ezComponentManagerSimple<class ScriptTestComponent, ezComponentUpdateType::WhenSimulating> ScriptTestComponentManager;

class ScriptTestComponent;
class ezScriptMemberProperty;

//////////////////////////////////////////////////////////////////////////
class ScriptContainerBase : public ezReflectedClass
{
  // Expand the following to override GetDynamicRTTI behavior.
  //EZ_ADD_DYNAMIC_REFLECTION(ScriptContainerBase, ezReflectedClass);
  EZ_ALLOW_PRIVATE_PROPERTIES(ScriptContainerBase);
public:
  typedef ezReflectedClass SUPER;
  EZ_ALWAYS_INLINE static const ezRTTI* GetStaticRTTI()
  {
    return &ScriptContainerBase::s_RTTI;
  }
  virtual const ezRTTI* GetDynamicRTTI() const override
  {
    return m_pType ? m_pType : &ScriptContainerBase::s_RTTI;
  }
private:
  static ezRTTI s_RTTI;
  EZ_REFLECTION_DEBUG_CODE
public:
  ScriptContainerBase();

private:
  friend struct ezScriptContainerAllocator;
  friend class ScriptTestComponent;
  friend class ezScriptMemberProperty;

  const ezRTTI* m_pType = nullptr;
  ScriptTestComponent* m_pComponent = nullptr;
};

//////////////////////////////////////////////////////////////////////////
struct ezScriptContainerAllocator : public ezRTTIAllocator
{
  ezScriptContainerAllocator(const ezRTTI* pType) : m_pType(pType) {}
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocatorBase* pAllocator) override
  {
    auto pScriptContainer = EZ_DEFAULT_NEW(ScriptContainerBase);
    pScriptContainer->m_pType = m_pType;
    return pScriptContainer;
  }

  virtual void Deallocate(void* pObject, ezAllocatorBase* pAllocator) override // [tested]
  {
    ScriptContainerBase* pPointer = static_cast<ScriptContainerBase*>(pObject);
    EZ_DEFAULT_DELETE(pPointer);
  }
  const ezRTTI* m_pType = nullptr;
};

//////////////////////////////////////////////////////////////////////////
class ScriptTestComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ScriptTestComponent, ezComponent, ScriptTestComponentManager);

public:
  ScriptTestComponent();
  ~ScriptTestComponent();

  void SetScript(const char* szScriptPath);
  const char* GetScript() const;

  void SetScriptContainer(ScriptContainerBase* pContainer);
  ScriptContainerBase* GetScriptContainer() const;

  void SetProperty(const ezScriptMemberProperty* pProp, void* pValue);
  void GetProperty(const ezScriptMemberProperty* pProp, void* pValue) const;

  virtual void OnSimulationStarted() override;
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

private:
  //////////////////////////////////////////////////////////////////////////
  /// Properties

  ezString m_sScriptPath;
  ScriptContainerBase* m_pScriptContainer = nullptr;


  virtual void Initialize() override;
};
