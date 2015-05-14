Reflection (RTTI) Usage {#ReflectionUsage}
====================================================

The ezEngine reflection system allows you to inspect structs and classes at runtime. It is used primarily for communication with tools and serialization.
The reflection system is macro-based, meaning that it is not generated automatically but needs to be written manually for each type, member etc that needs to be known at runtime.


Types
-----------------------

There are four distinct types that can be represented by the reflection: classes, structs, enums and bitflags. Each is represented by the ezRTTI class that stores the type information.

___________
###Class###

Classes are separated into two types: dynamic and static reflected. Dynamic classes derive from ezReflectedClass which allows you to determine its type using ezReflectedClass::GetDynamicRTTI. So as long as you have a pointer to a ezReflectedClass you can access its type information.

A static reflected class does not derive from ezReflectedClass so it is not possible to get the RTTI information in a common way. However, if you know the type of a variable you can use the function ezGetStaticRTTI to retrieve the ezRTTI instance of a specific type. Alternatively, you can also search for a type by name using ezRTTI::FindTypeByName.

~~~{.cpp}
    ezReflectedClass* pTest = new ezDynamicTestClass;
    const ezRTTI* pRtti = pTest->GetDynamicRTTI();
    const ezRTTI* pRtti2 = ezGetStaticRTTI<ezDynamicTestClass>();
    const ezRTTI* pRtti3 = ezRTTI::FindTypeByName("ezDynamicTestClass");
~~~

Declaring a dynamic class involves deriving from ezReflectedClass, adding the EZ_ADD_DYNAMIC_REFLECTION(SELF) macro into the class body and adding a EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, BaseType, Version, AllocatorType) block into a compilation unit.

~~~{.cpp}
    //Header
    class ezDynamicTestClass : public ezReflectedClass
    {
      EZ_ADD_DYNAMIC_REFLECTION(ezDynamicTestClass);
    };
~~~

~~~{.cpp}
    //Cpp
    EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicTestClass, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezDynamicTestClass>);
    EZ_END_DYNAMIC_REFLECTED_TYPE();
~~~

Declaring a static class is very similar to declaring a dynamic class. However, you need to declare the type outside the class via EZ_DECLARE_REFLECTABLE_TYPE(Linkage, TYPE) and use EZ_BEGIN_STATIC_REFLECTED_TYPE(Type, BaseType, Version, AllocatorType) in a compilation unit. If a class has no base class, use the dummy class ezNoBase instead.

~~~{.cpp}  
    // Header
    class ezStaticTestClass
    {
    };
    EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezStaticTestClass);
~~~

~~~{.cpp}
    // Cpp
    EZ_BEGIN_STATIC_REFLECTED_TYPE(ezStaticTestClass, ezNoBase, 1, ezRTTIDefaultAllocator<ezStaticTestClass>);
    EZ_END_STATIC_REFLECTED_TYPE();
~~~
    
____________
###Struct###

Structs are identical to static reflected classes so you can use the exact same macros.
   
   
__________
###Enum###

Enums are limited to structured enums, i.e. those used by the ezEnum class. Declaration is similar to static classes, but you use EZ_BEGIN_STATIC_REFLECTED_ENUM(Type, Version) instead in the compilation unit code.

~~~{.cpp}
    // Header
    struct ezExampleEnum
    {
      typedef ezInt8 StorageType;
      enum Enum
      {
        Value1 = 1,          // normal value
        Value2 = -2,         // normal value
        Value3 = 4,          // normal value
        Default = Value1     // Default initialization value (required)
      };
    };
    EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleEnum);
~~~

~~~{.cpp}
    // Cpp
    EZ_BEGIN_STATIC_REFLECTED_ENUM(ezExampleEnum, 1)
      EZ_ENUM_CONSTANTS(ezExampleEnum::Value1, ezExampleEnum::Value2) 
      EZ_ENUM_CONSTANT(ezExampleEnum::Value3),
    EZ_END_STATIC_REFLECTED_ENUM();
~~~

The enum constants can either be declared via EZ_ENUM_CONSTANTS() or EZ_ENUM_CONSTANT(Value) inside the begin / end block of the enum declaration. An enum type can be identified by its base type which is always the dummy ezEnumBase.
    
    
______________
###Bitflags###

