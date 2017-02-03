#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/World/CoordinateSystem.h>
#include <Core/World/SpatialSystem.h>

/// \brief Describes the initial state of a world.
struct ezWorldDesc
{
  EZ_DECLARE_POD_TYPE();

  ezWorldDesc(const char* szWorldName)
  {
    m_sName.Assign(szWorldName);
  }

  ezHashedString m_sName;

  ezUniquePtr<ezSpatialSystem> m_pSpatialSystem;
  ezUniquePtr<ezCoordinateSystemProvider> m_pCoordinateSystemProvider;
};
