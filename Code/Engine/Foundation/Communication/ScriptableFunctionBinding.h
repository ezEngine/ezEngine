#pragma once

/// \file

#include <Foundation/Communication/Message.h>
#include <Foundation/Strings/String.h>

/// \brief Macro to declare a member function that should be callable from scripts.
///
/// Pass the return type as the first parameter, and the function name as the second.
/// Then optionally pass in parameters as Type COMMA Name pairs.
///
/// Examples:
///   EZ_SCRIPTABLE_FUNCTION_DECL_VOID(void, DoStuff);
///   EZ_SCRIPTABLE_FUNCTION_DECL(float, GetStuff, ezInt32, param1);
///   EZ_SCRIPTABLE_FUNCTION_DECL(ezUInt8, GetMoreStuff, double, param1, const char*, param2);
///   EZ_SCRIPTABLE_FUNCTION_DECL_VOID(void, WithOutParams, double&, out_param1, int&, inout_param2);
///
/// You can use return values and non-const reference parameters to return values from the function.
/// For reference parameters to work, the parameter name must start with either "out_" or "inout_"
/// depending on whether it should be only an out value or also an in value.
///
/// Use EZ_SCRIPTABLE_FUNCTION_IMPL in the CPP file to implement the declared functionality.
///
/// Use EZ_SCRIPTABLE_FUNCTION_DECL_VOID to declare functions that do not return a value.
///
/// Details:
/// The macro will declare a message (derived from ezScriptFunctionMessage), a function to handle that message,
/// and a function with a regular signature "ReturnType FunctionName(Parameters)"
/// The message will be used as the glue to communicate from the script with the C++ function.
/// The message handler will be automatically registered. It unpacks incoming parameter values, forwards the call
/// to the regular function, and then packs inout and out parameter results back into the message.
///
/// \sa EZ_SCRIPTABLE_FUNCTION_DECL_VOID, EZ_SCRIPTABLE_FUNCTION_IMPL, EZ_SCRIPTABLE_FUNCTION_MSG_TYPE
#define EZ_SCRIPTABLE_FUNCTION_DECL(ReturnType, FuncName, ...) _EZ_SF_DECL(ReturnType, FuncName, __VA_ARGS__)

/// \brief Same as EZ_SCRIPTABLE_FUNCTION_DECL() but for functions that should return 'void'
#define EZ_SCRIPTABLE_FUNCTION_DECL_VOID(FuncName, ...) _EZ_SF_DECL_VOID(FuncName, __VA_ARGS__)

/// \brief Counter-part macro to EZ_SCRIPTABLE_FUNCTION_DECL() to insert in CPP files to implement the declared functionality
///
/// \sa EZ_SCRIPTABLE_FUNCTION_IMPL_VOID, EZ_SCRIPTABLE_FUNCTION_DECL, EZ_SCRIPTABLE_FUNCTION_MSG_TYPE
#define EZ_SCRIPTABLE_FUNCTION_IMPL(ReturnType, ClassName, FuncName, ...) _EZ_SF_IMPL(ReturnType, ClassName, FuncName, __VA_ARGS__)

/// \brief Same as EZ_SCRIPTABLE_FUNCTION_IMPL() but for functions that should return 'void'
#define EZ_SCRIPTABLE_FUNCTION_IMPL_VOID(ClassName, FuncName, ...) _EZ_SF_IMPL_VOID(ClassName, FuncName, __VA_ARGS__)

/// \brief Generates the full message class name for the given class's script function
///
/// Use this if you ever need to generate an instance of this message yourself, e.g. if you want to do a delayed function call.
///
/// \sa EZ_SCRIPTABLE_FUNCTION_DECL, EZ_SCRIPTABLE_FUNCTION_IMPL
#define EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName) ClassName::_EZ_SF_MSG_NAME(FuncName)

/// \brief Base class for all messages that were auto-generated for script function bindings
struct EZ_FOUNDATION_DLL ezScriptFunctionMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezScriptFunctionMessage, ezMessage);
};



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////// DANGER! HERE BE DRAGONS!!!
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// SERIOUSLY, DON'T SCROLL FURTHER IF YOU WANT TO KEEP YOUR SANITY
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// OK, WE START WITH SOME LIGHT READING
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



