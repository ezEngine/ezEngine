#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Math/ColorScheme.h>
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
  ezCategoryAttribute() = default;
  ezCategoryAttribute(const char* szCategory)
    : m_sCategory(szCategory)
  {
  }

  const char* GetCategory() const { return m_sCategory; }

private:
  ezUntrackedString m_sCategory;
};

/// \brief A property attribute that indicates that this feature is still in development and should not be shown to all users.
class EZ_FOUNDATION_DLL ezInDevelopmentAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInDevelopmentAttribute, ezPropertyAttribute);

public:
  enum Phase
  {
    Alpha,
    Beta
  };

  ezInDevelopmentAttribute() = default;
  ezInDevelopmentAttribute(ezInt32 iPhase) { m_Phase = iPhase; }

  const char* GetString() const;

  ezInt32 m_Phase = Phase::Beta;
};


/// \brief Used for dynamic titles of visual script nodes.
/// E.g. "Set Bool Property '{Name}'" will allow the title to by dynamic
/// by reading the current value of the 'Name' property.
class EZ_FOUNDATION_DLL ezTitleAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTitleAttribute, ezPropertyAttribute);

public:
  ezTitleAttribute() = default;
  ezTitleAttribute(const char* szTitle)
    : m_sTitle(szTitle)
  {
  }

  const char* GetTitle() const { return m_sTitle; }

private:
  ezUntrackedString m_sTitle;
};

/// \brief Used to colorize types
class EZ_FOUNDATION_DLL ezColorAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorAttribute, ezPropertyAttribute);

public:
  ezColorAttribute() = default;
  ezColorAttribute(const ezColor& color)
    : m_Color(color)
  {
  }
  const ezColor& GetColor() const { return m_Color; }

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
  ezSuffixAttribute() = default;
  ezSuffixAttribute(const char* szSuffix)
    : m_sSuffix(szSuffix)
  {
  }

  const char* GetSuffix() const { return m_sSuffix; }

private:
  ezUntrackedString m_sSuffix;
};

/// \brief Used to show a text instead of the minimum value of a property.
class EZ_FOUNDATION_DLL ezMinValueTextAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMinValueTextAttribute, ezPropertyAttribute);

public:
  ezMinValueTextAttribute() = default;
  ezMinValueTextAttribute(const char* szText)
    : m_sText(szText)
  {
  }

  const char* GetText() const { return m_sText; }

private:
  ezUntrackedString m_sText;
};

/// \brief Sets the default value of the property.
class EZ_FOUNDATION_DLL ezDefaultValueAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDefaultValueAttribute, ezPropertyAttribute);

public:
  ezDefaultValueAttribute() = default;

  ezDefaultValueAttribute(const ezVariant& value)
    : m_Value(value)
  {
  }

  ezDefaultValueAttribute(ezInt32 value)
    : m_Value(value)
  {
  }

  ezDefaultValueAttribute(float value)
    : m_Value(value)
  {
  }

  ezDefaultValueAttribute(double value)
    : m_Value(value)
  {
  }

  ezDefaultValueAttribute(ezStringView value)
    : m_Value(ezVariant(value, false))
  {
  }

  ezDefaultValueAttribute(const char* value)
    : m_Value(ezVariant(ezStringView(value), false))
  {
  }

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
  ezClampValueAttribute() = default;
  ezClampValueAttribute(const ezVariant& min, const ezVariant& max)
    : m_MinValue(min)
    , m_MaxValue(max)
  {
  }

  const ezVariant& GetMinValue() const { return m_MinValue; }
  const ezVariant& GetMaxValue() const { return m_MaxValue; }

protected:
  ezVariant m_MinValue;
  ezVariant m_MaxValue;
};

/// \brief Used to categorize properties into groups
class EZ_FOUNDATION_DLL ezGroupAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGroupAttribute, ezPropertyAttribute);

public:
  ezGroupAttribute();
  ezGroupAttribute(const char* szGroup, float fOrder = -1.0f);
  ezGroupAttribute(const char* szGroup, const char* szIconName, float fOrder = -1.0f);

  const char* GetGroup() const { return m_sGroup; }
  const char* GetIconName() const { return m_sIconName; }
  float GetOrder() const { return m_fOrder; }

