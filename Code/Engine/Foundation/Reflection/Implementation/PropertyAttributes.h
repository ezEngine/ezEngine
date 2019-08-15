#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Base class of all attributes can be used to decorate a RTTI property.
class EZ_FOUNDATION_DLL ezPropertyAttribute : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAttribute, ezReflectedClass);
};

/// \brief A property attribute that indicates that the property may not be modified through the UI
class EZ_FOUNDATION_DLL ezReadOnlyAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReadOnlyAttribute, ezPropertyAttribute);
};

/// \brief A property attribute that indicates that the property is not to be shown in the UI
class EZ_FOUNDATION_DLL ezHiddenAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHiddenAttribute, ezPropertyAttribute);
};

/// \brief A property attribute that indicates that the property is not to be serialized
/// and whatever it points to only exists temporarily while running or in editor.
class EZ_FOUNDATION_DLL ezTemporaryAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTemporaryAttribute, ezPropertyAttribute);
};

/// \brief Used to categorize types (e.g. add component menu)
class EZ_FOUNDATION_DLL ezCategoryAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCategoryAttribute, ezPropertyAttribute);

public:
  ezCategoryAttribute() {}
  ezCategoryAttribute(const char* szCategory) { m_sCategory = szCategory; }

  const char* GetCategory() const { return m_sCategory; }

private:
  ezUntrackedString m_sCategory;
};

/// \brief Used to categorize types (e.g. add component menu)
class EZ_FOUNDATION_DLL ezTitleAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTitleAttribute, ezPropertyAttribute);

public:
  ezTitleAttribute() {}
  ezTitleAttribute(const char* szTitle) { m_sTitle = szTitle; }

  const char* GetTitle() const { return m_sTitle; }

private:
  ezUntrackedString m_sTitle;
};

/// \brief Used to colorize types
class EZ_FOUNDATION_DLL ezColorAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorAttribute, ezPropertyAttribute);

public:
  ezColorAttribute() {}
  ezColorAttribute(ezColor color) { m_Color = color; }

  ezColor GetColor() const { return m_Color; }

private:
  ezColor m_Color;
};

/// \brief A property attribute that indicates that the alpha channel of an ezColorGammaUB or ezColor should be exposed in the UI.
class EZ_FOUNDATION_DLL ezExposeColorAlphaAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposeColorAlphaAttribute, ezPropertyAttribute);
};

/// \brief Used for any property shown as a line edit (int, float, vector etc).
class EZ_FOUNDATION_DLL ezSuffixAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSuffixAttribute, ezPropertyAttribute);

public:
  ezSuffixAttribute() {}
  ezSuffixAttribute(const char* szSuffix) { m_sSuffix = szSuffix; }

  const char* GetSuffix() const { return m_sSuffix; }

private:
  ezUntrackedString m_sSuffix;
};

/// \brief Used to show a text instead of the minimum value of a property.
class EZ_FOUNDATION_DLL ezMinValueTextAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMinValueTextAttribute, ezPropertyAttribute);

public:
  ezMinValueTextAttribute() {}
  ezMinValueTextAttribute(const char* szText) { m_sText = szText; }

  const char* GetText() const { return m_sText; }

private:
  ezUntrackedString m_sText;
};

/// \brief Sets the default value of the property.
class EZ_FOUNDATION_DLL ezDefaultValueAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDefaultValueAttribute, ezPropertyAttribute);

public:
  ezDefaultValueAttribute() {}
  ezDefaultValueAttribute(const ezVariant& value) { m_Value = value; }

  const ezVariant& GetValue() const { return m_Value; }

private:
  ezVariant m_Value;
};

/// \brief A property attribute that allows to define min and max values for the UI. Min or max may be set to an invalid variant to indicate
/// unbounded values in one direction.
class EZ_FOUNDATION_DLL ezClampValueAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClampValueAttribute, ezPropertyAttribute);

public:
  ezClampValueAttribute() {}
  ezClampValueAttribute(const ezVariant& min, const ezVariant& max)
  {
    m_MinValue = min;
    m_MaxValue = max;
  }

  const ezVariant& GetMinValue() const { return m_MinValue; }
  const ezVariant& GetMaxValue() const { return m_MaxValue; }

private:
  ezVariant m_MinValue;
  ezVariant m_MaxValue;
};


