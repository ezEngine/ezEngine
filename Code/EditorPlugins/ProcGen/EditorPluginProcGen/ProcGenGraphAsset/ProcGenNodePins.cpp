#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodePins.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProcGenNodePin, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProcGenNodeInputPin, ezProcGenNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProcGenNodeOutputPin, ezProcGenNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on
