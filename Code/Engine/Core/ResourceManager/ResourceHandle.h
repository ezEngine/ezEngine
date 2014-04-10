#pragma once

#include <Core/Basics.h>


template<typename ResourceType>
class ezResourceHandle
{
public:
  ezResourceHandle()
  {
    m_pResource = NULL;
  }

  ezResourceHandle(ResourceType* pResource)
  {
    m_pResource = pResource;

    if (m_pResource)
      m_pResource->m_iReferenceCount.Increment();
  }

  ezResourceHandle(const ezResourceHandle<ResourceType>& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
      m_pResource->m_iReferenceCount.Increment();
  }

  void operator=(const ezResourceHandle<ResourceType>& rhs)
  {
    if (m_pResource)
      m_pResource->m_iReferenceCount.Decrement();

    m_pResource = rhs.m_pResource;

    if (m_pResource)
      m_pResource->m_iReferenceCount.Increment();
  }

  ~ezResourceHandle()
  {
    Invalidate();
  }

  bool IsValid() const
  {
    return m_pResource != NULL;
  }

  void Invalidate()
  {
    if (m_pResource)
      m_pResource->m_iReferenceCount.Decrement();

    m_pResource = NULL;
  }

private:
  friend class ezResourceManager;

  ResourceType* m_pResource;
};