/// \brief Derive from this class if you want to define an attribute that replaces the property type widget.
///
/// Using this attribute affects both member properties as well as elements in a container but not the container widget.
/// When creating a property widget, the property grid will look for an attribute of this type and use
/// its type to look for a factory creator in ezRttiMappedObjectFactory<ezQtPropertyWidget>.
/// E.g. ezRttiMappedObjectFactory<ezQtPropertyWidget>::RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), FileBrowserCreator);
/// will replace the property widget for all properties that use ezFileBrowserAttribute.
class EZ_FOUNDATION_DLL ezTypeWidgetAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeWidgetAttribute, ezPropertyAttribute);
};

/// \brief Derive from this class if you want to define an attribute that replaces the property widget of containers.
///
/// Using this attribute affects the container widget but not container elements.
/// Only derive from this class if you want to replace the container widget itself, in every other case
/// prefer to use ezTypeWidgetAttribute.
class EZ_FOUNDATION_DLL ezContainerWidgetAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezContainerWidgetAttribute, ezPropertyAttribute);
};

/// \brief Add this attribute to a tag set member property to make it use the tag set editor
/// and define the categories it will use as a ; separated list of category names.
///
/// Usage: EZ_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new ezTagSetWidgetAttribute("Category1;Category2")),
class EZ_FOUNDATION_DLL ezTagSetWidgetAttribute : public ezContainerWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTagSetWidgetAttribute, ezContainerWidgetAttribute);

public:
  ezTagSetWidgetAttribute() {}
  ezTagSetWidgetAttribute(const char* szTagFilter) { m_sTagFilter = szTagFilter; }

  const char* GetTagFilter() const { return m_sTagFilter; }

private:
  ezUntrackedString m_sTagFilter;
};

/// \brief Add this attribute to a variant map property to make it map to the exposed parameters
/// of an asset. For this, the member property name of the asset reference needs to be passed in.
/// The exposed parameters of the currently set asset on that property will be used as the source.
///
/// Usage:
/// EZ_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new ezAssetBrowserAttribute("Particle
/// Effect")), EZ_MAP_ACCESSOR_PROPERTY("Parameters",...)->AddAttributes(new ezExposedParametersAttribute("Effect")),
class EZ_FOUNDATION_DLL ezExposedParametersAttribute : public ezContainerWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposedParametersAttribute, ezContainerWidgetAttribute);

public:
  ezExposedParametersAttribute() {}
  ezExposedParametersAttribute(const char* szParametersSource) { m_sParametersSource = szParametersSource; }

  const char* GetParametersSource() const { return m_sParametersSource; }

private:
  ezUntrackedString m_sParametersSource;
};


/// \brief Sets the allowed actions on a container.
class EZ_FOUNDATION_DLL ezContainerAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezContainerAttribute, ezPropertyAttribute);

public:
  ezContainerAttribute() {}
  ezContainerAttribute(bool bCanAdd, bool bCanDelete, bool bCanMove)
  {
    m_bCanAdd = bCanAdd;
    m_bCanDelete = bCanDelete;
    m_bCanMove = bCanMove;
  }

  bool CanAdd() const { return m_bCanAdd; }
  bool CanDelete() const { return m_bCanDelete; }
  bool CanMove() const { return m_bCanMove; }

private:
  bool m_bCanAdd;
  bool m_bCanDelete;
  bool m_bCanMove;
};

/// \brief Limits setting of pointer properties to derived types that have the given constant property and value
///
/// The szConstantValueProperty is a sibling property of the property this attribute is assigned to,
class EZ_FOUNDATION_DLL ezConstrainPointerAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConstrainPointerAttribute, ezPropertyAttribute);

public:
  ezConstrainPointerAttribute() {}
  ezConstrainPointerAttribute(const char* szConstantName, const char* szConstantValueProperty)
  {
    m_sConstantName = szConstantName;
    m_sConstantValueProperty = szConstantValueProperty;
  }

  const ezUntrackedString& GetConstantName() const { return m_sConstantName; }
  const ezUntrackedString& GetConstantValueProperty() const { return m_sConstantValueProperty; }

private:
  ezUntrackedString m_sConstantName;
  ezUntrackedString m_sConstantValueProperty;
};

/// \brief A property attribute that indicates that the string property should display a file browsing button.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: EZ_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new ezFileBrowserAttribute("Choose a File", "*.txt")),
class EZ_FOUNDATION_DLL ezFileBrowserAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFileBrowserAttribute, ezTypeWidgetAttribute);

public:
  ezFileBrowserAttribute() {}
  ezFileBrowserAttribute(const char* szDialogTitle, const char* szTypeFilter)
  {
    m_sDialogTitle = szDialogTitle;
    m_sTypeFilter = szTypeFilter;
  }

  const char* GetDialogTitle() const { return m_sDialogTitle; }
  const char* GetTypeFilter() const { return m_sTypeFilter; }