Bitflags are limited to structured bitflags, i.e. those used by the ezBitflags class. Declaration is similar to static classes, but you use EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(Type, Version) instead in the compilation unit code. 

~~~{.cpp}
    // Header
    struct ezExampleBitflags
    {
      typedef ezUInt64 StorageType;
      enum Enum : ezUInt64
      {
        Value1 = EZ_BIT(0),  // normal value
        Value2 = EZ_BIT(31), // normal value
        Value3 = EZ_BIT(63), // normal value
        Default = Value1     // Default initialization value (required)
      };

      struct Bits
      {
        StorageType Value1 : 1;
        StorageType Padding : 30;
        StorageType Value2 : 1;
        StorageType Padding2 : 31;
        StorageType Value3 : 1;
      };
    };
    EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleBitflags);
~~~

~~~{.cpp}
    // Cpp
    EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezExampleBitflags, 1)
      EZ_BITFLAGS_CONSTANTS(ezExampleBitflags::Value1, ezExampleBitflags::Value2) 
      EZ_BITFLAGS_CONSTANT(ezExampleBitflags::Value3),
    EZ_END_STATIC_REFLECTED_BITFLAGS();
~~~

The bitflags constants can either be declared via EZ_BITFLAGS_CONSTANTS() or EZ_BITFLAGS_CONSTANT(Value) inside the begin / end block of the bitflags declaration. A bitflags type can be identified by its base type which is always the dummy ezBitflagsBase.
  

  
Properties
-----------------------

Properties are the most important information in a type as they define the data inside it. The properties of a type can be accessed via ezRTTI::GetProperties. There are different categories of properties, each deriving from ezAbstractProperty. The type of property can be determined by calling ezAbstractProperty::GetCategory.
Properties are added via the property macros inside the EZ_BEGIN_PROPERTIES() / EZ_END_PROPERTIES() block of the type declaration like this:

~~~{.cpp}
    EZ_BEGIN_STATIC_REFLECTED_TYPE(ezStaticTestClass, ezNoBase, 1, ezRTTIDefaultAllocator<ezStaticTestClass>);
    EZ_BEGIN_PROPERTIES
      EZ_CONSTANT_PROPERTY("Constant", 5),
      EZ_MEMBER_PROPERTY("Member", m_fFloat),
      EZ_ACCESSOR_PROPERTY("MemberAccessor", GetInt, SetInt),
      EZ_ARRAY_MEMBER_PROPERTY("Array", m_Deque),
      EZ_ARRAY_ACCESSOR_PROPERTY("ArrayAccessor", GetCount, GetValue, SetValue, Insert, Remove),
      EZ_SET_MEMBER_PROPERTY("Set", m_SetMember),
      EZ_SET_ACCESSOR_PROPERTY("SetAccessor", GetSet, SetInsert, SetRemove),
    EZ_END_PROPERTIES
    EZ_END_STATIC_REFLECTED_TYPE();
~~~

###Constant###

Constants are declared via EZ_CONSTANT_PROPERTY(PropertyName, Value). The value is stored within the property so no instance of the class is necessary to access it. To access the constant, cast the property to ezAbstractConstantProperty and call ezAbstractConstantProperty::GetPropertyType to determine the constant type. Then either cast to ezTypedConstantProperty of the matching type, or if the type is not known to you at compile time, use ezAbstractConstantProperty::GetPropertyPointer to access its data.

____________
###Member###

There are two types of member properties, direct member properties and accessor properties. The first has direct access to the memory location of the property in the class while the later uses function to get and set the property's value.
Direct member properties are declared via EZ_MEMBER_PROPERTY(PropertyName, MemberName) while accessor properties are declared via EZ_ACCESSOR_PROPERTY(PropertyName, Getter, Setter). The getter and setter functions must have the following signature:

~~~{.cpp}
    Type GetterFunc() const;
    void SetterFunc(Type value);
~~~
      
Type can be decorated with const and reference but must be consistent between get and set function. The available macros are the following:
~~~{.cpp}
    EZ_MEMBER_PROPERTY("Member", m_fFloat1),
    EZ_MEMBER_PROPERTY_READ_ONLY("MemberRO", m_vProperty3),
    EZ_ACCESSOR_PROPERTY("MemberAccessor", GetInt, SetInt),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("MemberAccessorRO", GetInt),
