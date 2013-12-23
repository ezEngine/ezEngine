#include <Foundation/PCH.h>
#include <Foundation/Reflection/Implementation/StandardTypes.h>
#include <Foundation/Reflection/Reflection.h>

EZ_BEGIN_REFLECTED_TYPE(ezReflectedClass, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezReflectedClass);

// *********************************************
// ***** Standard POD Types for Properties *****

EZ_BEGIN_REFLECTED_TYPE(bool, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(bool);

EZ_BEGIN_REFLECTED_TYPE(float, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(float);

EZ_BEGIN_REFLECTED_TYPE(double, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(double);

EZ_BEGIN_REFLECTED_TYPE(ezInt8, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezInt8);

EZ_BEGIN_REFLECTED_TYPE(ezUInt8, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezUInt8);

EZ_BEGIN_REFLECTED_TYPE(ezInt16, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezInt16);

EZ_BEGIN_REFLECTED_TYPE(ezUInt16, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezUInt16);

EZ_BEGIN_REFLECTED_TYPE(ezInt32, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezInt32);

EZ_BEGIN_REFLECTED_TYPE(ezUInt32, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezUInt32);

EZ_BEGIN_REFLECTED_TYPE(ezInt64, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezInt64);

EZ_BEGIN_REFLECTED_TYPE(ezUInt64, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezUInt64);

EZ_BEGIN_REFLECTED_TYPE(ezConstCharPtr, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezConstCharPtr);

EZ_BEGIN_REFLECTED_TYPE(ezVec2, ezNoBase, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y)
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(ezVec2);

EZ_BEGIN_REFLECTED_TYPE(ezVec3, ezNoBase, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z)
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(ezVec3);

EZ_BEGIN_REFLECTED_TYPE(ezVec4, ezNoBase, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("x", x),
    EZ_MEMBER_PROPERTY("y", y),
    EZ_MEMBER_PROPERTY("z", z),
    EZ_MEMBER_PROPERTY("w", w)
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(ezVec4);

EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_StandardTypes);

