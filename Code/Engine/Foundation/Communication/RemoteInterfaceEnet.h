#pragma once

#include <Foundation/Communication/RemoteInterface.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

/// An implementation for ezRemoteInterface built on top of Enet
class EZ_FOUNDATION_DLL ezRemoteInterfaceEnet : public ezRemoteInterface
{
public:
  ~ezRemoteInterfaceEnet();

  /// Allocates a new instance with the given allocator
  static ezInternal::NewInstance<ezRemoteInterfaceEnet> Make(ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  /// The port through which the connection was started
  ezUInt16 GetPort() const { return m_uiPort; }

private:
  ezRemoteInterfaceEnet();
  friend class ezRemoteInterfaceEnetImpl;

protected:
  ezUInt16 m_uiPort = 0;
};

#endif
