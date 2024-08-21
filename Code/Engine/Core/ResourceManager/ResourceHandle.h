#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// \brief If this is set to EZ_ON, stack traces are recorded for every resource handle.
///
/// This can be used to find the places that create resource handles but do not properly clean them up.
#define EZ_RESOURCEHANDLE_STACK_TRACES EZ_OFF

class ezResource;

template <typename T>
class ezResourceLock;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
EZ_CORE_DLL void IncreaseResourceRefCount(ezResource* pResource, const void* pOwner);
EZ_CORE_DLL void DecreaseResourceRefCount(ezResource* pResource, const void* pOwner);

#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
EZ_CORE_DLL void MigrateResourceRefCount(ezResource* pResource, const void* pOldOwner, const void* pNewOwner);
#else
EZ_ALWAYS_INLINE void MigrateResourceRefCount(ezResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
  EZ_IGNORE_UNUSED(pResource);
  EZ_IGNORE_UNUSED(pOldOwner);
  EZ_IGNORE_UNUSED(pNewOwner);
}
#endif

/// \brief The typeless implementation of resource handles. A typed interface is provided by ezTypedResourceHandle.
class EZ_CORE_DLL ezTypelessResourceHandle
{
public:
  EZ_ALWAYS_INLINE ezTypelessResourceHandle() = default;

  /// \brief [internal] Increases the refcount of the given resource.
  ezTypelessResourceHandle(ezResource* pResource);

  /// \brief Increases the refcount of the given resource
  EZ_ALWAYS_INLINE ezTypelessResourceHandle(const ezTypelessResourceHandle& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
    {
      IncreaseResourceRefCount(m_pResource, this);
    }
  }

  /// \brief Move constructor, no refcount change is necessary.
  EZ_ALWAYS_INLINE ezTypelessResourceHandle(ezTypelessResourceHandle&& rhs)
  {
    m_pResource = rhs.m_pResource;
    rhs.m_pResource = nullptr;

    if (m_pResource)
    {
      MigrateResourceRefCount(m_pResource, &rhs, this);
    }
  }

  /// \brief Releases any referenced resource.
  EZ_ALWAYS_INLINE ~ezTypelessResourceHandle() { Invalidate(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }

  /// \brief Clears any reference to a resource and reduces its refcount.
  void Invalidate();

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  ezUInt64 GetResourceIDHash() const;

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// If the handle is not valid, an empty string is returned.
  ezStringView GetResourceID() const;

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const ezTypelessResourceHandle& rhs);

  /// \brief Move operator, no refcount change is necessary.
  void operator=(ezTypelessResourceHandle&& rhs);

  /// \brief Checks whether the two handles point to the same resource.
  EZ_ALWAYS_INLINE bool operator==(const ezTypelessResourceHandle& rhs) const { return m_pResource == rhs.m_pResource; }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezTypelessResourceHandle& rhs) const { return m_pResource != rhs.m_pResource; }

  /// \brief For storing handles as keys in maps
  EZ_ALWAYS_INLINE bool operator<(const ezTypelessResourceHandle& rhs) const { return m_pResource < rhs.m_pResource; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator==(const ezResource* rhs) const { return m_pResource == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezResource* rhs) const { return m_pResource != rhs; }

  /// \brief Returns the type information of the resource or nullptr if the handle is invalid.
  const ezRTTI* GetResourceType() const;

protected:
  ezResource* m_pResource = nullptr;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class ezResourceManager;
  friend class ezResourceHandleWriteContext;
  friend class ezResourceHandleReadContext;
  friend class ezResourceHandleStreamOperations;
};

template <>
struct ezHashHelper<ezTypelessResourceHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezTypelessResourceHandle& value) { return ezHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  EZ_ALWAYS_INLINE static bool Equal(const ezTypelessResourceHandle& a, const ezTypelessResourceHandle& b) { return a == b; }
};

/// \brief The ezTypedResourceHandle controls access to an ezResource.
///
/// All resources must be referenced using ezTypedResourceHandle instances (instantiated with the proper resource type as the template
/// argument). You must not store a direct pointer to a resource anywhere. Instead always store resource handles. To actually access a
/// resource, use ezResourceManager::BeginAcquireResource and ezResourceManager::EndAcquireResource after you have finished using it.
///
/// ezTypedResourceHandle implements reference counting on resources. It also allows to redirect resources to fallback resources when they
/// are not yet loaded (if possible).
///
/// As long as there is one resource handle that references a resource, it is considered 'in use' and thus might not get unloaded.
/// So be careful where you store resource handles.
/// If necessary you can call Invalidate() to clear a resource handle and thus also remove the reference to the resource.
template <typename RESOURCE_TYPE>
class ezTypedResourceHandle
{
public:
  using ResourceType = RESOURCE_TYPE;

  /// \brief A default constructed handle is invalid and does not reference any resource.
  ezTypedResourceHandle() = default;

  /// \brief Increases the refcount of the given resource.
  explicit ezTypedResourceHandle(ResourceType* pResource)
    : m_hTypeless(pResource)
  {
  }

