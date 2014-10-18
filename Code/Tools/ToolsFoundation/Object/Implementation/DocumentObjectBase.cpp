#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentObjectBaseProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();