~~~

To access an instance's member variable value, cast the property to ezAbstractMemberProperty and call ezAbstractMemberProperty::GetPropertyType to determine the member type. Then either cast to ezTypedMemberProperty of the matching type, or if the type is not known to you at compile time, use ezAbstractMemberProperty::GetPropertyPointer or ezAbstractMemberProperty::GetValuePtr and ezAbstractMemberProperty::SetValuePtr to access its data. The first solution will only return a valid pointer if the property is a direct member property.



___________
###Array###

Array properties are very similar to member properties, they just handle arrays instead of single values. Direct array properties are declared via EZ_ARRAY_MEMBER_PROPERTY(PropertyName, MemberName) while accessor array properties are declared via EZ_ARRAY_ACCESSOR_PROPERTY(PropertyName, GetCount, Getter, Setter, Insert, Remove). The accessor interface functions must have the following signature:

~~~{.cpp}
    ezUInt32 GetCount() const;
    Type GetValue(ezUInt32 uiIndex) const;
    void SetValue(ezUInt32 uiIndex, Type value);
    void Insert(ezUInt32 uiIndex, Type value);
    void Remove(ezUInt32 uiIndex);
~~~

The available macros are the following:  
~~~{.cpp}
    EZ_ARRAY_ACCESSOR_PROPERTY("ArrayAccessor", GetCount, GetValue, SetValue, Insert, Remove),
    EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("ArrayAccessorRO", GetCount, GetValue),
    EZ_ARRAY_MEMBER_PROPERTY("Hybrid", m_Hybrid),
    EZ_ARRAY_MEMBER_PROPERTY("Dynamic", m_Dynamic),
    EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("Deque", m_Deque),
~~~

To access an instance's array, cast the property to ezAbstractArrayProperty and call ezAbstractArrayProperty::GetElementType to determine the element type. From here you can use the various functions inside ezAbstractArrayProperty to manipulate an instance's array.


_________
###Set###

Set properties are very similar to member properties, they just handle sets instead of single values. Direct set properties are declared via EZ_SET_MEMBER_PROPERTY(PropertyName, MemberName) while accessor set properties are declared via EZ_SET_ACCESSOR_PROPERTY(PropertyName, GetValues, Insert, Remove). The accessor interface functions must have the following signature:

~~~{.cpp}
    void Insert(Type value);
    void Remove(Type value);
    Container<Type> GetValues() const;
~~~

The available macros are the following:  
~~~{.cpp}
    EZ_SET_ACCESSOR_PROPERTY("SetAccessor", GetValues, Insert, Remove),
    EZ_SET_ACCESSOR_PROPERTY_READ_ONLY("SetAccessorRO", GetValues),
    EZ_SET_MEMBER_PROPERTY("Set", m_SetMember),
    EZ_SET_MEMBER_PROPERTY_READ_ONLY("SetRO", m_SetMember),
~~~

To access an instance's set, cast the property to ezAbstractSetProperty and call ezAbstractSetProperty::GetElementType to determine the element type. From here you can use the various functions inside ezAbstractSetProperty to manipulate an instance's set.


Flags
-----------------------

Types as well as properties have flags that quickly let you determine the kind of type / property you are dealing with.
For types, ezRTTI::GetTypeFlags lets you access its ezTypeFlags::Enum flags which are automatically deduced from the type at compile time.

Properties can have flags as well, ezAbstractMemberProperty::GetFlags, ezAbstractArrayProperty::GetFlags and ezAbstractSetProperty::GetFlags let you access the ezPropertyFlags::Enum flags of the handled property type. The only difference here is that besides automatically deduced flags there are also user-defined flags that can be added during declaration of the property by using ezAbstractMemberProperty::AddFlags and the variants on the other property categories:
~~~{.cpp}
    EZ_ACCESSOR_PROPERTY("ArraysPtr", GetArrays, SetArrays)->AddFlags(ezPropertyFlags::PointerOwner),
~~~


Limitations
-----------------------

* No two types can share the same name.
* Each property name must be unique within its type.
* Nested classes cannot be declared directly, however, you can make a typedef to a nested class and declare the type via that.
* Only constants that are a basic type (i.e. can be stored inside an ezVariant) will be available to tools.
* A pointer to a type cannot be its own type, only exception to this is const char*.
