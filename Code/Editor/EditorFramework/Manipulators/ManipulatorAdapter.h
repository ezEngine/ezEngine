#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class ezManipulatorAttribute;
class ezDocumentObject;
struct ezDocumentObjectPropertyEvent;
struct ezQtDocumentWindowEvent;
class ezObjectAccessorBase;
class ezGridSettingsMsgToEngine;

/// Abstract base class for in-viewport gizmo adapters driven by an ezManipulatorAttribute.
///
/// An adapter bridges one document object and one ezManipulatorAttribute to a set of gizmos
/// rendered in the viewport. It is instantiated by ezManipulatorAdapterRegistry and initialized
/// via SetManipulator(). Subclasses implement Finalize(), Update(), and UpdateGizmoTransform()
/// to set up and maintain their gizmos. UpdateGizmoTransform() is called every frame before
/// the viewport is redrawn, while Update() is called on property changes and after transactions.
///
/// Adapters are deleted synchronously by ezManipulatorAdapterRegistry::ClearAdapters when the
/// active manipulator changes (e.g. due to a selection change). Any code path inside an adapter
/// that changes the selection must therefore defer the change to avoid deleting itself while
/// still on the call stack.
class EZ_EDITORFRAMEWORK_DLL ezManipulatorAdapter
{
public:
  ezManipulatorAdapter();
  virtual ~ezManipulatorAdapter();

  /// Connects this adapter to the given attribute and document object, then calls Finalize() and Update().
  void SetManipulator(const ezManipulatorAttribute* pAttribute, const ezDocumentObject* pObject);

  virtual void QueryGridSettings(ezGridSettingsMsgToEngine& out_gridSettings) {}

private:
  void DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);

protected:
  virtual ezTransform GetOffsetTransform() const;
  virtual ezTransform GetObjectTransform() const;
  ezObjectAccessorBase* GetObjectAccessor() const;
  const ezAbstractProperty* GetProperty(const char* szProperty) const;

  /// Called once after SetManipulator(). Use this for one-time gizmo setup that depends on m_pObject being valid.
  virtual void Finalize() = 0;

  /// Called when a relevant property on m_pObject changes, after a transaction finishes, or on visibility changes.
  virtual void Update() = 0;

  /// Called every frame before the viewport redraws. Update gizmo world-space transforms here.
  virtual void UpdateGizmoTransform() = 0;

  /// Starts a temporary command sequence for live drag interaction. Must be paired with EndTemporaryInteraction() or CancelTemporayInteraction().
  void BeginTemporaryInteraction();
  void EndTemporaryInteraction();
  void CancelTemporayInteraction();

  /// Convenience wrapper that starts a transaction, sets up to six properties on m_pObject, and finishes the transaction.
  void ChangeProperties(const char* szProperty1, ezVariant value1, const char* szProperty2 = nullptr, ezVariant value2 = ezVariant(),
    const char* szProperty3 = nullptr, ezVariant value3 = ezVariant(), const char* szProperty4 = nullptr, ezVariant value4 = ezVariant(),
    const char* szProperty5 = nullptr, ezVariant value5 = ezVariant(), const char* szProperty6 = nullptr, ezVariant value6 = ezVariant());

  bool m_bManipulatorIsVisible = false;
  const ezManipulatorAttribute* m_pManipulatorAttr = nullptr;
  const ezDocumentObject* m_pObject = nullptr; ///< The document object this adapter operates on. Valid for the lifetime of the adapter.

  void ClampProperty(const char* szProperty, ezVariant& value) const;
};