private:
  ezUntrackedString m_sGroup;
  ezUntrackedString m_sIconName;
  float m_fOrder = -1.0f;
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
  ezTagSetWidgetAttribute() = default;
  ezTagSetWidgetAttribute(const char* szTagFilter)
    : m_sTagFilter(szTagFilter)
  {
  }

  const char* GetTagFilter() const { return m_sTagFilter; }

private:
  ezUntrackedString m_sTagFilter;
};

/// \brief This attribute indicates that a widget should not use temporary transactions when changing the value.
class EZ_FOUNDATION_DLL ezNoTemporaryTransactionsAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNoTemporaryTransactionsAttribute, ezPropertyAttribute);
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
  ezExposedParametersAttribute() = default;
  ezExposedParametersAttribute(const char* szParametersSource)
    : m_sParametersSource(szParametersSource)
  {
  }

  const char* GetParametersSource() const { return m_sParametersSource; }

private:
  ezUntrackedString m_sParametersSource;
};

/// \brief Add this attribute to an embedded class or container property to make it retrieve its default values from a dynamic meta info object on an asset.
///
/// The default values are retrieved from the asset meta data of the currently set asset on that property.
///
/// Usage:
/// EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
///
/// // Use this if the embedded class m_SkeletonMetaData is of type ezSkeletonMetaData.
/// EZ_MEMBER_PROPERTY("SkeletonMetaData", m_SkeletonMetaData)->AddAttributes(new ezDynamicDefaultValueAttribute("Skeleton", "ezSkeletonMetaData")),
///
/// // Use this if you don't want embed the entire meta object but just some container of it. In this case the LocalBones container must match in type to the property 'BonesArrayNameInMetaData' in the meta data type 'ezSkeletonMetaData'.
/// EZ_MAP_MEMBER_PROPERTY("LocalBones", m_Bones)->AddAttributes(new ezDynamicDefaultValueAttribute("Skeleton", "ezSkeletonMetaData", "BonesArrayNameInMetaData")),
class EZ_FOUNDATION_DLL ezDynamicDefaultValueAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicDefaultValueAttribute, ezTypeWidgetAttribute);

public:
  ezDynamicDefaultValueAttribute() = default;
  ezDynamicDefaultValueAttribute(const char* szClassSource,
    const char* szClassType, const char* szClassProperty = nullptr)
    : m_sClassSource(szClassSource)
    , m_sClassType(szClassType)
    , m_sClassProperty(szClassProperty)
  {
  }

  const char* GetClassSource() const { return m_sClassSource; }
  const char* GetClassType() const { return m_sClassType; }
  const char* GetClassProperty() const { return m_sClassProperty; }

private:
  ezUntrackedString m_sClassSource;
  ezUntrackedString m_sClassType;
  ezUntrackedString m_sClassProperty;
};


/// \brief Sets the allowed actions on a container.
class EZ_FOUNDATION_DLL ezContainerAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezContainerAttribute, ezPropertyAttribute);

public:
  ezContainerAttribute() = default;
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
  bool m_bCanAdd = false;
  bool m_bCanDelete = false;
  bool m_bCanMove = false;
};

/// \brief Defines how a reference set by ezFileBrowserAttribute and ezAssetBrowserAttribute is treated.
///
/// A few examples to explain the flags:
/// ## Input for a mesh: **Transform | Thumbnail**
/// * The input (e.g. fbx) is obviously needed for transforming the asset.
/// * We also can't generate a thumbnail without it.
/// * But we don't need to package it with the final game as it is not used by the runtime.
///
/// ## Material on a mesh: **Thumbnail | Package**
/// * The default material on a mesh asset is not needed to transform the mesh. As only the material reference is stored in the mesh asset, any changes to the material do not affect the transform output of the mesh.
/// * It is obviously needed for the thumbnail as that is what is displayed in it.
/// * We also need to package this reference as otherwise the runtime would fail to instantiate the mesh without errors.
///
/// ## Surface on hit prefab: **Package**
/// * Transforming a surface is not affected if the prefab it spawns on impact changes. Only the reference is stored.
/// * The set prefab does not show up in the thumbnail so it is not needed.
/// * We do, however, need to package it or otherwise the runtime would fail to spawn the prefab on impact.
///
/// As a rule of thumb (also the default for each):
/// * ezFileBrowserAttribute are mostly Transform and Thumbnail.
/// * ezAssetBrowserAttribute are mostly Thumbnail and Package.
struct ezDependencyFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None = 0,              ///< The reference is not needed for anything in production. An example of this is editor references that are only used at edit time, e.g. a default animation clip for a skeleton.
    Thumbnail = EZ_BIT(0), ///< This reference is a dependency to generating a thumbnail. The material references of a mesh for example.
    Transform = EZ_BIT(1), ///< This reference is a dependency to transforming this asset. The input model of a mesh for example.
    Package = EZ_BIT(2),   ///< This reference needs to be packaged as it is used at runtime by this asset. All sounds or debris generated on impact of a surface are common examples of this.
    Default = 0
  };

  struct Bits
  {
    StorageType Thumbnail : 1;
    StorageType Transform : 1;
    StorageType Package : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezDependencyFlags);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezDependencyFlags);

