#include <FoundationPCH.h>

#include <Foundation/Reflection/Reflection.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAttribute, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReadOnlyAttribute, 1, ezRTTIDefaultAllocator<ezReadOnlyAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHiddenAttribute, 1, ezRTTIDefaultAllocator<ezHiddenAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTemporaryAttribute, 1, ezRTTIDefaultAllocator<ezTemporaryAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCategoryAttribute, 1, ezRTTIDefaultAllocator<ezCategoryAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Category", m_sCategory),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTitleAttribute, 1, ezRTTIDefaultAllocator<ezTitleAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Title", m_sTitle),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorAttribute, 1, ezRTTIDefaultAllocator<ezColorAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_Color),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezColor),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposeColorAlphaAttribute, 1, ezRTTIDefaultAllocator<ezExposeColorAlphaAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSuffixAttribute, 1, ezRTTIDefaultAllocator<ezSuffixAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Suffix", m_sSuffix),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMinValueTextAttribute, 1, ezRTTIDefaultAllocator<ezMinValueTextAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Text", m_sText),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDefaultValueAttribute, 1, ezRTTIDefaultAllocator<ezDefaultValueAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const ezVariant&),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClampValueAttribute, 1, ezRTTIDefaultAllocator<ezClampValueAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Min", m_MinValue),
    EZ_MEMBER_PROPERTY("Max", m_MaxValue),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const ezVariant&, const ezVariant&),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeWidgetAttribute, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezContainerWidgetAttribute, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTagSetWidgetAttribute, 1, ezRTTIDefaultAllocator<ezTagSetWidgetAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Filter", m_sTagFilter),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedParametersAttribute, 1, ezRTTIDefaultAllocator<ezExposedParametersAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ParametersSource", m_sParametersSource),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezContainerAttribute, 1, ezRTTIDefaultAllocator<ezContainerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CanAdd", m_bCanAdd),
    EZ_MEMBER_PROPERTY("CanDelete", m_bCanDelete),
    EZ_MEMBER_PROPERTY("CanMove", m_bCanMove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(bool, bool, bool),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConstrainPointerAttribute, 1, ezRTTIDefaultAllocator<ezConstrainPointerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ConstantName", m_sConstantName),
    EZ_MEMBER_PROPERTY("ConstantValue", m_sConstantValueProperty),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFileBrowserAttribute, 1, ezRTTIDefaultAllocator<ezFileBrowserAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Title", m_sDialogTitle),
    EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetBrowserAttribute, 1, ezRTTIDefaultAllocator<ezAssetBrowserAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Filter", m_sTypeFilter),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicEnumAttribute, 1, ezRTTIDefaultAllocator<ezDynamicEnumAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
   EZ_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
   EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicStringEnumAttribute, 1, ezRTTIDefaultAllocator<ezDynamicStringEnumAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezManipulatorAttribute, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Property1", m_sProperty1),
    EZ_MEMBER_PROPERTY("Property2", m_sProperty2),
    EZ_MEMBER_PROPERTY("Property3", m_sProperty3),
    EZ_MEMBER_PROPERTY("Property4", m_sProperty4),
    EZ_MEMBER_PROPERTY("Property5", m_sProperty5),
    EZ_MEMBER_PROPERTY("Property6", m_sProperty6),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezManipulatorAttribute::ezManipulatorAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/,
                                               const char* szProperty3 /*= nullptr*/, const char* szProperty4 /*= nullptr*/,
                                               const char* szProperty5 /*= nullptr*/, const char* szProperty6 /*= nullptr*/)
{
  m_sProperty1 = szProperty1;
  m_sProperty2 = szProperty2;
  m_sProperty3 = szProperty3;
  m_sProperty4 = szProperty4;
  m_sProperty5 = szProperty5;
  m_sProperty6 = szProperty6;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSphereManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezSphereManipulatorAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSphereManipulatorAttribute::ezSphereManipulatorAttribute()
    : ezManipulatorAttribute(nullptr)
{
}

ezSphereManipulatorAttribute::ezSphereManipulatorAttribute(const char* szOuterRadius, const char* szInnerRadius)
    : ezManipulatorAttribute(szOuterRadius, szInnerRadius)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCapsuleManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezCapsuleManipulatorAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCapsuleManipulatorAttribute::ezCapsuleManipulatorAttribute()
    : ezManipulatorAttribute(nullptr)
{
}

ezCapsuleManipulatorAttribute::ezCapsuleManipulatorAttribute(const char* szLength, const char* szRadius)
    : ezManipulatorAttribute(szLength, szRadius)
{
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoxManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezBoxManipulatorAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBoxManipulatorAttribute::ezBoxManipulatorAttribute()
    : ezManipulatorAttribute(nullptr)
{
}

ezBoxManipulatorAttribute::ezBoxManipulatorAttribute(const char* szSize)
    : ezManipulatorAttribute(szSize)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezNonUniformBoxManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezNonUniformBoxManipulatorAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezNonUniformBoxManipulatorAttribute::ezNonUniformBoxManipulatorAttribute()
    : ezManipulatorAttribute(nullptr)
{
}

ezNonUniformBoxManipulatorAttribute::ezNonUniformBoxManipulatorAttribute(const char* szNegXProp, const char* szPosXProp,
                                                                         const char* szNegYProp, const char* szPosYProp,
                                                                         const char* szNegZProp, const char* szPosZProp)
    : ezManipulatorAttribute(szNegXProp, szPosXProp, szNegYProp, szPosYProp, szNegZProp, szPosZProp)
{
}

ezNonUniformBoxManipulatorAttribute::ezNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ)
    : ezManipulatorAttribute(szSizeX, szSizeY, szSizeZ)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConeLengthManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezConeLengthManipulatorAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezConeLengthManipulatorAttribute::ezConeLengthManipulatorAttribute()
    : ezManipulatorAttribute(nullptr)
{
}

ezConeLengthManipulatorAttribute::ezConeLengthManipulatorAttribute(const char* szRadiusProperty)
    : ezManipulatorAttribute(szRadiusProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConeAngleManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezConeAngleManipulatorAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("scale", m_fScale),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezConeAngleManipulatorAttribute::ezConeAngleManipulatorAttribute()
    : ezManipulatorAttribute(nullptr)
{
  m_fScale = 1.0f;
}

ezConeAngleManipulatorAttribute::ezConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale, const char* szRadiusProperty)
    : ezManipulatorAttribute(szAngleProperty, szRadiusProperty)
{
  m_fScale = fScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransformManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezTransformManipulatorAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTransformManipulatorAttribute::ezTransformManipulatorAttribute()
    : ezManipulatorAttribute(nullptr)
{
}

ezTransformManipulatorAttribute::ezTransformManipulatorAttribute(const char* szTranslateProperty, const char* szRotateProperty,
                                                                 const char* szScaleProperty)
    : ezManipulatorAttribute(szTranslateProperty, szRotateProperty, szScaleProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualizerAttribute, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Property1", m_sProperty1),
    EZ_MEMBER_PROPERTY("Property2", m_sProperty2),
    EZ_MEMBER_PROPERTY("Property3", m_sProperty3),
    EZ_MEMBER_PROPERTY("Property4", m_sProperty4),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualizerAttribute::ezVisualizerAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/,
                                             const char* szProperty3 /*= nullptr*/, const char* szProperty4 /*= nullptr*/,
                                             const char* szProperty5 /*= nullptr*/)
{
  m_sProperty1 = szProperty1;
  m_sProperty2 = szProperty2;
  m_sProperty3 = szProperty3;
  m_sProperty4 = szProperty4;
  m_sProperty5 = szProperty5;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoxVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezBoxVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("color", m_Color),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&, const char*, ezVec3),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBoxVisualizerAttribute::ezBoxVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
}

ezBoxVisualizerAttribute::ezBoxVisualizerAttribute(const char* szSizeProperty, const char* szColorProperty /*= nullptr*/,
                                                   const ezColor& fixedColor /*= ezColor::MediumVioletRed*/,
                                                   const char* szOffsetProperty /*= nullptr*/,
                                                   ezVec3 fixedOffset /*= ezVec3::ZeroVector()*/)
    : ezVisualizerAttribute(szSizeProperty, szColorProperty, szOffsetProperty)
{
  m_Color = fixedColor;
  m_vOffset = fixedOffset;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSphereVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezSphereVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("color", m_Color),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&, const char*, ezVec3),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSphereVisualizerAttribute::ezSphereVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
}