private:
  ezUntrackedString m_sDialogTitle;
  ezUntrackedString m_sTypeFilter;
};

/// \brief A property attribute that indicates that the string property is actually an asset reference.
///
/// Allows to specify the allowed asset types, separated with ;
/// Usage: EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D;Texture 3D")),
class EZ_FOUNDATION_DLL ezAssetBrowserAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetBrowserAttribute, ezTypeWidgetAttribute);

public:
  ezAssetBrowserAttribute() {}
  ezAssetBrowserAttribute(const char* szTypeFilter) { SetTypeFilter(szTypeFilter); }

  void SetTypeFilter(const char* szTypeFilter)
  {
    ezStringBuilder sTemp(";", szTypeFilter, ";");
    m_sTypeFilter = sTemp;
  }
  const char* GetTypeFilter() const { return m_sTypeFilter; }

private:
  ezUntrackedString m_sTypeFilter;
};

/// \brief Can be used on integer properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See ezDynamicEnum for details.
class EZ_FOUNDATION_DLL ezDynamicEnumAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicEnumAttribute, ezTypeWidgetAttribute);

public:
  ezDynamicEnumAttribute() {}
  ezDynamicEnumAttribute(const char* szDynamicEnumName) { m_sDynamicEnumName = szDynamicEnumName; }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  ezUntrackedString m_sDynamicEnumName;
};

/// \brief Can be used on string properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See ezDynamicStringEnum for details.
class EZ_FOUNDATION_DLL ezDynamicStringEnumAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicStringEnumAttribute, ezTypeWidgetAttribute);

public:
  ezDynamicStringEnumAttribute() {}
  ezDynamicStringEnumAttribute(const char* szDynamicEnumName) { m_sDynamicEnumName = szDynamicEnumName; }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  ezUntrackedString m_sDynamicEnumName;
};


//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezManipulatorAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezManipulatorAttribute, ezPropertyAttribute);

public:
  ezManipulatorAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr,
    const char* szProperty4 = nullptr, const char* szProperty5 = nullptr, const char* szProperty6 = nullptr);

  ezUntrackedString m_sProperty1;
  ezUntrackedString m_sProperty2;
  ezUntrackedString m_sProperty3;
  ezUntrackedString m_sProperty4;
  ezUntrackedString m_sProperty5;
  ezUntrackedString m_sProperty6;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezSphereManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSphereManipulatorAttribute, ezManipulatorAttribute);

public:
  ezSphereManipulatorAttribute();
  ezSphereManipulatorAttribute(const char* szOuterRadiusProperty, const char* szInnerRadiusProperty = nullptr);

  const ezUntrackedString& GetOuterRadiusProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetInnerRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezCapsuleManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCapsuleManipulatorAttribute, ezManipulatorAttribute);

public:
  ezCapsuleManipulatorAttribute();
  ezCapsuleManipulatorAttribute(const char* szHeightProperty, const char* szRadiusProperty);

  const ezUntrackedString& GetLengthProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezBoxManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoxManipulatorAttribute, ezManipulatorAttribute);

public:
  ezBoxManipulatorAttribute();
  ezBoxManipulatorAttribute(const char* szSizeProperty, const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  const ezUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetOffsetProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetRotationProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezNonUniformBoxManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNonUniformBoxManipulatorAttribute, ezManipulatorAttribute);

public:
  ezNonUniformBoxManipulatorAttribute();
  ezNonUniformBoxManipulatorAttribute(const char* szNegXProp, const char* szPosXProp, const char* szNegYProp, const char* szPosYProp,
    const char* szNegZProp, const char* szPosZProp);
  ezNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ);

  bool HasSixAxis() const { return !m_sProperty4.IsEmpty(); }

  const ezUntrackedString& GetNegXProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetPosXProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetNegYProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetPosYProperty() const { return m_sProperty4; }
  const ezUntrackedString& GetNegZProperty() const { return m_sProperty5; }
  const ezUntrackedString& GetPosZProperty() const { return m_sProperty6; }

  const ezUntrackedString& GetSizeXProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetSizeYProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetSizeZProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezConeLengthManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConeLengthManipulatorAttribute, ezManipulatorAttribute);

public:
  ezConeLengthManipulatorAttribute();
  ezConeLengthManipulatorAttribute(const char* szRadiusProperty);

  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezConeAngleManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConeAngleManipulatorAttribute, ezManipulatorAttribute);

public:
  ezConeAngleManipulatorAttribute();
  ezConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale = 1.0f, const char* szRadiusProperty = nullptr);

  const ezUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty2; }

  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezTransformManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransformManipulatorAttribute, ezManipulatorAttribute);

