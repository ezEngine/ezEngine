#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Settings/Settings.h>
#include <QApplication>

class QColorDialog;

class EZ_EDITORFRAMEWORK_DLL ezEditorGUI : public QObject
{
  Q_OBJECT

public:
  static ezEditorGUI* GetInstance();

  void ShowColorDialog(const ezColor& color, bool bAlpha, QWidget* pParent, const char* slotCurColChanged, const char* slotAccept, const char* slotReject);

  void LoadState();
  void SaveState();

private slots:
  void SlotColorDialogClosed();

private:
  QColorDialog* m_pColorDlg;
  QPoint m_ColorDlgPos;

private:
  ezEditorGUI();
  EZ_DISALLOW_COPY_AND_ASSIGN(ezEditorGUI);
};

