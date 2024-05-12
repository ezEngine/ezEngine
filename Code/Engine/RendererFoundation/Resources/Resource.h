
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALResourceBase : public ezRefCounted
{
public:
  void SetDebugName(const char* szName) const
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    m_sDebugName.Assign(szName);
#endif

    SetDebugNamePlatform(szName);
  }

  virtual const ezGALResourceBase* GetParentResource() const { return this; }

protected:
  friend class ezGALDevice;

  inline ~ezGALResourceBase() = default;

  virtual void SetDebugNamePlatform(const char* szName) const = 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  mutable ezHashedString m_sDebugName;
#endif
};

/// \brief Base class for GAL resources, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class ezGALResource : public ezGALResourceBase
{
public:
  EZ_ALWAYS_INLINE ezGALResource(const CreationDescription& description)
    : m_Description(description)
  {
  }

  EZ_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};