namespace ezInternal
{
  /// \internal Used to strip away "out_" and "inout_" from parameter names
  ///
  /// These name prefixes are used to detect whether a parameter should be exposed as an output or as both input and output to the function.
  /// If it has no prefix, it is only input.
  ///
  /// Additionally, for "reasons" (preprocessor parsing rules), we cannot just flag return values as reference parameters, so the function return
  /// value gets a special name, to flag it as such. To make it nicer, we remap it here as well.
  inline const char* SfRemoveParamPrefix(bool isNonConstReference, const char* szParamName)
  {
    if (ezStringUtils::IsEqual(szParamName, "_SfResult"))
      return "Result";

    // early out, we assert in SfGetParamFlags() that the user didn't use wrong combinations
    if (!isNonConstReference)
      return szParamName;

    if (ezStringUtils::StartsWith(szParamName, "out_"))
      return szParamName + 4;
    if (ezStringUtils::StartsWith(szParamName, "inout_"))
      return szParamName + 6;

    return szParamName;
  }

  /// \internal Used to detect ezPropertyFlags for the message properties, to tag out and inout parameters
  ///
  /// Again, the return value of the scriptable function gets special treatment, to tag it as an 'out' parameter even though it is not a reference.
  inline ezPropertyFlags::Enum SfGetParamFlags(bool isNonConstReference, const char* szParamName)
  {
    // the return value is always handled like an 'out' variable
    if (ezStringUtils::IsEqual(szParamName, "_SfResult"))
      return ezPropertyFlags::VarOut;

    if (ezStringUtils::StartsWith(szParamName, "out_"))
    {
      EZ_ASSERT_DEBUG(isNonConstReference, "Parameter '{}' has an 'out_' prefix, but is not a non-const reference type", szParamName);
      return ezPropertyFlags::VarOut;
    }

    if (ezStringUtils::StartsWith(szParamName, "inout_"))
    {
      EZ_ASSERT_DEBUG(isNonConstReference, "Parameter '{}' has an 'inout_' prefix, but is not a non-const reference type", szParamName);
      return ezPropertyFlags::VarInOut;
    }

    EZ_ASSERT_DEBUG(!isNonConstReference, "Parameter '{}' is a non-const reference type, but does not use an 'out_' or 'inout_' name prefix", szParamName);
    return ezPropertyFlags::Void;
  }

  /// \internal Template struct used to remap function parameter types to and from types that are actually supported by the script runtime
  ///
  /// The (Visual) Script runtime only supports a very limited set of types. E.g. all numbers are just doubles. Lua and JavaScript do the same.
  /// Thus, if we want to have a function exposed to the scripting system, but also want to use integers and maybe const char* for strings etc.
  /// we need to remap those types to a supported type.
  ///
  /// Template specializations of SfType declare the 'MappedType' typedef, which specifies which type to use in the script (or rather in the
  /// message that is exposed to the scripts).
  /// For instance, all int specializations map the int type to double.
  ///
  /// The struct then also defines to functions to cast back and forth between the original type and the mapped type.
  template <typename VarType>
  struct SfType
  {
  };

