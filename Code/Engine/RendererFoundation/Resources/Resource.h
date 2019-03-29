
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

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

  inline ~ezGALResourceBase()
  {
    EZ_ASSERT_DEV(m_hDefaultResourceView.IsInvalidated(), "");
    EZ_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");

    EZ_ASSERT_DEV(m_ResourceViews.IsEmpty(), "Dangling resource views");
    EZ_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
    EZ_ASSERT_DEV(m_UnorderedAccessViews.IsEmpty(), "Dangling unordered access views");
  }

  virtual void SetDebugNamePlatform(const char* szName) const = 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  mutable ezHashedString m_sDebugName;
#endif

  ezGALResourceViewHandle m_hDefaultResourceView;
  ezGALRenderTargetViewHandle m_hDefaultRenderTargetView;

  ezHashTable<ezUInt32, ezGALResourceViewHandle> m_ResourceViews;
  ezHashTable<ezUInt32, ezGALRenderTargetViewHandle> m_RenderTargetViews;
  ezHashTable<ezUInt32, ezGALUnorderedAccessViewHandle> m_UnorderedAccessViews;
};

/// \brief Base class for GAL resources, stores a creation description of the object and also allows for reference counting.
template<typename CreationDescription>
class ezGALResource : public ezGALResourceBase
{
public:
  EZ_ALWAYS_INLINE ezGALResource(const CreationDescription& Description)
    : m_Description(Description)
  {
  }

  EZ_ALWAYS_INLINE const CreationDescription& GetDescription() const
  {
    return m_Description;
  }

protected:
  const CreationDescription m_Description;
};

