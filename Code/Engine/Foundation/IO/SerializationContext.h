
#pragma once

/// \brief Base class for serialization contexts. A serialization context can be used to add high level logic to serialization, e.g.
/// de-duplicating objects.
///
/// Typically a context is created before any serialization happens and can then be accessed anywhere through the GetContext method.
template <typename Derived>
class EZ_FOUNDATION_DLL ezSerializationContext
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSerializationContext);

public:
  ezSerializationContext() { SetContext(this); }
  ~ezSerializationContext() { SetContext(nullptr); }

  static Derived* GetContext();

protected:
  static void SetContext(ezSerializationContext* pContext);
};

/// \brief Implements the necessary functions to access a serialization context through GetContext.
#define EZ_IMPLEMENT_SERIALIZATION_CONTEXT(type)                                                                                           \
  thread_local type* EZ_CONCAT(s_pActiveContext, type);                                                                                    \
  template <>                                                                                                                              \
  type* ezSerializationContext<type>::GetContext()                                                                                         \
  {                                                                                                                                        \
    return EZ_CONCAT(s_pActiveContext, type);                                                                                              \
  }                                                                                                                                        \
  template <>                                                                                                                              \
  void ezSerializationContext<type>::SetContext(ezSerializationContext* pContext)                                                          \
  {                                                                                                                                        \
    EZ_ASSERT_DEV(pContext == nullptr || EZ_CONCAT(s_pActiveContext, type) == nullptr, "Only one context can be active at a time.");       \
    EZ_CONCAT(s_pActiveContext, type) = static_cast<type*>(pContext);                                                                      \
  }

