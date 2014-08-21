#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/Map.h>

/// \brief This needs to be put into the class declaration of EVERY dynamically reflectable class.
///
/// This macro extends a class, such that it is now able to return its own type information via GetDynamicRTTI(),
/// which is a virtual function, that is reimplemented on each type. A class needs to be derived from ezReflectedClass
/// (at least indirectly) for this.
#define EZ_ADD_DYNAMIC_REFLECTION(SELF)                               \
  EZ_ALLOW_PRIVATE_PROPERTIES(SELF);                                  \
  public:                                                             \
    EZ_FORCE_INLINE static const ezRTTI* GetStaticRTTI()              \
    {                                                                 \
      return &SELF::s_RTTI;                                           \
    }                                                                 \
    virtual const ezRTTI* GetDynamicRTTI() const                      \
    {                                                                 \
      return &SELF::s_RTTI;                                           \
    }                                                                 \
  private:                                                            \
    static ezRTTI s_RTTI;

/// \brief Implements the necessary functionality for a type to be dynamically reflectable.
///
/// \param Type
///   The type for which the reflection functionality should be implemented.
/// \param BaseType
///   The base class type of \a Type. If it has no base class, pass ezNoBase
/// \param AllocatorType
///   The type of an ezRTTIAllocator that can be used to create and destroy instances
///   of \a Type. Pass ezRTTINoAllocator for types that should not be created dynamically.
///   Pass ezRTTIDefaultAllocator<Type> for types that should be created on the default heap.
///   Pass a custom ezRTTIAllocator type to handle allocation differently.
#define EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, BaseType, Version, AllocatorType)  \
  EZ_RTTIINFO_DECL(Type, BaseType, Version)                                      \
  ezRTTI Type::s_RTTI = ezRTTInfo_##Type::GetRTTI();                             \
  EZ_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, AllocatorType)

/// \brief Ends the reflection code block that was opened with EZ_BEGIN_DYNAMIC_REFLECTED_TYPE.
#define EZ_END_DYNAMIC_REFLECTED_TYPE()                             \
    return ezRTTI(GetTypeName(),                                    \
      ezGetStaticRTTI<OwnBaseType>(),                               \
      sizeof(OwnType),                                              \
      GetTypeVersion(),                                             \
      ezVariant::TypeDeduction<ezReflectedClass*>::value,           \
      &Allocator, Properties, MessageHandlers);                     \
  }

class ezReflectedClassSerializationContext;

/// \brief All classes that should be dynamically reflectable, need to be derived from this base class.
///
/// The only functionality that this class provides is the GetDynamicRTTI() function.
class EZ_FOUNDATION_DLL ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReflectedClass);
public:
  EZ_FORCE_INLINE ezReflectedClass()
  {
  }

  virtual ~ezReflectedClass() {}

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  EZ_FORCE_INLINE bool IsInstanceOf(const ezRTTI* pType)
  {
    return GetDynamicRTTI()->IsDerivedFrom(pType);
  }

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  template<typename T>
  EZ_FORCE_INLINE bool IsInstanceOf()
  {
    return GetDynamicRTTI()->IsDerivedFrom<T>();
  }

  /// \brief This function is called to serialize the instance, when it is written via << to a stream.
  ///
  /// It should be overridden by deriving classes. In general each overridden version should always call the
  /// function of the base class. Only classes directly derived from ezReflectedClass must not do this, due to the assert in the
  /// base implementation.
  ///
  /// The serialization context can be used to ensure reading/writing of RTTI version information.
  /// This is not necessary for classes derived from ezReflectedClass (already done automatically), but for static reflected
  /// struct types, you need to call ezReflectedClassSerializationContext::WriteRttiVersion manually.
  virtual void Serialize(ezStreamWriterBase& stream, ezReflectedClassSerializationContext& context) const
  {
    EZ_REPORT_FAILURE("Serialize is not overridden by deriving class.");
  }

  /// \brief This function is called to deserialize the instance, when it is read via >> from a stream.
  ///
  /// It should be overridden by deriving classes. In general each overridden version should always call the
  /// function of the base class. Only classes directly derived from ezReflectedClass must not do this, due to the assert in the
  /// base implementation.
  ///
  /// The serialization context can be used to ensure reading/writing of RTTI version information.
  /// This is not necessary for classes derived from ezReflectedClass (already done automatically), but for static reflected
  /// struct types, you need to call ezReflectedClassSerializationContext::ReadRttiVersion manually.
  ///
  /// During deserialization you should call ezReflectedClassSerializationContext::GetStoredTypeVersion to find out
  /// with which version the instance was written to the stream, so that you know how to deserialize it.
  virtual void Deserialize(ezStreamReaderBase& stream, ezReflectedClassSerializationContext& context)
  {
    EZ_REPORT_FAILURE("Deserialize is not overridden by deriving class.");
  }
};

