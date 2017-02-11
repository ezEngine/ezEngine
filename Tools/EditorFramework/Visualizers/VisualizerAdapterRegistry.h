#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct ezVisualizerManagerEvent;
class ezDocument;

class EZ_EDITORFRAMEWORK_DLL ezVisualizerAdapterRegistry
{
  EZ_DECLARE_SINGLETON(ezVisualizerAdapterRegistry);

public:
  ezVisualizerAdapterRegistry();
  ~ezVisualizerAdapterRegistry();

  ezRttiMappedObjectFactory<ezVisualizerAdapter> m_Factory;

private:
  void VisualizerManagerEventHandler(const ezVisualizerManagerEvent& e);
  void ClearAdapters(const ezDocument* pDocument);
  void CreateAdapters(const ezDocument* pDocument, const ezDocumentObject* pObject);

  struct Data
  {
    ezHybridArray<ezVisualizerAdapter*, 8> m_Adapters;

  };

  ezMap<const ezDocument*, Data> m_DocumentAdapters;

};


