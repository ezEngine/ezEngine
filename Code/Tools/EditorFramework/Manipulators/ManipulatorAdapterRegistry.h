#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct ezManipulatorManagerEvent;
class ezDocument;

class EZ_EDITORFRAMEWORK_DLL ezManipulatorAdapterRegistry
{
  EZ_DECLARE_SINGLETON(ezManipulatorAdapterRegistry);

public:
  ezManipulatorAdapterRegistry();
  ~ezManipulatorAdapterRegistry();

  ezRttiMappedObjectFactory<ezManipulatorAdapter> m_Factory;

private:
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);
  void ClearAdapters(const ezDocument* pDocument);

  struct Data
  {
    ezHybridArray<ezManipulatorAdapter*, 8> m_Adapters;

  };

  ezMap<const ezDocument*, Data> m_DocumentAdapters;

};