/// \brief A property attribute that indicates that the string property should display a file browsing button.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: EZ_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new ezFileBrowserAttribute("Choose a File", "*.txt")),
class EZ_FOUNDATION_DLL ezFileBrowserAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFileBrowserAttribute, ezTypeWidgetAttribute);

public:
  // Predefined common type filters
  static constexpr ezStringView Meshes = "*.obj;*.fbx;*.gltf;*.glb"_ezsv;
  static constexpr ezStringView MeshesWithAnimations = "*.fbx;*.gltf;*.glb"_ezsv;
  static constexpr ezStringView ImagesLdrOnly = "*.dds;*.tga;*.png;*.jpg;*.jpeg"_ezsv;
  static constexpr ezStringView ImagesHdrOnly = "*.hdr;*.exr"_ezsv;
  static constexpr ezStringView ImagesLdrAndHdr = "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr;*.exr"_ezsv;
  static constexpr ezStringView CubemapsLdrAndHdr = "*.dds;*.hdr"_ezsv;

  ezFileBrowserAttribute() = default;
  ezFileBrowserAttribute(ezStringView sDialogTitle, ezStringView sTypeFilter, ezStringView sCustomAction = {}, ezStringView sCreateTitle = {}, ezBitflags<ezDependencyFlags> depencyFlags = ezDependencyFlags::Transform | ezDependencyFlags::Thumbnail)
    : m_sDialogTitle(sDialogTitle)
    , m_sTypeFilter(sTypeFilter)
    , m_sCustomAction(sCustomAction)
    , m_sCreateTitle(sCreateTitle)
    , m_DependencyFlags(depencyFlags)
  {
  }

  ezStringView GetDialogTitle() const { return m_sDialogTitle; }
  ezStringView GetTypeFilter() const { return m_sTypeFilter; }
  ezStringView GetCustomAction() const { return m_sCustomAction; }
  ezStringView GetCreateTitle() const { return m_sCreateTitle; }
  ezBitflags<ezDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

private:
  ezUntrackedString m_sDialogTitle;
  ezUntrackedString m_sTypeFilter;
  ezUntrackedString m_sCustomAction;
  ezUntrackedString m_sCreateTitle;
  ezBitflags<ezDependencyFlags> m_DependencyFlags;
};

/// \brief Indicates that the string property should allow to browse for an file (or programs) outside the project directories.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: EZ_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new ezFileBrowserAttribute("Choose a File", "*.exe")),
class EZ_FOUNDATION_DLL ezExternalFileBrowserAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExternalFileBrowserAttribute, ezTypeWidgetAttribute);

public:
  ezExternalFileBrowserAttribute() = default;
  ezExternalFileBrowserAttribute(ezStringView sDialogTitle, ezStringView sTypeFilter)
    : m_sDialogTitle(sDialogTitle)
    , m_sTypeFilter(sTypeFilter)
  {
  }

  ezStringView GetDialogTitle() const { return m_sDialogTitle; }
  ezStringView GetTypeFilter() const { return m_sTypeFilter; }

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
  ezAssetBrowserAttribute() = default;
  ezAssetBrowserAttribute(const char* szTypeFilter, ezBitflags<ezDependencyFlags> depencyFlags = ezDependencyFlags::Thumbnail | ezDependencyFlags::Package)
    : m_DependencyFlags(depencyFlags)
  {
    SetTypeFilter(szTypeFilter);
  }

  ezAssetBrowserAttribute(const char* szTypeFilter, const char* szRequiredTag, ezBitflags<ezDependencyFlags> depencyFlags = ezDependencyFlags::Thumbnail | ezDependencyFlags::Package)
    : m_DependencyFlags(depencyFlags)
  {
    SetTypeFilter(szTypeFilter);
    m_sRequiredTag = szRequiredTag;
  }

  void SetTypeFilter(const char* szTypeFilter)
  {
    ezStringBuilder sTemp(";", szTypeFilter, ";");
    m_sTypeFilter = sTemp;
  }

  const char* GetTypeFilter() const { return m_sTypeFilter; }
  ezBitflags<ezDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

  const char* GetRequiredTag() const { return m_sRequiredTag; }

