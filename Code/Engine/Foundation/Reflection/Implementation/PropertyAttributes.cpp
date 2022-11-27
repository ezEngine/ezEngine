#include <Foundation/FoundationPCH.h>

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInDevelopmentAttribute, 1, ezRTTIDefaultAllocator<ezInDevelopmentAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Phase", m_Phase),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezInt32),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;


const char* ezInDevelopmentAttribute::GetString() const
{
  switch (m_Phase)
  {
  case Phase::Alpha:
    return "ALPHA";

  case Phase::Beta:
    return "BETA";

    EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGroupAttribute, 1, ezRTTIDefaultAllocator<ezGroupAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Group", m_sGroup),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, float),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, float),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezGroupAttribute::ezGroupAttribute()
{

}

ezGroupAttribute::ezGroupAttribute(const char* szGroup, float fOrder)
{
  m_sGroup = szGroup;
  m_fOrder = fOrder;
}

ezGroupAttribute::ezGroupAttribute(const char* szGroup, const char* szIconName, float fOrder)
{
  m_sGroup = szGroup;
  m_sIconName = szIconName;
  m_fOrder = fOrder;
}

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicDefaultValueAttribute, 1, ezRTTIDefaultAllocator<ezDynamicDefaultValueAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ClassSource", m_sClassSource),
    EZ_MEMBER_PROPERTY("ClassType", m_sClassType),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
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

ezManipulatorAttribute::ezManipulatorAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/,
  const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/, const char* szProperty6 /*= nullptr*/)
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
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("scale", m_fSizeScale),
    EZ_MEMBER_PROPERTY("recenter", m_bRecenterParent),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, bool, float),
    EZ_CONSTRUCTOR_PROPERTY(const char*, bool, float),
    EZ_CONSTRUCTOR_PROPERTY(const char*, bool, float, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, bool, float, const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBoxManipulatorAttribute::ezBoxManipulatorAttribute()
  : ezManipulatorAttribute(nullptr)
{
}

ezBoxManipulatorAttribute::ezBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty, const char* szRotationProperty)
  : ezManipulatorAttribute(szSizeProperty, szOffsetProperty, szRotationProperty)
{
  m_bRecenterParent = bRecenterParent;
  m_fSizeScale = fSizeScale;
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

ezNonUniformBoxManipulatorAttribute::ezNonUniformBoxManipulatorAttribute(
  const char* szNegXProp, const char* szPosXProp, const char* szNegYProp, const char* szPosYProp, const char* szNegZProp, const char* szPosZProp)
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

ezTransformManipulatorAttribute::ezTransformManipulatorAttribute(
  const char* szTranslateProperty, const char* szRotateProperty, const char* szScaleProperty, const char* szOffsetTranslation, const char* szOffsetRotation)
  : ezManipulatorAttribute(szTranslateProperty, szRotateProperty, szScaleProperty, szOffsetTranslation, szOffsetRotation)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoneManipulatorAttribute, 1, ezRTTIDefaultAllocator<ezBoneManipulatorAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBoneManipulatorAttribute::ezBoneManipulatorAttribute()
  : ezManipulatorAttribute(nullptr)
{
}

ezBoneManipulatorAttribute::ezBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo)
  : ezManipulatorAttribute(szTransformProperty, szBindTo)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezVisualizerAnchor, 1)
  EZ_BITFLAGS_CONSTANTS(ezVisualizerAnchor::Center, ezVisualizerAnchor::PosX, ezVisualizerAnchor::NegX, ezVisualizerAnchor::PosY, ezVisualizerAnchor::NegY, ezVisualizerAnchor::PosZ, ezVisualizerAnchor::NegZ)
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

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
    EZ_MEMBER_PROPERTY("Property5", m_sProperty5),
    EZ_BITFLAGS_MEMBER_PROPERTY("Anchor", ezVisualizerAnchor, m_Anchor),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualizerAttribute::ezVisualizerAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/,
  const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/)
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
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    EZ_MEMBER_PROPERTY("SizeScale", m_fSizeScale),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float),
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

