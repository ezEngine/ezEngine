#include <PCH.h>

#include <GameEngine/Messages/BuildNavMeshMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgBuildNavMesh);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgBuildNavMesh, 1, ezRTTIDefaultAllocator<ezMsgBuildNavMesh>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_BuildNavMeshMessage);
