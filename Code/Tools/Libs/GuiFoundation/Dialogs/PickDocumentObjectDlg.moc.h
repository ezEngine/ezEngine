#pragma once

#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_PickDocumentObjectDlg.h>
#include <QDialog>

class ezDocumentObject;

class EZ_GUIFOUNDATION_DLL ezQtPickDocumentObjectDlg : public QDialog, public Ui_PickDocumentObjectDlg
{
  Q_OBJECT

public:
  struct Element
  {
    const ezDocumentObject* m_pObject;
    ezString m_sDisplayName;
  };

  ezQtPickDocumentObjectDlg(QWidget* pParent, const ezArrayPtr<Element>& objects, const ezUuid& currentObject);

  /// Stores the result that the user picked
  const ezDocumentObject* m_pPickedObject = nullptr;

private Q_SLOTS:
  void on_ObjectTree_itemDoubleClicked(QTreeWidgetItem* pItem, int column);

private:
  void UpdateTable();

  ezArrayPtr<Element> m_Objects;
  ezUuid m_CurrentObject;
};