ezBoxVisualizerAttribute::ezBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, ezBitflags<ezVisualizerAnchor> anchor /*= ezVisualizerAnchor::Center*/, ezVec3 vOffsetOrScale /*= ezVec3::ZeroVector*/, const char* szOffsetProperty /*= nullptr*/, const char* szRotationProperty /*= nullptr*/)
  : ezVisualizerAttribute(szSizeProperty, szColorProperty, szOffsetProperty, szRotationProperty)
{
  m_Color = fixedColor;
  m_vOffsetOrScale = vOffsetOrScale;
  m_Anchor = anchor;
  m_fSizeScale = fSizeScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSphereVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezSphereVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const ezColor&),
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

ezSphereVisualizerAttribute::ezSphereVisualizerAttribute(const char* szRadiusProperty, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, ezBitflags<ezVisualizerAnchor> anchor /*= ezVisualizerAnchor::Center*/, ezVec3 vOffsetOrScale /*= ezVec3::ZeroVector*/, const char* szOffsetProperty /*= nullptr*/)
  : ezVisualizerAttribute(szRadiusProperty, szColorProperty, szOffsetProperty)
{
  m_Color = fixedColor;
  m_vOffsetOrScale = vOffsetOrScale;
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCapsuleVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezCapsuleVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_Color),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCapsuleVisualizerAttribute::ezCapsuleVisualizerAttribute()
  : ezVisualizerAttribute(nullptr)
{
}

ezCapsuleVisualizerAttribute::ezCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, ezBitflags<ezVisualizerAnchor> anchor /*= ezVisualizerAnchor::Center*/)
  : ezVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty)
{
  m_Color = fixedColor;
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCylinderVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezCylinderVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>, ezVec3),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const ezColor&, const char*, ezBitflags<ezVisualizerAnchor>),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCylinderVisualizerAttribute::ezCylinderVisualizerAttribute()
  : ezVisualizerAttribute(nullptr)
{
}

ezCylinderVisualizerAttribute::ezCylinderVisualizerAttribute(ezEnum<ezBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, ezBitflags<ezVisualizerAnchor> anchor /*= ezVisualizerAnchor::Center*/, ezVec3 vOffsetOrScale /*= ezVec3::ZeroVector*/, const char* szOffsetProperty /*= nullptr*/)
  : ezVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty)
{
  m_Color = fixedColor;
  m_vOffsetOrScale = vOffsetOrScale;
  m_Axis = axis;
  m_Anchor = anchor;
}

ezCylinderVisualizerAttribute::ezCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, ezBitflags<ezVisualizerAnchor> anchor /*= ezVisualizerAnchor::Center*/, ezVec3 vOffsetOrScale /*= ezVec3::ZeroVector()*/, const char* szOffsetProperty /*= nullptr*/)
  : ezVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty, szAxisProperty)
{
  m_Color = fixedColor;
  m_vOffsetOrScale = vOffsetOrScale;
  m_Axis = ezBasisAxis::Default;
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDirectionVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezDirectionVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis),
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("Scale", m_fScale)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float, const ezColor&, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, float),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&, const char*, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float, const ezColor&),
    EZ_CONSTRUCTOR_PROPERTY(const char*, float),
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
  m_Color = ezColor::White;
}

ezDirectionVisualizerAttribute::ezDirectionVisualizerAttribute(ezEnum<ezBasisAxis> axis, float fScale, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/)
  : ezVisualizerAttribute(szColorProperty, szLengthProperty)
{
  m_Axis = axis;
  m_fScale = fScale;
  m_Color = fixedColor;
}

