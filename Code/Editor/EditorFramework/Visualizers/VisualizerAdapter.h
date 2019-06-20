#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Document/Document.h>

class ezVisualizerAttribute;
class ezDocumentObject;
struct ezDocumentObjectPropertyEvent;
struct ezQtDocumentWindowEvent;
class ezObjectAccessorBase;

/// \brief Base class for the editor side code that sets up a 'visualizer' for object properties.
///
/// Typically visualizers are configured with ezVisualizerAttribute's on component types.
/// The adapter reads the attribute values and sets up the necessary code to render them in the engine.
/// This is usually achieved by creating ezEngineGizmoHandle objects (which get automatically synchronized
/// with the engine process).
/// The adapter then reacts to editor side object changes and adjusts the engine side representation
/// as needed.
class EZ_EDITORFRAMEWORK_DLL ezVisualizerAdapter
{
public:
  ezVisualizerAdapter();
  virtual ~ezVisualizerAdapter();

  void SetVisualizer(const ezVisualizerAttribute* pAttribute, const ezDocumentObject* pObject);

  static ezQuat GetBasisRotation(ezBasisAxis::Enum identity, ezBasisAxis::Enum axis);

private:
  void DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);

protected:
  virtual ezTransform GetObjectTransform() const;
  ezObjectAccessorBase* GetObjectAccessor() const;
  const ezAbstractProperty* GetProperty(const char* szProperty) const;

  /// \brief Called to actually properly set up the adapter. All setup code is implemented here.
  virtual void Finalize() = 0;
  /// \brief Called when object properties have changed and the visualizer may need to react.
  virtual void Update() = 0;
  /// \brief Called when the object has been moved somehow. More light weight than a full update.
  virtual void UpdateGizmoTransform() = 0;

  bool m_bVisualizerIsVisible;
  const ezVisualizerAttribute* m_pVisualizerAttr;
  const ezDocumentObject* m_pObject;
};