public:
  ezTransformManipulatorAttribute();
  ezTransformManipulatorAttribute(
    const char* szTranslateProperty, const char* szRotateProperty = nullptr, const char* szScaleProperty = nullptr);

  const ezUntrackedString& GetTranslateProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRotateProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetScaleProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezVisualizerAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualizerAttribute, ezPropertyAttribute);

public:
  ezVisualizerAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr,
    const char* szProperty4 = nullptr, const char* szProperty5 = nullptr);

  ezUntrackedString m_sProperty1;
  ezUntrackedString m_sProperty2;
  ezUntrackedString m_sProperty3;
  ezUntrackedString m_sProperty4;
  ezUntrackedString m_sProperty5;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezBoxVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoxVisualizerAttribute, ezVisualizerAttribute);

public:
  ezBoxVisualizerAttribute();
  ezBoxVisualizerAttribute(const char* szSizeProperty, const char* szColorProperty = nullptr,
    const ezColor& fixedColor = ezColor::MediumVioletRed, const char* szOffsetProperty = nullptr,
    ezVec3 fixedOffset = ezVec3::ZeroVector());
  ezBoxVisualizerAttribute(const char* szSizeProperty, const char* szOffsetProperty, const char* szRotationProperty,
    const char* szColorProperty = nullptr, const ezColor& fixedColor = ezColor::MediumVioletRed);

  const ezUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetOffsetProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetRotationProperty() const { return m_sProperty4; }

  ezColor m_Color;
  ezVec3 m_vOffset;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezSphereVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSphereVisualizerAttribute, ezVisualizerAttribute);

public:
  ezSphereVisualizerAttribute();
  ezSphereVisualizerAttribute(const char* szRadiusProperty, const char* szColorProperty = nullptr,
    const ezColor& fixedColor = ezColor::MediumVioletRed, const char* szOffsetProperty = nullptr,
    ezVec3 fixedOffset = ezVec3::ZeroVector());

  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetOffsetProperty() const { return m_sProperty3; }

  ezColor m_Color;
  ezVec3 m_vOffset;
};


//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezCapsuleVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCapsuleVisualizerAttribute, ezVisualizerAttribute);

public:
  ezCapsuleVisualizerAttribute();
  ezCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const char* szColorProperty);
  ezCapsuleVisualizerAttribute(
    const char* szHeightProperty, const char* szRadiusProperty, const ezColor& fixedColor = ezColor::MediumVioletRed);

  const ezUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty3; }

  ezColor m_Color;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezCylinderVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCylinderVisualizerAttribute, ezVisualizerAttribute);

public:
  ezCylinderVisualizerAttribute();
  ezCylinderVisualizerAttribute(ezEnum<ezBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty,
    const char* szColorProperty = nullptr, const ezColor& fixedColor = ezColor::MediumVioletRed, const char* szOffsetProperty = nullptr,
    ezVec3 fixedOffset = ezVec3::ZeroVector());

  const ezUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetOffsetProperty() const { return m_sProperty4; }

  ezEnum<ezBasisAxis> m_Axis;
  ezColor m_Color;
  ezVec3 m_vOffset;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezDirectionVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDirectionVisualizerAttribute, ezVisualizerAttribute);

public:
  ezDirectionVisualizerAttribute();
  ezDirectionVisualizerAttribute(
    ezEnum<ezBasisAxis> axis, float fScale, const char* szColorProperty, const char* szLengthProperty = nullptr);
  ezDirectionVisualizerAttribute(
    ezEnum<ezBasisAxis> axis, float fScale, const ezColor& fixedColor = ezColor::MediumVioletRed, const char* szLengthProperty = nullptr);

  const ezUntrackedString& GetColorProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetLengthProperty() const { return m_sProperty2; }

  ezEnum<ezBasisAxis> m_Axis;
  ezColor m_Color;
  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezConeVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConeVisualizerAttribute, ezVisualizerAttribute);

public:
  ezConeVisualizerAttribute();

  /// \brief Attribute to add on an RTTI type to add a cone visualizer for specific properties.
  ///
  /// szRadiusProperty may be nullptr, in which case it is assumed to be 1
  /// fScale will be multiplied with value of szRadiusProperty to determine the size of the cone
  /// szColorProperty may be nullptr. In this case it is ignored and fixedColor is used instead.
  /// fixedColor is ignored if szColorProperty is valid.
  ezConeVisualizerAttribute(ezEnum<ezBasisAxis> axis, const char* szAngleProperty, float fScale, const char* szRadiusProperty,
    const char* szColorProperty = nullptr, const ezColor& fixedColor = ezColor::MediumVioletRed);

  const ezUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty3; }

  ezEnum<ezBasisAxis> m_Axis;
  ezColor m_Color;
  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezCameraVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCameraVisualizerAttribute, ezVisualizerAttribute);