  /// \brief Increases the refcount of the given resource.
  ezTypedResourceHandle(const ezTypedResourceHandle<ResourceType>& rhs)
    : m_hTypeless(rhs.m_hTypeless)
  {
  }

  /// \brief Move constructor, no refcount change is necessary.
  ezTypedResourceHandle(ezTypedResourceHandle<ResourceType>&& rhs)
    : m_hTypeless(std::move(rhs.m_hTypeless))
  {
  }

  template <typename BaseOrDerivedType>
  ezTypedResourceHandle(const ezTypedResourceHandle<BaseOrDerivedType>& rhs)
    : m_hTypeless(rhs.m_hTypeless)
  {
    static_assert(std::is_base_of<ResourceType, BaseOrDerivedType>::value || std::is_base_of<BaseOrDerivedType, ResourceType>::value, "Only related types can be assigned to handles of this type");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (std::is_base_of<BaseOrDerivedType, ResourceType>::value)
    {
      EZ_ASSERT_DEBUG(rhs.IsValid(), "Cannot cast invalid base handle to derived type!");
      ezResourceLock<BaseOrDerivedType> lock(rhs, ezResourceAcquireMode::PointerOnly);
      EZ_ASSERT_DEBUG(ezDynamicCast<const ResourceType*>(lock.GetPointer()) != nullptr, "Types are not related!");
    }
#endif
  }

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const ezTypedResourceHandle<ResourceType>& rhs) { m_hTypeless = rhs.m_hTypeless; }

  /// \brief Move operator, no refcount change is necessary.
  void operator=(ezTypedResourceHandle<ResourceType>&& rhs) { m_hTypeless = std::move(rhs.m_hTypeless); }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_ALWAYS_INLINE bool operator==(const ezTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless == rhs.m_hTypeless; }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless != rhs.m_hTypeless; }

  /// \brief For storing handles as keys in maps
  EZ_ALWAYS_INLINE bool operator<(const ezTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless < rhs.m_hTypeless; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator==(const ezResource* rhs) const { return m_hTypeless == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezResource* rhs) const { return m_hTypeless != rhs; }


  /// \brief Returns the corresponding typeless resource handle.
  EZ_ALWAYS_INLINE operator const ezTypelessResourceHandle() const { return m_hTypeless; }

  /// \brief Returns the corresponding typeless resource handle.
  EZ_ALWAYS_INLINE operator ezTypelessResourceHandle() { return m_hTypeless; }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_hTypeless.IsValid(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_ALWAYS_INLINE explicit operator bool() const { return m_hTypeless.IsValid(); }

  /// \brief Clears any reference to a resource and reduces its refcount.
  EZ_ALWAYS_INLINE void Invalidate() { m_hTypeless.Invalidate(); }

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  EZ_ALWAYS_INLINE ezUInt64 GetResourceIDHash() const { return m_hTypeless.GetResourceIDHash(); }

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  EZ_ALWAYS_INLINE ezStringView GetResourceID() const { return m_hTypeless.GetResourceID(); }

  /// \brief Attempts to copy the given typeless handle to this handle.
  ///
  /// It is an error to assign a typeless handle that references a resource with a mismatching type.
  void AssignFromTypelessHandle(const ezTypelessResourceHandle& hHandle)
  {
    if (!hHandle.IsValid())
      return;

    EZ_ASSERT_DEV(hHandle.GetResourceType()->IsDerivedFrom<RESOURCE_TYPE>(), "Type '{}' does not match resource type '{}' in typeless handle.", ezGetStaticRTTI<RESOURCE_TYPE>()->GetTypeName(), hHandle.GetResourceType()->GetTypeName());

    m_hTypeless = hHandle;
  }

private:
  template <typename T>
  friend class ezTypedResourceHandle;

  // you must go through the resource manager to get access to the resource pointer
  friend class ezResourceManager;
  friend class ezResourceHandleWriteContext;
  friend class ezResourceHandleReadContext;
  friend class ezResourceHandleStreamOperations;

  ezTypelessResourceHandle m_hTypeless;
};

template <typename T>
struct ezHashHelper<ezTypedResourceHandle<T>>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezTypedResourceHandle<T>& value) { return ezHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  EZ_ALWAYS_INLINE static bool Equal(const ezTypedResourceHandle<T>& a, const ezTypedResourceHandle<T>& b) { return a == b; }
};

// Stream operations
class ezResource;

class EZ_CORE_DLL ezResourceHandleStreamOperations
{
public:
  template <typename ResourceType>
  static void WriteHandle(ezStreamWriter& inout_stream, const ezTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(inout_stream, hResource.m_hTypeless.m_pResource);
  }

  template <typename ResourceType>
  static void ReadHandle(ezStreamReader& inout_stream, ezTypedResourceHandle<ResourceType>& ref_hResourceHandle)
  {
    ReadHandle(inout_stream, ref_hResourceHandle.m_hTypeless);
  }

private:
  static void WriteHandle(ezStreamWriter& Stream, const ezResource* pResource);
  static void ReadHandle(ezStreamReader& Stream, ezTypelessResourceHandle& ResourceHandle);
};

