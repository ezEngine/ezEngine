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
    m_uiRandomNumberGeneratorSeed = 0;
    m_bReportErrorWhenStaticObjectMoves = true;
  }

  ezHashedString m_sName;
  ezUInt64 m_uiRandomNumberGeneratorSeed;

  ezUniquePtr<ezSpatialSystem> m_pSpatialSystem;
  ezUniquePtr<ezCoordinateSystemProvider> m_pCoordinateSystemProvider;

  bool m_bReportErrorWhenStaticObjectMoves;
};