private:
  ezUntrackedString m_sTypeFilter;
  ezUntrackedString m_sRequiredTag;
  ezBitflags<ezDependencyFlags> m_DependencyFlags;
};

/// \brief Can be used on integer properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See ezDynamicEnum for details.
class EZ_FOUNDATION_DLL ezDynamicEnumAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicEnumAttribute, ezTypeWidgetAttribute);

public:
  ezDynamicEnumAttribute() = default;
  ezDynamicEnumAttribute(const char* szDynamicEnumName)
    : m_sDynamicEnumName(szDynamicEnumName)
  {
  }

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
  ezDynamicStringEnumAttribute() = default;
  ezDynamicStringEnumAttribute(const char* szDynamicEnumName)
    : m_sDynamicEnumName(szDynamicEnumName)
  {
  }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  ezUntrackedString m_sDynamicEnumName;
};

/// \brief Can be used on integer properties to display them as bitflags. The valid bitflags and their names may change at runtime.
class EZ_FOUNDATION_DLL ezDynamicBitflagsAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicBitflagsAttribute, ezTypeWidgetAttribute);

public:
  ezDynamicBitflagsAttribute() = default;
  ezDynamicBitflagsAttribute(ezStringView sDynamicName)
    : m_sDynamicBitflagsName(sDynamicName)
  {
  }

  ezStringView GetDynamicBitflagsName() const { return m_sDynamicBitflagsName; }

private:
  ezUntrackedString m_sDynamicBitflagsName;
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
  ezBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  bool m_bRecenterParent = false;
  float m_fSizeScale = 1.0f;

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
  ezNonUniformBoxManipulatorAttribute(
    const char* szNegXProp, const char* szPosXProp, const char* szNegYProp, const char* szPosYProp, const char* szNegZProp, const char* szPosZProp);
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
  ezTransformManipulatorAttribute(const char* szTranslateProperty, const char* szRotateProperty = nullptr, const char* szScaleProperty = nullptr, const char* szOffsetTranslation = nullptr, const char* szOffsetRotation = nullptr);

  const ezUntrackedString& GetTranslateProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRotateProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetScaleProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetGetOffsetTranslationProperty() const { return m_sProperty4; }
  const ezUntrackedString& GetGetOffsetRotationProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezBoneManipulatorAttribute : public ezManipulatorAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoneManipulatorAttribute, ezManipulatorAttribute);

public:
  ezBoneManipulatorAttribute();
  ezBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo);

  const ezUntrackedString& GetTransformProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

struct ezVisualizerAnchor
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Center = 0,
    PosX = EZ_BIT(0),
    NegX = EZ_BIT(1),
    PosY = EZ_BIT(2),
    NegY = EZ_BIT(3),
    PosZ = EZ_BIT(4),
    NegZ = EZ_BIT(5),

    Default = Center
  };

  struct Bits
  {
    StorageType PosX : 1;
    StorageType NegX : 1;
    StorageType PosY : 1;
    StorageType NegY : 1;
    StorageType PosZ : 1;
    StorageType NegZ : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVisualizerAnchor);

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
  ezBitflags<ezVisualizerAnchor> m_Anchor;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezBoxVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoxVisualizerAttribute, ezVisualizerAttribute);

