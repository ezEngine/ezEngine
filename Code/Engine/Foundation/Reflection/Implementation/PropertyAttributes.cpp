#include <Foundation/PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAttribute, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReadOnlyAttribute, 1, ezRTTIDefaultAllocator<ezReadOnlyAttribute>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHiddenAttribute, 1, ezRTTIDefaultAllocator<ezHiddenAttribute>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCategoryAttribute, 1, ezRTTIDefaultAllocator<ezCategoryAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Category", m_sCategory)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDefaultValueAttribute, 1, ezRTTIDefaultAllocator<ezDefaultValueAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Value", m_Value),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClampValueAttribute, 1, ezRTTIDefaultAllocator<ezClampValueAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Min", m_MinValue),
EZ_MEMBER_PROPERTY("Max", m_MaxValue),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeWidgetAttribute, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezContainerWidgetAttribute, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTagSetWidgetAttribute, 1, ezRTTIDefaultAllocator<ezTagSetWidgetAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Filter", m_sTagFilter)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezContainerAttribute, 1, ezRTTIDefaultAllocator<ezContainerAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("CanAdd", m_bCanAdd),
EZ_MEMBER_PROPERTY("CanDelete", m_bCanDelete),
EZ_MEMBER_PROPERTY("CanMove", m_bCanMove),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFileBrowserAttribute, 1, ezRTTIDefaultAllocator<ezFileBrowserAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Title", m_sDialogTitle),
EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetBrowserAttribute, 1, ezRTTIDefaultAllocator<ezAssetBrowserAttribute>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter)
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();



EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyAttributes);