/// \brief A serialization context that ensures that version information is stored for each ezReflectedClass type.
///
/// During serialization this serialization context stores the RTTI version number for each type, the first time it is
/// written to the stream.
/// During deserialization, it makes sure that for each type read from the stream the version number, with which it was written, is available.
/// The info is only written once, so no space is wasted when many instances are stored.
///
/// For serialization, the context calls ezReflectedClass::Serialize().
/// For deserialization, the context calls ezReflectedClass::Deserialize() and passes itself along. The deserialization function
/// should then call GetStoredTypeVersion() with its own type to find out which version was used to write the data.
class EZ_FOUNDATION_DLL ezReflectedClassSerializationContext : public ezSerializationContext<ezReflectedClassSerializationContext>
{
public:
  /// \brief The constructor takes one reader and one writer stream to which this serialization context shall be registered. Both may be NULL.
  ezReflectedClassSerializationContext(ezStreamReaderBase* pReader, ezStreamWriterBase* pWriter) // [tested]
  {
    RegisterReaderStream(pReader);
    RegisterWriterStream(pWriter);
  }

  /// \brief Returns the version with which the given type was stored. Used to know during deserialization, how to read the data.
  /// Asserts, if the given type is unknown at this point during deserialization.
  ezUInt32 GetStoredTypeVersion(const ezRTTI* pRtti) const; // [tested]

  /// \brief Returns the version with which the given type was stored. Used to know during deserialization, how to read the data.
  /// Asserts, if the given type is unknown at this point during deserialization.
  template<typename TYPE>
  ezUInt32 GetStoredTypeVersion() const // [tested]
  {
    return GetStoredTypeVersion(ezGetStaticRTTI<TYPE>());
  }

  /// \brief Ensures that the version number for the given type (and all base classes) is written to the stream.
  /// This function must be called in conjunction with ReadRttiVersion !
  ///
  /// If it was already written before, the data is not written again.
  /// It is not necessary to call this manually for types derived from ezReflectedClass, as the serialization operators will already
  /// do this. However, when manually serializing static reflected types (structs), this needs to be done.
  void WriteRttiVersion(ezStreamWriterBase& stream, const ezRTTI* pRtti); // [tested]

  /// \brief Ensures that the version number for the given type (and all base classes) is written to the stream.
  /// This function must be called in conjunction with ReadRttiVersion !
  ///
  /// If it was already written before, the data is not written again.
  /// It is not necessary to call this manually for types derived from ezReflectedClass, as the serialization operators will already
  /// do this. However, when manually serializing static reflected types (structs), this needs to be done.
  template<typename TYPE>
  void WriteRttiVersion(ezStreamWriterBase& stream) // [tested]
  {
    WriteRttiVersion(stream, ezGetStaticRTTI<TYPE>());
  }

  /// \brief Ensures that the version number for the given type (and all base classes) is read from the stream.
  /// This function must be called in conjunction with WriteRttiVersion !
  ///
  /// If it was already read before, the data is not read again.
  /// It is not necessary to call this manually for types derived from ezReflectedClass, as the serialization operators will already
  /// do this. However, when manually serializing static reflected types (structs), this needs to be done.
  void ReadRttiVersion(ezStreamReaderBase& stream, const ezRTTI* pRtti); // [tested]

  /// \brief Ensures that the version number for the given type (and all base classes) is read from the stream.
  /// This function must be called in conjunction with WriteRttiVersion !
  ///
  /// If it was already read before, the data is not read again.
  /// It is not necessary to call this manually for types derived from ezReflectedClass, as the serialization operators will already
  /// do this. However, when manually serializing static reflected types (structs), this needs to be done.
  template<typename TYPE>
  void ReadRttiVersion(ezStreamReaderBase& stream) // [tested]
  {
    ReadRttiVersion(stream, ezGetStaticRTTI<TYPE>());
  }

private:

  void Write(ezStreamWriterBase& stream, const ezReflectedClass& Type);
  void Read(ezStreamReaderBase& stream, ezReflectedClass& Type);

  ezSet<const ezRTTI*> m_Written;
  ezMap<const ezRTTI*, ezUInt32> m_Read;

  EZ_MAKE_SERIALIZATION_CONTEXT_OPERATORS_FRIENDS(ezReflectedClass&);
};

// adds the << and >> operators to serialize types derived from ezReflectedClass. The serialization will be redirected to ezReflectedClass::Serialize and ezReflectedClass::Deserialize.
EZ_ADD_SERIALIZATION_CONTEXT_OPERATORS(ezReflectedClassSerializationContext, ezReflectedClass&);