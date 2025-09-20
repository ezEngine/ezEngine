#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/World/CoordinateSystem.h>
#include <Core/World/SpatialSystem.h>

class ezTimeStepSmoothing;

/// \brief Describes the initial state of a world.
struct ezWorldDesc
{
  EZ_DECLARE_POD_TYPE();

  ezWorldDesc(ezStringView sWorldName) { m_sName.Assign(sWorldName); }

  ezHashedString m_sName;                                                         ///< Name of the world for identification
  ezUInt64 m_uiRandomNumberGeneratorSeed = 0;                                     ///< Seed for the world's random number generator (0 = use current time)

  ezUniquePtr<ezSpatialSystem> m_pSpatialSystem;                                  ///< Custom spatial system to use for this world
  bool m_bAutoCreateSpatialSystem = true;                                         ///< Automatically create a default spatial system if none is set

  ezSharedPtr<ezCoordinateSystemProvider> m_pCoordinateSystemProvider;            ///< Optional provider for position-dependent coordinate systems
  ezUniquePtr<ezTimeStepSmoothing> m_pTimeStepSmoothing;                          ///< Custom time step smoothing (if nullptr, ezDefaultTimeStepSmoothing will be used)

  bool m_bReportErrorWhenStaticObjectMoves = true;                                ///< Whether to log errors when objects marked as static change position

  ezTime m_MaxComponentInitializationTimePerFrame = ezTime::MakeFromHours(10000); ///< Maximum time to spend on component initialization per frame
};
