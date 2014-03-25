#pragma once

#include <Foundation/Basics.h>

class ezDelegateBase
{
public:
  union InstancePtr
  {
    void* m_Ptr;
    const void* m_ConstPtr;
  };

  EZ_FORCE_INLINE ezDelegateBase()
  {
    m_pInstance.m_Ptr = NULL;
  }

protected:
  InstancePtr m_pInstance;
};

/// \brief A generic delegate class which supports static functions and member functions.
template <typename T>
class ezDelegate : public ezDelegateBase
{
  /// \todo Some more documentation, please ?

};

#include <Foundation/Basics/Types/Implementation/Delegate_inl.h>