  // float -> double
  template <>
  struct SfType<float>
  {
    typedef float Type;
    typedef double MappedType;
    static float CastFromMapped(MappedType in) { return in; }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // double -> double
  template <>
  struct SfType<double>
  {
    typedef double Type;
    typedef double MappedType;
    static double CastFromMapped(MappedType in) { return in; }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // bool -> bool
  template <>
  struct SfType<bool>
  {
    typedef bool Type;
    typedef bool MappedType;
    static bool CastFromMapped(MappedType in) { return in; }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // ezInt32 -> double (+ rounding when casting back)
  template <>
  struct SfType<ezInt32>
  {
    typedef ezInt32 Type;
    typedef double MappedType;
    static ezInt32 CastFromMapped(MappedType in) { return static_cast<ezInt32>(ezMath::Round(in)); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // ezInt16 -> double (+ rounding when casting back)
  template <>
  struct SfType<ezInt16>
  {
    typedef ezInt16 Type;
    typedef double MappedType;
    static ezInt16 CastFromMapped(MappedType in) { return static_cast<ezInt16>(ezMath::Round(in)); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // ezInt8 -> double (+ rounding when casting back)
  template <>
  struct SfType<ezInt8>
  {
    typedef ezInt8 Type;
    typedef double MappedType;
    static ezInt8 CastFromMapped(MappedType in) { return static_cast<ezInt8>(ezMath::Round(in)); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // ezUInt32 -> double (+ rounding when casting back)
  template <>
  struct SfType<ezUInt32>
  {
    typedef ezUInt32 Type;
    typedef double MappedType;
    static ezUInt32 CastFromMapped(MappedType in) { return static_cast<ezUInt32>(ezMath::Round(in)); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // ezUInt16 -> double (+ rounding when casting back)
  template <>
  struct SfType<ezUInt16>
  {
    typedef ezUInt16 Type;
    typedef double MappedType;
    static ezUInt16 CastFromMapped(MappedType in) { return static_cast<ezUInt16>(ezMath::Round(in)); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // ezUInt8 -> double (+ rounding when casting back)
  template <>
  struct SfType<ezUInt8>
  {
    typedef ezUInt8 Type;
    typedef double MappedType;
    static ezUInt8 CastFromMapped(MappedType in) { return static_cast<ezUInt8>(ezMath::Round(in)); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // const char* -> ezString
  template <>
  struct SfType<const char*>
  {
    typedef const char* Type;
    typedef ezString MappedType;
    static const char* CastFromMapped(const MappedType& in) { return in.GetData(); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };

  // ezStringView\ -> ezString
  template <>
  struct SfType<ezStringView>
  {
    typedef ezStringView Type;
    typedef ezString MappedType;
    static ezStringView CastFromMapped(const MappedType& in) { return in.GetView(); }
    static MappedType CastToMapped(Type in) { return static_cast<MappedType>(in); }
  };
}; // namespace ezInternal

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// SOME HELPER MACROS TO MAKE LIFE LESS MISERABLE
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \internal Builds the name of the ezMessage that is used to bind a script function
#define _EZ_SF_MSG_NAME(FuncName) EZ_CONCAT(SfMsg_, FuncName)

/// \internal Builds the name of the message handler function, e.g. generates "OnMyFunc"
#define _EZ_SF_MSG_HANDLER_NAME(FuncName) EZ_CONCAT(On, FuncName)

/// \internal Builds the full name of the message handler function, e.g. generates "MyClass::OnMyFunc"
#define _EZ_SF_MSG_HANDLER_NAME_FULL(ClassName, FuncName) ClassName::_EZ_SF_MSG_HANDLER_NAME(FuncName)

/// \internal Builds the name of a temp variable, takes two parameters and ignores the first, because the macro is called with pairs of Type/Name
#define _EZ_SF_TMP_PARAM_NAME(Type, Name) EZ_CONCAT(Tmp, Name)

/// \internal When this macro is called, it simply outputs the values of the two parameters after each other
/// the only reason for this to exist is because it is called by another macro for all pairs of variadic parameters
#define _EZ_SF_DECL_FUNC_PARAM(Type, Name) Type Name

/// \internal Generates C++ code that detects whether a type is a reference type and not const
///
/// For this to work as expected, one needs to peel away the reference type and THEN check for const-ness,
/// otherwise a "const int&" will be reported as non-const, because technically the variable is not const, only the pointed to variable is,
/// but that is what we are more interested in
#define _EZ_SF_IS_NON_CONST_REF(Type) (std::is_reference<Type>::value) && (std::is_const<std::remove_reference<Type>::type>::value == false)

/// \internal Creates an instance of ezScriptFunctionRegistrar to auto-register the auto-generated message handler function
///
/// This is basically a copy of EZ_MESSAGE_HANDLER, just with minor adjustments to register our function
#define _EZ_SF_REGISTER(ClassName, FuncName)                                              \
  static ezInternal::ezScriptFunctionRegistrar<ClassName> EZ_CONCAT(ClassName, FuncName)( \
    new ezInternal::MessageHandler<EZ_IS_CONST_MESSAGE_HANDLER(                           \
      ClassName,                                                                          \
      EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName),                               \
      &_EZ_SF_MSG_HANDLER_NAME_FULL(ClassName, FuncName))>::Impl<ClassName, EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName), &_EZ_SF_MSG_HANDLER_NAME_FULL(ClassName, FuncName)>())



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// AND NOW THE TRULY HORRIFIC PART
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// \internal Implementation of EZ_SCRIPTABLE_FUNCTION_DECL
///
/// This macro first declares a message type for binding the function call.
/// It then declares a function to handle that message type, and finally another
/// function for the regular function call (what should be used from C++ directly).
///
/// This macro uses EZ_EXPAND_ARGS_PAIR and EZ_EXPAND_ARGS_PAIR_COMMA to expand all vararg parameters as pairs of (Type, Name).
/// It does so once inside the message body to declare all message members, and a second time in the declaration of the
/// regular function, to declare the function parameters.
///
/// The latter case is easy, we just use _EZ_SF_DECL_FUNC_PARAM to simply pass through all vararg parameter pairs into
/// the function declaration. The EZ_EXPAND_ARGS_PAIR_COMMA automatically inserts commas between each parameter type/name pair.
///
/// The former case is way more tricky, because we also want to insert the function return type as a message member, so that we
/// have a place where the return value can be written to (and then be picked up by the scripting system again and just handled
/// like an out parameter).
/// HOWEVER, the ReturnType may be 'void' and we cannot declare a void member!
/// The current work-around is to use a separate macro for this case, ie. _EZ_SF_DECL_VOID()
#define _EZ_SF_DECL(ReturnType, FuncName, ...)                                      \
  struct _EZ_SF_MSG_NAME(FuncName)                                                  \
    : public ezScriptFunctionMessage                                                \
  {                                                                                 \
    EZ_DECLARE_MESSAGE_TYPE(_EZ_SF_MSG_NAME(FuncName), ezScriptFunctionMessage);    \
    EZ_EXPAND_ARGS_PAIR(_EZ_SF_DECL_MSG_MEMBER, ReturnType, _SfResult, __VA_ARGS__) \
  };                                                                                \
  void _EZ_SF_MSG_HANDLER_NAME(FuncName)(_EZ_SF_MSG_NAME(FuncName) & msg);          \
  ReturnType FuncName(EZ_EXPAND_ARGS_PAIR_COMMA(_EZ_SF_DECL_FUNC_PARAM, __VA_ARGS__))

/// \internal Same as _EZ_SF_DECL() but with adjustments for the ReturnType being 'void'
#define _EZ_SF_DECL_VOID(FuncName, ...)                                          \
  struct _EZ_SF_MSG_NAME(FuncName)                                               \
    : public ezScriptFunctionMessage                                             \
  {                                                                              \
    EZ_DECLARE_MESSAGE_TYPE(_EZ_SF_MSG_NAME(FuncName), ezScriptFunctionMessage); \
    EZ_EXPAND_ARGS_PAIR(_EZ_SF_DECL_MSG_MEMBER, __VA_ARGS__)                     \
  };                                                                             \
  void _EZ_SF_MSG_HANDLER_NAME(FuncName)(_EZ_SF_MSG_NAME(FuncName) & msg);       \
  void FuncName(EZ_EXPAND_ARGS_PAIR_COMMA(_EZ_SF_DECL_FUNC_PARAM, __VA_ARGS__))


/// \internal Used to insert a member into the declaration of the message
///
/// This macro expands to "SfType<Type>::MappedType Name", which simply declares a member of the supported type, ie. for int/float types the
/// member is mapped to use 'double' as the type, etc. We also remove const-ness and reference-ness as those would not work in the message.
#define _EZ_SF_DECL_MSG_MEMBER(Type, Name) ezInternal::SfType<ezTypeTraits<Type>::NonConstReferenceType>::MappedType Name;



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//// IT GETS WORSE, I WARNED YOU !
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// \internal Implementation of EZ_SCRIPTABLE_FUNCTION_IMPL
///
/// This macro first builds the RTTI block for the message.
/// For that it needs to generate an EZ_MEMBER_PROPERTY call for every function parameter and the function return type.
/// _EZ_SF_IMPL_VOID() is a separate variant to handle 'void' as the function return type.
///
/// Additionally it has to handle zero to nine function parameters.
/// That is why the _EZ_SF_MSG_PROP_LIST_XY macros below exist. Unfortunately, on 'standard-compliant' preprocessor implementations
/// our macros to auto-expand lists start to fail at this macro nesting depth (working theory...). So instead the _EZ_SF_MSG_PROP_LIST_ macros implement all the different
/// cases manually.
///
/// After the message type is defined, we call _EZ_SF_REGISTER to auto-register the message handling function with the RTTI info of 'ClassName'.
///
/// After that we define the message handler function itself. It uses _EZ_SF_FUNC_PUSH_TMP_PARAMS to first copy all message members into local temp variables of
/// the correct type.
/// Then it does the actual function call (yay! finally!)
/// And afterwards it uses _EZ_SF_FUNC_PULL_TMP_PARAMS to copy the values of all 'out' and 'inout' parameters from the temp variables back into the message members
/// (with correct casting) such that the values are available to the scripting system.
/// It has to do the same thing for the return value of the function.
///
/// After all this, the signature of the regular member function is inserted and finally the user can write his curly braces and the script function body.
/// So for the user everything is shiny and awesome and all the parameters are just like he expected and he can return a value and he doesn't have to think about
/// all the horrors that power his maddest ideas. Oh god what have I done! I've created a monster!! It's alive!!!!
#define _EZ_SF_IMPL(ReturnType, ClassName, FuncName, ...)                                                                                                                \
  EZ_IMPLEMENT_MESSAGE_TYPE(EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName));                                                                                       \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName), 1, ezRTTIDefaultAllocator<EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName)>) \
    {                                                                                                                                                                    \
      EZ_BEGIN_ATTRIBUTES                                                                                                                                                \
      {                                                                                                                                                                  \
        new ezAutoGenVisScriptMsgSender(),                                                                                                                               \
      } EZ_END_ATTRIBUTES;                                                                                                                                               \
      EZ_CALL_MACRO(EZ_CONCAT(_EZ_SF_MSG_PROP_LIST_, EZ_VA_NUM_ARGS(ReturnType, _SfResult, __VA_ARGS__)), (ReturnType, _SfResult, __VA_ARGS__))                          \
    }                                                                                                                                                                    \
  EZ_END_DYNAMIC_REFLECTED_TYPE;                                                                                                                                         \
  _EZ_SF_REGISTER(ClassName, FuncName);                                                                                                                                  \
  void _EZ_SF_MSG_HANDLER_NAME_FULL(ClassName, FuncName)(EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName) & msg)                                                     \
  {                                                                                                                                                                      \
    EZ_EXPAND_ARGS_PAIR(_EZ_SF_FUNC_PUSH_TMP_PARAMS, __VA_ARGS__);                                                                                                       \
    msg._SfResult = ezInternal::SfType<ReturnType>::CastToMapped(FuncName(EZ_EXPAND_ARGS_PAIR_COMMA(_EZ_SF_TMP_PARAM_NAME, __VA_ARGS__)));                               \
    EZ_EXPAND_ARGS_PAIR(_EZ_SF_FUNC_PULL_TMP_PARAMS, __VA_ARGS__);                                                                                                       \
  }                                                                                                                                                                      \
  ReturnType ClassName::FuncName(EZ_EXPAND_ARGS_PAIR_COMMA(_EZ_SF_DECL_FUNC_PARAM, __VA_ARGS__))


/// \internal Same as _EZ_SF_IMPL() but with adjustments for the ReturnType being 'void'
#define _EZ_SF_IMPL_VOID(ClassName, FuncName, ...)                                                                                                                       \
  EZ_IMPLEMENT_MESSAGE_TYPE(EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName));                                                                                       \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName), 1, ezRTTIDefaultAllocator<EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName)>) \
    {                                                                                                                                                                    \
      EZ_BEGIN_ATTRIBUTES                                                                                                                                                \
      {                                                                                                                                                                  \
        new ezAutoGenVisScriptMsgSender(),                                                                                                                               \
      } EZ_END_ATTRIBUTES;                                                                                                                                               \
      EZ_CALL_MACRO(EZ_CONCAT(_EZ_SF_MSG_PROP_LIST_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))                                                                        \
    }                                                                                                                                                                    \
  EZ_END_DYNAMIC_REFLECTED_TYPE;                                                                                                                                         \
  _EZ_SF_REGISTER(ClassName, FuncName);                                                                                                                                  \
  void _EZ_SF_MSG_HANDLER_NAME_FULL(ClassName, FuncName)(EZ_SCRIPTABLE_FUNCTION_MSG_TYPE(ClassName, FuncName) & msg)                                                     \
  {                                                                                                                                                                      \
    EZ_EXPAND_ARGS_PAIR(_EZ_SF_FUNC_PUSH_TMP_PARAMS, __VA_ARGS__);                                                                                                       \
    FuncName(EZ_EXPAND_ARGS_PAIR_COMMA(_EZ_SF_TMP_PARAM_NAME, __VA_ARGS__));                                                                                             \
    EZ_EXPAND_ARGS_PAIR(_EZ_SF_FUNC_PULL_TMP_PARAMS, __VA_ARGS__);                                                                                                       \
  }                                                                                                                                                                      \
  void ClassName::FuncName(EZ_EXPAND_ARGS_PAIR_COMMA(_EZ_SF_DECL_FUNC_PARAM, __VA_ARGS__))


//////////////////////////////////////////////////////////////////////////

/// \internal Macro used to create the reflection information for a message member
///
/// This macro boils down to inserting an EZ_MEMBER_PROPERTY call for the given member variable.
/// It uses SfRemoveParamPrefix to trim 'out_' and 'inout_' from the exposed name and
/// it calls SfGetParamFlags to tag the property with the necessary ezPropertyFlag's such that the scripting system knows whether
/// this property should be handled as an in, out or inout parameter.
#define _EZ_SF_IMPL_MSG_MEMBER(Type, Name) EZ_MEMBER_PROPERTY(ezInternal::SfRemoveParamPrefix(_EZ_SF_IS_NON_CONST_REF(Type), EZ_STRINGIZE(Name)), Name)->AddFlags(ezInternal::SfGetParamFlags(_EZ_SF_IS_NON_CONST_REF(Type), EZ_STRINGIZE(Name)))

//////////////////////////////////////////////////////////////////////////

/// \internal Handles the case of no return type and no parameters, ie. an empty __VA_ARGS__ list (which will still be reported as having one element)
#define _EZ_SF_MSG_PROP_LIST_1(...)

/// \internal Handles the case of having a return type but no parameters, ie. an empty __VA_ARGS__ list (which will still be reported as having one element)
#define _EZ_SF_MSG_PROP_LIST_3(t1, v1, ...) _EZ_SF_MSG_PROP_LIST_2(t1, v1)

/// \internal Handles the case of 1 parameter type/name pair
#define _EZ_SF_MSG_PROP_LIST_2(t1, v1) \
  EZ_BEGIN_PROPERTIES                  \
  {                                    \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),    \
  } EZ_END_PROPERTIES;

/// \internal Handles the case of 2 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_4(t1, v1, t2, v2) \
  EZ_BEGIN_PROPERTIES                          \
  {                                            \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),            \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),            \
  } EZ_END_PROPERTIES;

/// \internal Handles the case of 3 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_6(t1, v1, t2, v2, t3, v3) \
  EZ_BEGIN_PROPERTIES                                  \
  {                                                    \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                    \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                    \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                    \
  } EZ_END_PROPERTIES;

/// \internal Handles the case of 4 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_8(t1, v1, t2, v2, t3, v3, t4, v4) \
  EZ_BEGIN_PROPERTIES                                          \
  {                                                            \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                            \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                            \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                            \
    _EZ_SF_IMPL_MSG_MEMBER(t4, v4),                            \
  } EZ_END_PROPERTIES;


/// \internal Handles the case of 5 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_10(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5) \
  EZ_BEGIN_PROPERTIES                                                   \
  {                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t4, v4),                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t5, v5),                                     \
  } EZ_END_PROPERTIES;


