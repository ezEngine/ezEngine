#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Settings/Settings.h>
#include <ToolsFoundation/Basics/Status.h>
#include <QApplication>

class QColorDialog;

class EZ_GUIFOUNDATION_DLL ezUIServices : public QObject
{
  Q_OBJECT

public:
  static ezUIServices* GetInstance();

  void ShowColorDialog(const ezColor& color, bool bAlpha, QWidget* pParent, const char* slotCurColChanged, const char* slotAccept, const char* slotReject);

  static void SetMessageBoxTitle(const char* szTitle) { s_sMessageBoxTitle = szTitle; }
  static void MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg = "", bool bOnlySuccessMsgIfDetails = true);
  static void MessageBoxInformation(const char* szMsg);
  static void MessageBoxWarning(const char* szMsg);

  void LoadState();
  void SaveState();

private slots:
  void SlotColorDialogClosed();

private:
  static ezString s_sMessageBoxTitle;
  QColorDialog* m_pColorDlg;
  QPoint m_ColorDlgPos;

private:
  ezUIServices();
  EZ_DISALLOW_COPY_AND_ASSIGN(ezUIServices);
};