public:
  ezBoxVisualizerAttribute();
  ezBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale = 1.0f, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr, ezBitflags<ezVisualizerAnchor> anchor = ezVisualizerAnchor::Center, ezVec3 vOffsetOrScale = ezVec3::MakeZero(), const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  const ezUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetOffsetProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetRotationProperty() const { return m_sProperty4; }

  float m_fSizeScale = 1.0f;
  ezColor m_Color;
  ezVec3 m_vOffsetOrScale;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezSphereVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSphereVisualizerAttribute, ezVisualizerAttribute);

public:
  ezSphereVisualizerAttribute();
  ezSphereVisualizerAttribute(const char* szRadiusProperty, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr, ezBitflags<ezVisualizerAnchor> anchor = ezVisualizerAnchor::Center, ezVec3 vOffsetOrScale = ezVec3::MakeZero(), const char* szOffsetProperty = nullptr);

  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetOffsetProperty() const { return m_sProperty3; }

  ezColor m_Color;
  ezVec3 m_vOffsetOrScale;
};


//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezCapsuleVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCapsuleVisualizerAttribute, ezVisualizerAttribute);

public:
  ezCapsuleVisualizerAttribute();
  ezCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr, ezBitflags<ezVisualizerAnchor> anchor = ezVisualizerAnchor::Center);

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
  ezCylinderVisualizerAttribute(ezEnum<ezBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr, ezBitflags<ezVisualizerAnchor> anchor = ezVisualizerAnchor::Center, ezVec3 vOffsetOrScale = ezVec3::MakeZero(), const char* szOffsetProperty = nullptr);
  ezCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr, ezBitflags<ezVisualizerAnchor> anchor = ezVisualizerAnchor::Center, ezVec3 vOffsetOrScale = ezVec3::MakeZero(), const char* szOffsetProperty = nullptr);

  const ezUntrackedString& GetAxisProperty() const { return m_sProperty5; }
  const ezUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetOffsetProperty() const { return m_sProperty4; }

  ezColor m_Color;
  ezVec3 m_vOffsetOrScale;
  ezEnum<ezBasisAxis> m_Axis;
};

//////////////////////////////////////////////////////////////////////////

class EZ_FOUNDATION_DLL ezDirectionVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDirectionVisualizerAttribute, ezVisualizerAttribute);

public:
  ezDirectionVisualizerAttribute();
  ezDirectionVisualizerAttribute(ezEnum<ezBasisAxis> axis, float fScale, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);
  ezDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);

  const ezUntrackedString& GetColorProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetLengthProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetAxisProperty() const { return m_sProperty3; }

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
  ezConeVisualizerAttribute(ezEnum<ezBasisAxis> axis, const char* szAngleProperty, float fScale, const char* szRadiusProperty, const ezColor& fixedColor = ezColorScheme::LightUI(ezColorScheme::Grape), const char* szColorProperty = nullptr);

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
  ezCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty, const char* szNearPlaneProperty, const char* szFarPlaneProperty);

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
  ezMaxArraySizeAttribute() = default;
  ezMaxArraySizeAttribute(ezUInt32 uiMaxSize) { m_uiMaxSize = uiMaxSize; }

  const ezUInt32& GetMaxSize() const { return m_uiMaxSize; }

private:
  ezUInt32 m_uiMaxSize = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief If this attribute is set, the UI is encouraged to prevent the user from creating duplicates of the same thing.
///
/// For arrays of objects this means that multiple objects of the same type are not allowed.
class EZ_FOUNDATION_DLL ezPreventDuplicatesAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPreventDuplicatesAttribute, ezPropertyAttribute);

public:
  ezPreventDuplicatesAttribute() = default;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Attribute for types that should not be exposed to the scripting framework
class EZ_FOUNDATION_DLL ezExcludeFromScript : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExcludeFromScript, ezPropertyAttribute);
};

