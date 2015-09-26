#pragma once

/// \file

#include <Foundation/Communication/Message.h>
#include <Foundation/Reflection/Reflection.h>

#include <Core/World/Declarations.h>

/// \brief Base class of all component types.
///
/// Derive from this class to implement custom component types. Also add the EZ_DECLARE_COMPONENT_TYPE macro to your class declaration.
/// Also add a EZ_BEGIN_COMPONENT_TYPE/EZ_END_COMPONENT_TYPE block to a cpp file. In that block you can add reflected members or message handlers.
/// Note that every component type needs a corresponding manager type. Take a look at ezComponentManagerSimple for a simple manager 
/// implementation that calls an update method on its components every frame.
/// To create a component instance call CreateComponent on the corresponding manager. Never store a direct pointer to a component but store a 
/// component handle instead.
class EZ_CORE_DLL ezComponent : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponent);

protected:
  /// \brief Keep the constructor private or protected in derived classes, so it cannot be called manually.
  ezComponent();
  virtual ~ezComponent();

public:
  /// \brief Returns whether this component is dynamic and thus can only be attached to dynamic game objects.
  bool IsDynamic() const;

  /// \brief Sets the active state of the component. Note that it is up to the manager if he differentiates between active and inactive components.
  void SetActive(bool bActive);

  /// \brief Activates the component. Note that it is up to the manager if he differentiates between active and inactive components.
  void Activate();

  /// \brief Deactivates the component.
  void Deactivate();

  /// \brief Returns whether this component is active.
  bool IsActive() const;

  /// \brief Returns the corresponding manager for this component.
  ezComponentManagerBase* GetManager() const;

  /// \brief Returns the owner game object if the component is attached to one or nullptr.
  ezGameObject* GetOwner();

  /// \brief Returns the owner game object if the component is attached to one or nullptr.
  const ezGameObject* GetOwner() const;

  /// \brief Returns a handle to this component.
  ezComponentHandle GetHandle() const;

  /// \brief Returns the type id corresponding to this component type.
  static ezUInt16 TypeId();

  /// \brief Gets the next component id for a new type. Internal use only.
  static ezUInt16 GetNextTypeId();

  ezUInt32 m_uiEditorPickingID;

protected:
  friend class ezWorld;
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  template <typename T>
  ezComponentHandle GetHandle() const;

  ezBitflags<ezObjectFlags> m_Flags;

private:
  virtual ezUInt16 GetTypeId() const = 0;

  /// \brief This method is called after the constructor. A derived type can override this method to do common initialization work.
  virtual ezResult Initialize();

  /// \brief This method is called before the destructor. A derived type can override this method to do common deinitialization work.
  virtual ezResult Deinitialize();

  /// \brief Returns whether this component is initialized. Internal method.
  bool IsInitialized() const;

  /// \brief This method is called when the component is attached to a game object. At this point the owner pointer is already set. A derived type can override this method to do additional work.
  virtual void OnAfterAttachedToObject();

  /// \brief This method is called when the component is detached from a game object. At this point the owner pointer is still set. A derived type can override this method to do additional work.
  virtual void OnBeforeDetachedFromObject();

  void OnMessage(ezMessage& msg);
  void OnMessage(ezMessage& msg) const;

  ezGenericComponentId m_InternalId;

  ezComponentManagerBase* m_pManager;
  ezGameObject* m_pOwner;

  static ezUInt16 TYPE_ID;
  static ezUInt16 s_uiNextTypeId;
};

#include <Core/World/Implementation/Component_inl.h>

/// \brief Add this macro to a custom component type inside the type declaration.
#define EZ_DECLARE_COMPONENT_TYPE(componentType, managerType) \
  EZ_ADD_DYNAMIC_REFLECTION(componentType); \
  public: \
    ezComponentHandle GetHandle() const; \
    managerType* GetManager() const; \
    EZ_FORCE_INLINE ezWorld* GetWorld() const { return GetManager()->GetWorld(); } \
    virtual ezUInt16 GetTypeId() const override { return TYPE_ID; } \
    static EZ_FORCE_INLINE ezUInt16 TypeId() { return TYPE_ID; } \
    static ezComponentHandle CreateComponent(ezWorld* pWorld, componentType*& pComponent) { return pWorld->GetComponentManager<managerType>()->CreateComponent(pComponent); } \
  private: \
    friend managerType; \
    static ezUInt16 TYPE_ID;

/// \brief Implements rtti and component specific functionality. Add this macro to a cpp file.
///
/// \see EZ_BEGIN_DYNAMIC_REFLECTED_TYPE
#define EZ_BEGIN_COMPONENT_TYPE(componentType, baseType, version, managerType) \
  ezUInt16 componentType::TYPE_ID = ezComponent::GetNextTypeId(); \
  ezComponentHandle componentType::GetHandle() const { return ezComponent::GetHandle<componentType>(); } \
  managerType* componentType::GetManager() const { return static_cast<managerType*>(ezComponent::GetManager()); } \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, baseType, version, ezRTTINoAllocator);

/// \brief Ends the component implementation code block that was opened with EZ_BEGIN_COMPONENT_TYPE.
#define EZ_END_COMPONENT_TYPE EZ_END_DYNAMIC_REFLECTED_TYPE