ezSphereVisualizerAttribute::ezSphereVisualizerAttribute(const char* szRadiusProperty, const char* szColorProperty /*= nullptr*/,
                                                         const ezColor& fixedColor /*= ezColor::MediumVioletRed*/,
                                                         const char* szOffsetProperty /*= nullptr*/,
                                                         ezVec3 fixedOffset /*= ezVec3::ZeroVector()*/)
    : ezVisualizerAttribute(szRadiusProperty, szColorProperty, szOffsetProperty)
{
  m_vOffset = fixedOffset;
  m_Color = fixedColor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCapsuleVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezCapsuleVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("color", m_Color),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCapsuleVisualizerAttribute::ezCapsuleVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
}

ezCapsuleVisualizerAttribute::ezCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty,
                                                           const char* szColorProperty)
    : ezVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty)
{
  m_Color = ezColor::MediumVioletRed;
}

ezCapsuleVisualizerAttribute::ezCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty,
                                                           const ezColor& fixedColor /*= ezColor::MediumVioletRed*/)
    : ezVisualizerAttribute(szHeightProperty, szRadiusProperty)
{
  m_Color = fixedColor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCylinderVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezCylinderVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("color", m_Color),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const char*, const ezColor&, const char*, ezVec3),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const char*, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCylinderVisualizerAttribute::ezCylinderVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
}

