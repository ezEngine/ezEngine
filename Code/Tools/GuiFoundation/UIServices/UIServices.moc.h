#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Settings/Settings.h>
#include <ToolsFoundation/Basics/Status.h>
#include <Foundation/Communication/Event.h>
#include <QApplication>

class QColorDialog;

class EZ_GUIFOUNDATION_DLL ezUIServices : public QObject
{
  Q_OBJECT

public:
  struct Event
  {
    enum Type
    {
      ShowDocumentStatusBarText,
      ShowGlobalStatusBarText,
    };

    Type m_Type;
    ezString m_sText;
    ezTime m_Time;
  };

  static ezEvent<const ezUIServices::Event&> s_Events;

public:
  static ezUIServices* GetInstance();

  void ShowColorDialog(const ezColor& color, bool bAlpha, QWidget* pParent, const char* slotCurColChanged, const char* slotAccept, const char* slotReject);

  static void SetApplicationName(const char* szTitle) { s_sApplicationName = szTitle; }
  static const char* GetApplicationName() { return s_sApplicationName ; }

  static void MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg = "", bool bOnlySuccessMsgIfDetails = true);
  static void MessageBoxInformation(const char* szMsg);
  static void MessageBoxWarning(const char* szMsg);

  /// \brief Use this if you need to display a status bar message in any/all documents. Go directly through the document, if you only want to show a message in a single document window.
  static void ShowAllDocumentsStatusBarMessage(const char* szMsg, ezTime timeOut);

  /// \brief Shows a 'critical' message in all container windows (in red), which does not disappear, until it is replaced with another (empty) string.
  static void ShowGlobalStatusBarMessage(const char* szMsg);

  /// \brief Opens the given file or folder in the Explorer
  static void OpenInExplorer(const char* szPath);

  void LoadState();
  void SaveState();

private slots:
  void SlotColorDialogClosed();

private:
  static ezString s_sApplicationName;
  QColorDialog* m_pColorDlg;
  QPoint m_ColorDlgPos;

private:
  ezUIServices();
  EZ_DISALLOW_COPY_AND_ASSIGN(ezUIServices);
};

