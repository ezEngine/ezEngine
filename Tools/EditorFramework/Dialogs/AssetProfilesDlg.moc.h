#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Tools/EditorFramework/ui_AssetProfilesDlg.h>

#include <QDialog>

class ezAssetProfilesDocument;
class ezAssetProfile;
class ezQtDocumentTreeView;
class ezDocument;
struct ezDocumentObjectPropertyEvent;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetProfilesDlg : public QDialog, public Ui_ezQtAssetProfilesDlg
{
public:
  Q_OBJECT

public:
  ezQtAssetProfilesDlg(QWidget* parent);
  ~ezQtAssetProfilesDlg();

  ezUInt32 m_uiActiveConfig = 0;

private slots:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void OnItemDoubleClicked(QModelIndex idx);
  void on_AddButton_clicked();
  void on_DeleteButton_clicked();
  void on_RenameButton_clicked();
  void on_SwitchToButton_clicked();

private:
  struct Binding
  {
    enum class State
    {
      None,
      Added,
      Deleted
    };

    State m_State = State::None;
    ezAssetProfile* m_pProfile = nullptr;
  };

  void AllAssetProfilesToObject();
  void PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();
  ezUuid NativeToObject(ezAssetProfile* pProfile);
  void ObjectToNative(ezUuid objectGuid, ezAssetProfile* pProfile);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  ezAssetProfilesDocument* m_pDocument;
  ezMap<ezUuid, Binding> m_ProfileBindings;
};
