#include <Core/CorePCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Core/Utils/CustomData.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void ezCustomData::Load(ezStreamReader& inout_stream)
{
  ezReflectionSerializer::ReadObjectPropertiesFromBinary(inout_stream, *GetDynamicRTTI(), this);
}

void ezCustomData::Save(ezStreamWriter& inout_stream) const
{
  ezReflectionSerializer::WriteObjectToBinary(inout_stream, GetDynamicRTTI(), this);
}

EZ_STATICLINK_FILE(Core, Core_Utils_Implementation_CustomData);
