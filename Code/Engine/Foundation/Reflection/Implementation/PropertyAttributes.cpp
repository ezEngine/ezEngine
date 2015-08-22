#include <Foundation/PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAttribute, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReadOnlyAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezReadOnlyAttribute>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHiddenAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezHiddenAttribute>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFileBrowserAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezFileBrowserAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Title", m_sDialogTitle),
EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetBrowserAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezAssetBrowserAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

