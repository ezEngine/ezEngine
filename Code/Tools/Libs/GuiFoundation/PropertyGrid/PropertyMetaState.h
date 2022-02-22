#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/RefCounted.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

/// \brief Describes the current meta state of a property for display purposes in the property grid
struct ezPropertyUiState
{
  enum Visibility
  {
    Default,   ///< Displayed normally, for editing (unless the property is read-only)
    Invisible, ///< Hides the property entirely
    Disabled,  ///< The property is shown but disabled, when multiple objects are selected and in one the property is invisible, in the other it is
               ///< disabled, the disabled state takes precedence
  };

  ezPropertyUiState()
  {
    m_Visibility = Visibility::Default;
  }

  Visibility m_Visibility;
  ezString m_sNewLabelText;
};

/// \brief Event that is broadcast whenever information about how to present properties is required
struct ezPropertyMetaStateEvent
{
  /// The object for which the information is queried
  const ezDocumentObject* m_pObject = nullptr;

  /// The map into which event handlers should write their information about the state of each property.
  /// The string is the property name that identifies the property in m_pObject.
  ezMap<ezString, ezPropertyUiState>* m_pPropertyStates = nullptr;
};

/// \brief Event that is broadcast whenever information about how to present elements in a container is required
struct ezContainerElementMetaStateEvent
{
  /// The object for which the information is queried
  const ezDocumentObject* m_pObject = nullptr;
  /// The Container property
  const char* m_szProperty = nullptr;
  /// The map into which event handlers should write their information about the state of each container element.
  /// The ezVariant should be the key of the container element, either ezUInt32 for arrays and sets or ezString for maps.
  ezHashTable<ezVariant, ezPropertyUiState>* m_pContainerElementStates = nullptr;
};

/// \brief This class allows to query additional information about how to present properties in the property grid
///
/// The property grid calls GetTypePropertiesState() and GetContainerElementsState() with the current selection of ezDocumentObject's.
/// This triggers the ezPropertyMetaStateEvent to be broadcast, which allows for other code to determine additional
/// information for the properties and write it into the event data.
class EZ_GUIFOUNDATION_DLL ezPropertyMetaState
{
  EZ_DECLARE_SINGLETON(ezPropertyMetaState);

public:
  ezPropertyMetaState();

  /// \brief Queries the property meta state for a single ezDocumentObject
  void GetTypePropertiesState(const ezDocumentObject* pObject, ezMap<ezString, ezPropertyUiState>& out_PropertyStates);

  /// \brief Queries the property meta state for a multi selection of ezDocumentObject's
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetTypePropertiesState(const ezHybridArray<ezPropertySelection, 8>& items, ezMap<ezString, ezPropertyUiState>& out_PropertyStates);

  /// \brief Queries the meta state for the elements of a single container property on one ezDocumentObject.
  void GetContainerElementsState(const ezDocumentObject* pObject, const char* szProperty, ezHashTable<ezVariant, ezPropertyUiState>& out_PropertyStates);

  /// \brief Queries the meta state for the elements of a single container property on a multi selection of ezDocumentObjects.
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetContainerElementsState(const ezHybridArray<ezPropertySelection, 8>& items, const char* szProperty, ezHashTable<ezVariant, ezPropertyUiState>& out_PropertyStates);

  /// Attach to this event to get notified of property state queries.
  /// Add information to ezPropertyMetaStateEvent::m_pPropertyStates to return data.
  ezEvent<ezPropertyMetaStateEvent&> m_Events;
  /// Attach to this event to get notified of container element state queries.
  /// Add information to ezContainerElementMetaStateEvent::m_pContainerElementStates to return data.
  ezEvent<ezContainerElementMetaStateEvent&> m_ContainerEvents;

private:
  ezMap<ezString, ezPropertyUiState> m_Temp;
  ezHashTable<ezVariant, ezPropertyUiState> m_Temp2;
};
