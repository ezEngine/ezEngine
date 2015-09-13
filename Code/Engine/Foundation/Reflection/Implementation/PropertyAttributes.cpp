#include <Foundation/PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAttribute, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReadOnlyAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezReadOnlyAttribute>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHiddenAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezHiddenAttribute>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDefaultValueAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezDefaultValueAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Value", m_Value),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeWidgetAttribute, ezPropertyAttribute, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezContainerWidgetAttribute, ezPropertyAttribute, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezContainerAttribute, ezPropertyAttribute, 1, ezRTTIDefaultAllocator<ezContainerAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("CanAdd", m_bCanAdd),
EZ_MEMBER_PROPERTY("CanDelete", m_bCanDelete),
EZ_MEMBER_PROPERTY("CanMove", m_bCanMove),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFileBrowserAttribute, ezTypeWidgetAttribute, 1, ezRTTIDefaultAllocator<ezFileBrowserAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Title", m_sDialogTitle),
EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetBrowserAttribute, ezTypeWidgetAttribute, 1, ezRTTIDefaultAllocator<ezAssetBrowserAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

