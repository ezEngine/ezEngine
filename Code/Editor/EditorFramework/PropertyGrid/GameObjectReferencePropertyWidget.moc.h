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
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void on_customContextMenuRequested(const QPoint& pt);
  void OnSelectReferencedObject();
  void OnCopyReference();
  void OnClearReference();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;
  void FillContextMenu(QMenu& menu);
  bool PickObjectOverride(const ezDocumentObject* pObject);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  QLineEdit* m_pWidget = nullptr;
  QToolButton* m_pButton = nullptr;
  ezHybridArray<ezSelectionContext*, 8> m_SelectionContextsToUnsubscribe;
};
