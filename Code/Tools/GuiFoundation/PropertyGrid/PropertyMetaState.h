#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

/// \brief Describes the current meta state of a property for display purposes in the property grid
struct ezPropertyUiState
{
  enum Visibility
  {
    Default,    ///< Displayed normally, for editing (unless the property is read-only)
    Invisible,  ///< Hides the property entirely
    Disabled,   ///< The property is shown but disabled, when multiple objects are selected and in one the property is invisible, in the other it is disabled, the disabeled state takes precedence
  };

  ezPropertyUiState()
  {
    m_Visibility = Visibility::Default;
  }

  Visibility m_Visibility;
};

/// \brief Event that is broadcast whenever information about how to present properties is required
struct ezPropertyMetaStateEvent
{
  /// The object for which the information is queried
  const ezDocumentObject* m_pObject;

  /// The map into which event handlers should write their information about the state of each property.
  /// The string is the ezPropertyPath that identifies the property in m_pObject.
  ezMap<ezString, ezPropertyUiState>* m_pPropertyStates;
};

/// \brief This class allows to query additional information about how to present properties in the property grid
///
/// The property grid calls GetPropertyState() with the current selection of ezDocumentObject's.
/// This triggers the ezPropertyMetaStateEvent to be broadcast, which allows for other code to determine additional
/// information for the properties and write it into the event data.
class EZ_GUIFOUNDATION_DLL ezPropertyMetaState
{
  EZ_DECLARE_SINGLETON(ezPropertyMetaState);

public:

  ezPropertyMetaState();

  /// \brief Queries the property meta state for a single ezDocumentObject
  void GetPropertyState(const ezDocumentObject* pObject, ezMap<ezString, ezPropertyUiState>& out_PropertyStates);

  /// \brief Queries the property meta state for a multi selection of ezDocumentObject's
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetPropertyState(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items, ezMap<ezString, ezPropertyUiState>& out_PropertyStates);
  
  /// Attach to this event to get notified of property state queries.
  /// Add information to ezPropertyMetaStateEvent::m_pPropertyStates to return data.
  ezEvent<ezPropertyMetaStateEvent&> m_Events;

private:
  ezMap<ezString, ezPropertyUiState> m_Temp;
};

