#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>
#include <QModelIndex>

class ezSelectionContext;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectReferencePropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtGameObjectReferencePropertyWidget();

private Q_SLOTS:
  void on_PickObject_clicked();

protected slots:
  void on_customContextMenuRequested(const QPoint& pt);
  void OnSelectReferencedObject();
  void OnCopyReference();
  void OnClearReference();
  void OnPasteReference();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;
  void FillContextMenu(QMenu& menu);
  void PickObjectOverride(const ezDocumentObject* pObject);
  void SetValue(const QString& sText);
  void ClearPicking();
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  QLabel* m_pWidget = nullptr;
  QString m_sInternalValue;
  QToolButton* m_pButton = nullptr;
  ezHybridArray<ezSelectionContext*, 8> m_SelectionContextsToUnsubscribe;
};
