#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Types/Variant.h>
#include <CoreUtils/DataStructures/ObjectMetaData.h>
#include <ToolsFoundation/Document/Document.h>

class ezVisualizerAttribute;
class ezDocumentObject;
struct ezDocumentObjectPropertyEvent;
struct ezQtDocumentWindowEvent;
class ezObjectAccessorBase;

class EZ_EDITORFRAMEWORK_DLL ezVisualizerAdapter
{
public:
  ezVisualizerAdapter();
  virtual ~ezVisualizerAdapter();

  void SetVisualizer(const ezVisualizerAttribute* pAttribute, const ezDocumentObject* pObject);

private:
  void DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);

protected:
  virtual ezTransform GetObjectTransform() const;
  ezObjectAccessorBase* GetObjectAccessor() const;
  const ezAbstractProperty* GetProperty(const char* szProperty) const;

  virtual void Finalize() = 0;
  virtual void Update() = 0;
  virtual void UpdateGizmoTransform() = 0;

  bool m_bVisualizerIsVisible;
  const ezVisualizerAttribute* m_pVisualizerAttr;
  const ezDocumentObject* m_pObject;
};