public:
  ezCameraVisualizerAttribute();

  /// \brief Attribute to add on an RTTI type to add a camera cone visualizer.
  ezCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty,
    const char* szNearPlaneProperty, const char* szFarPlaneProperty);

  const ezUntrackedString& GetModeProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetFovProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetOrthoDimProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetNearPlaneProperty() const { return m_sProperty4; }
  const ezUntrackedString& GetFarPlaneProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

// Implementation moved here as it requires ezPropertyAttribute to be fully defined.
template <typename Type>
const Type* ezRTTI::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  if (GetParentType() != nullptr)
    return GetParentType()->GetAttributeByType<Type>();
  else
    return nullptr;
}

template <typename Type>
const Type* ezAbstractProperty::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

/// \brief A property attribute that specifies the max size of an array. If it is reached, no further elemets are allowed to be added.
class EZ_FOUNDATION_DLL ezMaxArraySizeAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaxArraySizeAttribute, ezPropertyAttribute);

public:
  ezMaxArraySizeAttribute() {}
  ezMaxArraySizeAttribute(ezUInt32 uiMaxSize) { m_uiMaxSize = uiMaxSize; }

  const ezUInt32& GetMaxSize() const { return m_uiMaxSize; }

private:
  ezUInt32 m_uiMaxSize;
};

//////////////////////////////////////////////////////////////////////////

/// \brief If this attribute is set, the UI is encouraged to prevent the user from creating duplicates of the same thing.
///
/// For arrays of objects this means that multiple objects of the same type are not allowed.
class EZ_FOUNDATION_DLL ezPreventDuplicatesAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPreventDuplicatesAttribute, ezPropertyAttribute);

public:
  ezPreventDuplicatesAttribute() {}
};

//////////////////////////////////////////////////////////////////////////

/// \brief Attribute for ezMessages to instruct the visual script framework to automatically generate a node for sending this type of
/// message
class EZ_FOUNDATION_DLL ezAutoGenVisScriptMsgSender : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAutoGenVisScriptMsgSender, ezPropertyAttribute);
};

/// \brief Attribute for ezMessages to instruct the visual script framework to automatically generate a node for handling this type of
/// message
class EZ_FOUNDATION_DLL ezAutoGenVisScriptMsgHandler : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAutoGenVisScriptMsgHandler, ezPropertyAttribute);
};

/// \brief Attribute to mark a function up to be exposed to the scripting system. Arguments specify the names of the function parameters.
class EZ_FOUNDATION_DLL ezScriptableFunctionAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptableFunctionAttribute, ezPropertyAttribute);

  ezScriptableFunctionAttribute(const char* szArg1 = nullptr, const char* szArg2 = nullptr, const char* szArg3 = nullptr, const char* szArg4 = nullptr, const char* szArg5 = nullptr, const char* szArg6 = nullptr)
  {
    m_sArg1 = szArg1;
    m_sArg2 = szArg2;
    m_sArg3 = szArg3;
    m_sArg4 = szArg4;
    m_sArg5 = szArg5;
    m_sArg6 = szArg6;
  }

  const char* GetArgumentName(ezUInt32 index) const
  {
    switch (index)
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

    return nullptr;
  }

  ezUntrackedString m_sArg1;
  ezUntrackedString m_sArg2;
  ezUntrackedString m_sArg3;
  ezUntrackedString m_sArg4;
  ezUntrackedString m_sArg5;
  ezUntrackedString m_sArg6;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to mark that a component provides functionality that is executed with a long operation in the editor.
///
/// \a szOpTypeName must be the class name of a class derived from ezLongOpProxy.
/// Once a component is added to a scene with this attribute, the named long op will appear in the UI and can be executed.
///
/// The automatic registration is done by ezLongOpsAdapter
class EZ_FOUNDATION_DLL ezLongOpAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpAttribute, ezPropertyAttribute);

public:
  ezLongOpAttribute() = default;
  ezLongOpAttribute(const char* szOpTypeName) { m_sOpTypeName = szOpTypeName; }

  ezUntrackedString m_sOpTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A property attribute that indicates that the string property is actually a game object reference.
class EZ_FOUNDATION_DLL ezGameObjectReferenceAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectReferenceAttribute, ezTypeWidgetAttribute);

public:
  ezGameObjectReferenceAttribute() = default;
};
