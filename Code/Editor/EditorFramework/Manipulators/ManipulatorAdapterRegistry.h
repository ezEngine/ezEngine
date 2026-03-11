#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

struct ezManipulatorManagerEvent;
class ezDocument;

/// Singleton that owns and manages ezManipulatorAdapter instances for all open documents.
///
/// Listens to ezManipulatorManager::m_Events. When the active manipulator changes, it tears
/// down all existing adapters for that document and creates new ones via m_Factory, one per
/// object in the selection. m_Factory maps the RTTI of an ezManipulatorAttribute subclass to
/// the corresponding ezManipulatorAdapter subclass. Adapters must be registered there by the
/// code that introduces them.
///
/// Adapters are heap-allocated and owned by this registry. They are deleted synchronously
/// in ClearAdapters, which means an adapter must not call into code that reaches ClearAdapters
/// on the same call stack (e.g. changing the document selection from within a gizmo event).
class EZ_EDITORFRAMEWORK_DLL ezManipulatorAdapterRegistry
{
  EZ_DECLARE_SINGLETON(ezManipulatorAdapterRegistry);

public:
  ezManipulatorAdapterRegistry();
  ~ezManipulatorAdapterRegistry();

  /// Maps ezManipulatorAttribute RTTI types to their corresponding ezManipulatorAdapter factory functions.
  /// Register new adapter types here to make them available to the system.
  ezRttiMappedObjectFactory<ezManipulatorAdapter> m_Factory;

  /// Collects grid settings from all active adapters for the given document.
  void QueryGridSettings(const ezDocument* pDocument, ezGridSettingsMsgToEngine& out_gridSettings);

private:
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);
  void ClearAdapters(const ezDocument* pDocument);

  struct Data
  {
    ezHybridArray<ezManipulatorAdapter*, 8> m_Adapters;
  };

  ezMap<const ezDocument*, Data> m_DocumentAdapters;
};
