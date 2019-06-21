#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QModelIndex>
#include <QLineEdit>
//#include <EditorFramework/PropertyGrid/QtAssetLineEdit.moc.h>


/// *** Asset Browser ***

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectReferencePropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtGameObjectReferencePropertyWidget();

  //bool IsValidAssetType(const char* szAssetReference) const;

private Q_SLOTS:
  //void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void on_customContextMenuRequested(const QPoint& pt);
  //void OnOpenAssetDocument();
  //void OnSelectInAssetBrowser();
  //void OnCopyAssetGuid();
  void OnClearReference();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;
  void FillContextMenu(QMenu& menu);

protected:

  QHBoxLayout* m_pLayout = nullptr;
  QLineEdit* m_pWidget = nullptr;
  QToolButton* m_pButton = nullptr;
  //ezUuid m_AssetGuid;
};