/// \internal Handles the case of 6 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_12(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6) \
  EZ_BEGIN_PROPERTIES                                                           \
  {                                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t4, v4),                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t5, v5),                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t6, v6),                                             \
  } EZ_END_PROPERTIES;

/// \internal Handles the case of 7 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_14(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7) \
  EZ_BEGIN_PROPERTIES                                                                   \
  {                                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t4, v4),                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t5, v5),                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t6, v6),                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t7, v7),                                                     \
  } EZ_END_PROPERTIES;

/// \internal Handles the case of 8 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_16(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8) \
  EZ_BEGIN_PROPERTIES                                                                           \
  {                                                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t4, v4),                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t5, v5),                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t6, v6),                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t7, v7),                                                             \
    _EZ_SF_IMPL_MSG_MEMBER(t8, v8),                                                             \
  } EZ_END_PROPERTIES;

/// \internal Handles the case of 9 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_18(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9) \
  EZ_BEGIN_PROPERTIES                                                                                   \
  {                                                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t4, v4),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t5, v5),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t6, v6),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t7, v7),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t8, v8),                                                                     \
    _EZ_SF_IMPL_MSG_MEMBER(t9, v9),                                                                     \
  } EZ_END_PROPERTIES;

/// \internal Handles the case of 10 parameter type/name pairs
#define _EZ_SF_MSG_PROP_LIST_20(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9, t10, v10) \
  EZ_BEGIN_PROPERTIES                                                                                             \
  {                                                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t1, v1),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t2, v2),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t3, v3),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t4, v4),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t5, v5),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t6, v6),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t7, v7),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t8, v8),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t9, v9),                                                                               \
    _EZ_SF_IMPL_MSG_MEMBER(t10, v10),                                                                             \
  } EZ_END_PROPERTIES;