/// \brief Attribute to mark a function up to be exposed to the scripting system. Arguments specify the names of the function parameters.
class EZ_FOUNDATION_DLL ezScriptableFunctionAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptableFunctionAttribute, ezPropertyAttribute);

  enum ArgType : ezUInt8
  {
    In,
    Out,
    Inout
  };

  ezScriptableFunctionAttribute(ArgType argType1 = In, const char* szArg1 = nullptr, ArgType argType2 = In, const char* szArg2 = nullptr,
    ArgType argType3 = In, const char* szArg3 = nullptr, ArgType argType4 = In, const char* szArg4 = nullptr, ArgType argType5 = In,
    const char* szArg5 = nullptr, ArgType argType6 = In, const char* szArg6 = nullptr, ArgType argType7 = In, const char* szArg7 = nullptr, ArgType argType8 = In, const char* szArg8 = nullptr, ArgType argType9 = In, const char* szArg9 = nullptr, ArgType argType10 = In, const char* szArg10 = nullptr, ArgType argType11 = In, const char* szArg11 = nullptr, ArgType argType12 = In, const char* szArg12 = nullptr);

  ezUInt32 GetArgumentCount() const { return m_ArgNames.GetCount(); }
  const char* GetArgumentName(ezUInt32 uiIndex) const { return m_ArgNames[uiIndex]; }

  ArgType GetArgumentType(ezUInt32 uiIndex) const { return static_cast<ArgType>(m_ArgTypes[uiIndex]); }

private:
  ezHybridArray<ezUntrackedString, 6> m_ArgNames;
  ezHybridArray<ezUInt8, 6> m_ArgTypes;
};

/// \brief Wrapper Attribute to add an attribute to a function argument
class EZ_FOUNDATION_DLL ezFunctionArgumentAttributes : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFunctionArgumentAttributes, ezPropertyAttribute);

  ezFunctionArgumentAttributes() = default;
  ezFunctionArgumentAttributes(ezUInt32 uiArgIndex, const ezPropertyAttribute* pAttribute1, const ezPropertyAttribute* pAttribute2 = nullptr, const ezPropertyAttribute* pAttribute3 = nullptr, const ezPropertyAttribute* pAttribute4 = nullptr);
  ~ezFunctionArgumentAttributes();

  ezUInt32 GetArgumentIndex() const { return m_uiArgIndex; }
  ezArrayPtr<const ezPropertyAttribute* const> GetArgumentAttributes() const { return m_ArgAttributes; }

private:
  ezUInt32 m_uiArgIndex = 0;
  // Not pretty, but the values in the array are either created using 'new' when using this class as a reflection decoration, or created using 'EZ_DEFAULT_NEW' when serialized and sent to the editor so in the dtor we need to know where these came from.
  bool m_bUsesGlobalNew = false;
  ezHybridArray<const ezPropertyAttribute*, 4> m_ArgAttributes;
};

/// \brief Used to mark an array or (unsigned)int property as source for dynamic pin generation on nodes
class EZ_FOUNDATION_DLL ezDynamicPinAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicPinAttribute, ezPropertyAttribute);

public:
  ezDynamicPinAttribute() = default;
  ezDynamicPinAttribute(const char* szProperty);

  const ezUntrackedString& GetProperty() const { return m_sProperty; }

private:
  ezUntrackedString m_sProperty;
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
  ezLongOpAttribute(const char* szOpTypeName)
    : m_sOpTypeName(szOpTypeName)
  {
  }

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

//////////////////////////////////////////////////////////////////////////

/// \brief Displays the value range as an image, allowing users to pick a value like on a slider.
///
/// This attribute always has to be combined with an ezClampValueAttribute to define the min and max value range.
/// The constructor takes the name of an image generator. The generator is used to build the QImage used for the slider background.
///
/// Image generators are registered through ezQtImageSliderWidget::s_ImageGenerators. Search the codebase for that variable
/// to determine which types of image generators are available.
/// You can register custom generators as well.
class EZ_FOUNDATION_DLL ezImageSliderUiAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImageSliderUiAttribute, ezTypeWidgetAttribute);

public:
  ezImageSliderUiAttribute() = default;
  ezImageSliderUiAttribute(const char* szImageGenerator)
  {
    m_sImageGenerator = szImageGenerator;
  }

  ezUntrackedString m_sImageGenerator;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Attribute that turns a string property into a selector for an RTTI type.
///
/// The base type defines what types to display.
/// For example if "ezComponent" is passed in, only types derived from ezComponent are listed.
class EZ_FOUNDATION_DLL ezRttiTypeStringAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRttiTypeStringAttribute, ezTypeWidgetAttribute);

public:
  ezRttiTypeStringAttribute() = default;
  ezRttiTypeStringAttribute(const char* szBaseType)
    : m_sBaseType(szBaseType)
  {
  }

  const char* GetBaseType() const { return m_sBaseType; }

private:
  ezUntrackedString m_sBaseType;
};