/// \brief Operator to serialize resource handles
template <typename ResourceType>
void operator<<(ezStreamWriter& inout_stream, const ezTypedResourceHandle<ResourceType>& hValue)
{
  ezResourceHandleStreamOperations::WriteHandle(inout_stream, hValue);
}

/// \brief Operator to deserialize resource handles
template <typename ResourceType>
void operator>>(ezStreamReader& inout_stream, ezTypedResourceHandle<ResourceType>& ref_hValue)
{
  ezResourceHandleStreamOperations::ReadHandle(inout_stream, ref_hValue);
}

/// \brief Adds two member functions to a class, GetXyzFile() and SetXyzFile() with Xyz being equal to 'name', which allow to access the handle through strings.
///
/// This macro is just for convenience, so that one doesn't need to write this boilerplate code by hand for every resource handle that
/// should be exposed through the reflection system.
/// The accessors still need to be exposed to the reflection system like this:
///
/// EZ_ACCESSOR_PROPERTY("XyzResource", GetXyzFile, SetXyzFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Xyz")),
///
#define EZ_ADD_RESOURCEHANDLE_ACCESSORS(name, member)                                  \
  void Set##name##File(ezStringView sFile)                                             \
  {                                                                                    \
    if (!sFile.IsEmpty())                                                              \
    {                                                                                  \
      member = ezResourceManager::LoadResource<decltype(member)::ResourceType>(sFile); \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
      member = {};                                                                     \
    }                                                                                  \
  }                                                                                    \
                                                                                       \
  ezStringView Get##name##File() const                                                 \
  {                                                                                    \
    return member.GetResourceID();                                                     \
  }

/// \brief Same as EZ_ADD_RESOURCEHANDLE_ACCESSORS, but calls 'setterFunc' instead of assigning to 'member' directly.
///
/// This can be used, if the setter should do additional validation or bookkeeping.
#define EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(name, member, setterFunc)             \
  void Set##name##File(ezStringView sFile)                                                \
  {                                                                                       \
    if (!sFile.IsEmpty())                                                                 \
    {                                                                                     \
      setterFunc(ezResourceManager::LoadResource<decltype(member)::ResourceType>(sFile)); \
    }                                                                                     \
    else                                                                                  \
    {                                                                                     \
      setterFunc({});                                                                     \
    }                                                                                     \
  }                                                                                       \
                                                                                          \
  ezStringView Get##name##File() const                                                    \
  {                                                                                       \
    return member.GetResourceID();                                                        \
  }


/// \brief [internal] Helper class to generate accessor functions for (private) resource handle members
template <typename Class, typename Type, Type Class::*Member>
struct ezResourceHandlePropertyAccessor
{
  static ezStringView GetValue(const Class* pInstance) { return ((*pInstance).*Member).GetResourceID(); }

  static void SetValue(Class* pInstance, ezStringView value)
  {
    if (!value.IsEmpty())
    {
      (*pInstance).*Member = ezResourceManager::LoadResource<Type::ResourceType>(value);
    }
    else
    {
      (*pInstance).*Member = {};
    }
  }

  static void* GetPropertyPointer(const Class* pInstance)
  {
    EZ_IGNORE_UNUSED(pInstance);

    // No access to sub-properties
    return nullptr;
  }
};

/// \brief Similar to EZ_MEMBER_PROPERTY, but makes it convenient to expose resource handle properties
#define EZ_RESOURCE_MEMBER_PROPERTY(PropertyName, MemberName)                                                        \
  (new ezMemberProperty<OwnType, ezStringView>(PropertyName,                                                         \
    &ezResourceHandlePropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
    &ezResourceHandlePropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
    &ezResourceHandlePropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))



/// \brief [internal] An implementation of ezTypedMemberProperty that uses custom getter / setter functions to access a property.
template <typename Class, typename Type>
class ezResourceAccessorProperty : public ezTypedMemberProperty<ezStringView>
{
public:
  using RealType = ezStringView;
  using HandleType = typename ezTypeTraits<Type>::NonConstReferenceType;
  using ResourceType = typename HandleType::ResourceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  ezResourceAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : ezTypedMemberProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      ezAbstractMemberProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    EZ_IGNORE_UNUSED(pInstance);

    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual RealType GetValue(const void* pInstance) const override // [tested]
  {
    return (static_cast<const Class*>(pInstance)->*m_Getter)().GetResourceID();
  }

  virtual void SetValue(void* pInstance, RealType value) const override // [tested]
  {
    EZ_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());

    if (m_Setter)
    {
      if (!value.IsEmpty())
      {
        (static_cast<Class*>(pInstance)->*m_Setter)(ezResourceManager::LoadResource<ResourceType>(value));
      }
      else
      {
        (static_cast<Class*>(pInstance)->*m_Setter)({});
      }
    }
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};

#define EZ_RESOURCE_ACCESSOR_PROPERTY(PropertyName, Getter, Setter) \
  (new ezResourceAccessorProperty<OwnType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))