//////////////////////////////////////////////////////////////////////////

/// \internal Used BEFORE the regular function is called to define temp variables within the message handler function to store/copy all the message members
///
/// At this point we cast the member variables to the actual type, such that they can be passed easily into the regular function.
/// Although this could be done in-place in the function call, the temp variables are needed to enable _EZ_SF_FUNC_PULL_TMP_PARAMS to
/// cast any out and inout parameters to the MappedType and move the output into the message members again.
#define _EZ_SF_FUNC_PUSH_TMP_PARAMS(Type, Name) ezTypeTraits<Type>::NonConstReferenceType _EZ_SF_TMP_PARAM_NAME(Type, Name) = ezInternal::SfType<ezTypeTraits<Type>::NonConstReferenceType>::CastFromMapped(msg.Name);

/// \internal Used AFTER the regular function was called, to take the 'out' and 'inout' parameters and store them back into the message members.
///
/// At this point we also need to cast the values from the actual parameter type to the MappedType again.
///
/// To prevent unnecessary copies, only non-const reference parameters get pulled back. Although this is wrapped in an if statement,
/// the if boils down to a compile-time constant and should be optimized away. Ideally this will then also enable the compiler to optimize away the entire temporary.
#define _EZ_SF_FUNC_PULL_TMP_PARAMS(Type, Name)                                                                                \
  if (_EZ_SF_IS_NON_CONST_REF(Type))                                                                                           \
  {                                                                                                                            \
    msg.Name = ezInternal::SfType<ezTypeTraits<Type>::NonConstReferenceType>::CastToMapped(_EZ_SF_TMP_PARAM_NAME(Type, Name)); \
  }