ezCylinderVisualizerAttribute::ezCylinderVisualizerAttribute(ezEnum<ezBasisAxis> axis, const char* szHeightProperty,
                                                             const char* szRadiusProperty, const char* szColorProperty /*= nullptr*/,
                                                             const ezColor& fixedColor /*= ezColor::MediumVioletRed*/,
                                                             const char* szOffsetProperty /*= nullptr*/,
                                                             ezVec3 fixedOffset /*= ezVec3::ZeroVector()*/)
    : ezVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty)
{
  m_Axis = axis;
  m_Color = fixedColor;
  m_vOffset = fixedOffset;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDirectionVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezDirectionVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("axis", ezBasisAxis, m_Axis),
    EZ_MEMBER_PROPERTY("color", m_Color),
    EZ_MEMBER_PROPERTY("scale", m_fScale)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDirectionVisualizerAttribute::ezDirectionVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
  m_Axis = ezBasisAxis::PositiveX;
  m_fScale = 1.0f;
}

ezDirectionVisualizerAttribute::ezDirectionVisualizerAttribute(ezEnum<ezBasisAxis> axis, float fScale, const char* szColorProperty,
                                                               const char* szLengthProperty /*= nullptr*/)
    : ezVisualizerAttribute(szColorProperty, szLengthProperty)
{
  m_Axis = axis;
  m_fScale = fScale;
  m_Color = ezColor::MediumVioletRed;
}

ezDirectionVisualizerAttribute::ezDirectionVisualizerAttribute(ezEnum<ezBasisAxis> axis, float fScale,
                                                               const ezColor& fixedColor /*= ezColor::MediumVioletRed*/,
                                                               const char* szLengthProperty /*= nullptr*/)
    : ezVisualizerAttribute(nullptr, szLengthProperty)
{
  m_Axis = axis;
  m_fScale = fScale;
  m_Color = fixedColor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConeVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezConeVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("axis", ezBasisAxis, m_Axis),
    EZ_MEMBER_PROPERTY("color", m_Color),
    EZ_MEMBER_PROPERTY("scale", m_fScale),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, float, const char*, const char*, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, float, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, float, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezConeVisualizerAttribute::ezConeVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
  m_Axis = ezBasisAxis::PositiveX;
  m_Color = ezColor::Red;
  m_fScale = 1.0f;
}

ezConeVisualizerAttribute::ezConeVisualizerAttribute(ezEnum<ezBasisAxis> axis, const char* szAngleProperty, float fScale,
                                                     const char* szRadiusProperty, const char* szColorProperty,
                                                     const ezColor& fixedColor /*= ezColor::MediumVioletRed*/)
    : ezVisualizerAttribute(szAngleProperty, szRadiusProperty, szColorProperty)
{
  m_Axis = axis;
  m_Color = fixedColor;
  m_fScale = fScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezCameraVisualizerAttribute>)
{
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCameraVisualizerAttribute::ezCameraVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
}

ezCameraVisualizerAttribute::ezCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty,
                                                         const char* szOrthoDimProperty, const char* szNearPlaneProperty,
                                                         const char* szFarPlaneProperty)
    : ezVisualizerAttribute(szModeProperty, szFovProperty, szOrthoDimProperty, szNearPlaneProperty, szFarPlaneProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaxArraySizeAttribute, 1, ezRTTIDefaultAllocator<ezMaxArraySizeAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxSize", m_uiMaxSize),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezUInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPreventDuplicatesAttribute, 1, ezRTTIDefaultAllocator<ezPreventDuplicatesAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAutoGenVisScriptMsgSender, 1, ezRTTIDefaultAllocator<ezAutoGenVisScriptMsgSender>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAutoGenVisScriptMsgHandler, 1, ezRTTIDefaultAllocator<ezAutoGenVisScriptMsgHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyAttributes);

