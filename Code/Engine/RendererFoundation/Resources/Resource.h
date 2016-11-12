
#pragma once

#include <RendererFoundation/Basics.h>
#include <Foundation/Containers/HashTable.h>

class EZ_RENDERERFOUNDATION_DLL ezGALResourceBase : public ezRefCounted
{
public:
  virtual void SetDebugName(const char* szName) const = 0;

protected:
  friend class ezGALDevice;

  EZ_FORCE_INLINE ~ezGALResourceBase()
  {
    EZ_ASSERT_DEV(m_hDefaultResourceView.IsInvalidated(), "");
    EZ_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");

    EZ_ASSERT_DEV(m_ResourceViews.IsEmpty(), "Dangling resource views");
    EZ_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
  }

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
  EZ_FORCE_INLINE ezGALResource(const CreationDescription& Description)
    : m_Description(Description)
  {
  }

  EZ_FORCE_INLINE const CreationDescription& GetDescription() const
  {
    return m_Description;
  }

protected:
  const CreationDescription m_Description;
};