ezDirectionVisualizerAttribute::ezDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/)
  : ezVisualizerAttribute(szColorProperty, szLengthProperty, szAxisProperty)
{
  m_Axis = ezBasisAxis::PositiveX;
  m_fScale = fScale;
  m_Color = fixedColor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConeVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezConeVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis),
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("Scale", m_fScale),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, float, const char*, const ezColor&, const char*),
    EZ_CONSTRUCTOR_PROPERTY(ezEnum<ezBasisAxis>, const char*, float, const char*, const ezColor&),
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
  const char* szRadiusProperty, const ezColor& fixedColor /*= ezColorScheme::LightUI(ezColorScheme::Grape)*/, const char* szColorProperty)
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

ezCameraVisualizerAttribute::ezCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty,
  const char* szNearPlaneProperty, const char* szFarPlaneProperty)
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

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptableFunctionAttribute, 1, ezRTTIDefaultAllocator<ezScriptableFunctionAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Arg1", m_sArg1),
    EZ_MEMBER_PROPERTY("Arg2", m_sArg2),
    EZ_MEMBER_PROPERTY("Arg3", m_sArg3),
    EZ_MEMBER_PROPERTY("Arg4", m_sArg4),
    EZ_MEMBER_PROPERTY("Arg5", m_sArg5),
    EZ_MEMBER_PROPERTY("Arg6", m_sArg6),
    EZ_MEMBER_PROPERTY("ArgType1", m_ArgType1),
    EZ_MEMBER_PROPERTY("ArgType2", m_ArgType2),
    EZ_MEMBER_PROPERTY("ArgType3", m_ArgType3),
    EZ_MEMBER_PROPERTY("ArgType4", m_ArgType4),
    EZ_MEMBER_PROPERTY("ArgType5", m_ArgType5),
    EZ_MEMBER_PROPERTY("ArgType6", m_ArgType6),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScriptableFunctionAttribute::ezScriptableFunctionAttribute(ArgType argType1 /*= In*/, const char* szArg1 /*= nullptr*/, ArgType argType2 /*= In*/,
  const char* szArg2 /*= nullptr*/, ArgType argType3 /*= In*/, const char* szArg3 /*= nullptr*/, ArgType argType4 /*= In*/,
  const char* szArg4 /*= nullptr*/, ArgType argType5 /*= In*/, const char* szArg5 /*= nullptr*/, ArgType argType6 /*= In*/,
  const char* szArg6 /*= nullptr*/)
{
  m_sArg1 = szArg1;
  m_sArg2 = szArg2;
  m_sArg3 = szArg3;
  m_sArg4 = szArg4;
  m_sArg5 = szArg5;
  m_sArg6 = szArg6;

  m_ArgType1 = argType1;
  m_ArgType2 = argType2;
  m_ArgType3 = argType3;
  m_ArgType4 = argType4;
  m_ArgType5 = argType5;
  m_ArgType6 = argType6;
}

const char* ezScriptableFunctionAttribute::GetArgumentName(ezUInt32 uiIndex) const
{
  switch (uiIndex)
  {
    case 0:
      return m_sArg1;
    case 1:
      return m_sArg2;
    case 2:
      return m_sArg3;
    case 3:
      return m_sArg4;
    case 4:
      return m_sArg5;
    case 5:
      return m_sArg6;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

ezScriptableFunctionAttribute::ArgType ezScriptableFunctionAttribute::GetArgumentType(ezUInt32 uiIndex) const
{
  switch (uiIndex)
  {
    case 0:
      return (ArgType)m_ArgType1;
    case 1:
      return (ArgType)m_ArgType2;
    case 2:
      return (ArgType)m_ArgType3;
    case 3:
      return (ArgType)m_ArgType4;
    case 4:
      return (ArgType)m_ArgType5;
    case 5:
      return (ArgType)m_ArgType6;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ArgType::In;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptMappingAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptMappingAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Mapping", m_iMapping)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpAttribute, 1, ezRTTIDefaultAllocator<ezLongOpAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sOpTypeName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(),
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectReferenceAttribute, 1, ezRTTIDefaultAllocator<ezGameObjectReferenceAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyAttributes);
