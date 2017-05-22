#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Communication/Message.h>

struct ezNavMeshDescription;

struct EZ_GAMEENGINE_DLL ezBuildNavMeshMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezBuildNavMeshMessage, ezMessage);

  /// \brief Append data to this description to include additional obstacles into the nav mesh generation
  ezNavMeshDescription* m_pNavMeshDescription;
};

