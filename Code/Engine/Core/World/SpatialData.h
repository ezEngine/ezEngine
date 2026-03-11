#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Strings/HashedString.h>

/// \brief Defines categories and metadata for spatial data used by spatial systems.
///
/// Provides a category system for organizing spatial objects (like render objects, collision objects)
/// that can be used by spatial systems for efficient queries and updates. Categories are registered
/// globally and can have flags to indicate update frequency hints.
struct ezSpatialData
{
  struct Flags
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      None = 0,
      FrequentChanges = EZ_BIT(0), ///< Indicates that objects in this category change their bounds frequently. Spatial System implementations can use that as hint for internal optimizations.

      Default = None
    };

    struct Bits
    {
      StorageType FrequentUpdates : 1;
    };
  };

  /// \brief Represents a spatial data category for organizing objects in spatial systems.
  struct Category
  {
    EZ_ALWAYS_INLINE Category()
      : m_uiValue(ezSmallInvalidIndex)
    {
    }

    EZ_ALWAYS_INLINE explicit Category(ezUInt16 uiValue)
      : m_uiValue(uiValue)
    {
    }

    EZ_ALWAYS_INLINE bool operator==(const Category& other) const { return m_uiValue == other.m_uiValue; }
    EZ_ALWAYS_INLINE bool operator!=(const Category& other) const { return m_uiValue != other.m_uiValue; }

    ezUInt16 m_uiValue;

    /// \brief Returns the bitmask representation of this category for use in queries.
    EZ_ALWAYS_INLINE ezUInt32 GetBitmask() const { return m_uiValue != ezSmallInvalidIndex ? static_cast<ezUInt32>(EZ_BIT(m_uiValue)) : 0; }
  };

  /// \brief Registers a spatial data category under the given name.
  ///
  /// If the same category was already registered before, it returns that instead.
  /// Asserts that there are no more than 32 unique categories.
  EZ_CORE_DLL static Category RegisterCategory(ezStringView sCategoryName, const ezBitflags<Flags>& flags);

  /// \brief Returns either an existing category with the given name or ezInvalidSpatialDataCategory.
  EZ_CORE_DLL static Category FindCategory(ezStringView sCategoryName);

  /// \brief Returns the name of the given category.
  EZ_CORE_DLL static const ezHashedString& GetCategoryName(Category category);

  /// \brief Returns the flags for the given category.
  EZ_CORE_DLL static const ezBitflags<Flags>& GetCategoryFlags(Category category);

private:
  struct CategoryData
  {
    ezHashedString m_sName;
    ezBitflags<Flags> m_Flags;
  };

  static ezHybridArray<ezSpatialData::CategoryData, 32>& GetCategoryData();
};

/// \brief Predefined spatial data categories commonly used throughout the engine.
struct EZ_CORE_DLL ezDefaultSpatialDataCategories
{
  static ezSpatialData::Category RenderStatic;     ///< Static render objects that don't change position frequently
  static ezSpatialData::Category RenderDynamic;    ///< Dynamic render objects that may change position frequently
  static ezSpatialData::Category OcclusionStatic;  ///< Static objects used for occlusion culling
  static ezSpatialData::Category OcclusionDynamic; ///< Dynamic objects used for occlusion culling
};

/// \brief When an object is 'seen' by a view and thus tagged as 'visible', this enum describes what kind of observer triggered this.
///
/// This is used to determine how important certain updates, such as animations, are to execute.
/// E.g. when a 'shadow view' or 'reflection view' is the only thing that observes an object, animations / particle effects and so on,
/// can be updated less frequently.
struct ezVisibilityState
{
  using StorageType = ezUInt8;

  enum Enum : StorageType
  {
    Invisible = 0, ///< The object isn't visible to any view.
    Indirect = 1,  ///< The object is seen by a view that only indirectly makes the object visible (shadow / reflection / render target).
    Direct = 2,    ///< The object is seen directly by a main view and therefore it needs to be updated at maximum frequency.

    Default = Invisible
  };
};

#define ezInvalidSpatialDataCategory ezSpatialData::Category()
