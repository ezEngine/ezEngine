#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Communication/Event.h>
#include <QApplication>
#include <QMessageBox>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Strings/FormatString.h>

class QColorDialog;
class ezQtColorDialog;

class EZ_GUIFOUNDATION_DLL ezQtUiServices : public QObject
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtUiServices);

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

  static ezEvent<const ezQtUiServices::Event&> s_Events;

public:
  ezQtUiServices();

  /// \brief True if the application doesn't show any window and only works in the background
  static bool IsHeadless();

  /// \brief Set to true if the application doesn't show any window and only works in the background
  static void SetHeadless(bool bHeadless);

  /// \brief Shows a non-modal color dialog. The Qt slots are called when the selected color is changed or when the dialog is closed and the result accepted or rejected.
  void ShowColorDialog(const ezColor& color, bool bAlpha, bool bHDR, QWidget* pParent, const char* slotCurColChanged, const char* slotAccept, const char* slotReject);

  /// \brief Might show a message box depending on the given status. If the status is 'failure' the szFailureMsg is shown, including the message in ezStatus.
  /// If the status is success a message box with text szSuccessMsg is shown, but only if the status message is not empty or if bOnlySuccessMsgIfDetails is false.
  static void MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg = "", bool bOnlySuccessMsgIfDetails = true);

  /// \brief Shows an information message box
  static void MessageBoxInformation(const ezFormatString& msg);

  /// \brief Shows an warning message box
  static void MessageBoxWarning(const ezFormatString& msg);

  /// \brief Shows a question message box and returns which button the user pressed
  static QMessageBox::StandardButton MessageBoxQuestion(const ezFormatString& msg, QMessageBox::StandardButtons buttons,
                                                        QMessageBox::StandardButton defaultButton);

  /// \brief Use this if you need to display a status bar message in any/all documents. Go directly through the document, if you only want to show a message in a single document window.
  static void ShowAllDocumentsStatusBarMessage(const ezFormatString& msg, ezTime timeOut);

  /// \brief Shows a 'critical' message in all container windows (in red), which does not disappear, until it is replaced with another (empty) string.
  static void ShowGlobalStatusBarMessage(const ezFormatString& msg);

  /// \brief Opens the given file in the program that is registered in the OS to handle that file type.
  static bool OpenFileInDefaultProgram(const char* szPath);

  /// \brief Opens the given file or folder in the Explorer
  static void OpenInExplorer(const char* szPath, bool bIsFile);

  /// \brief Attempts to launch Visual Studio Code with the given command line
  static ezStatus OpenInVsCode(const QStringList& arguments);

  /// \brief Loads some global state used by ezQtUiServices from the registry. E.g. the last position of the color dialog.
  void LoadState();

  /// \brief Saves some global state used by ezQtUiServices to the registry.
  void SaveState();

  /// \brief Returns a cached QIcon that was created from an internal Qt resource (e.g. 'QIcon(":QtNamespace/MyIcon.png")' ). Prevents creating the object over and over.
  static const QIcon& GetCachedIconResource(const char* szIdentifier);

  /// \brief Returns a cached QImage that was created from an internal Qt resource (e.g. 'QImage(":QtNamespace/MyIcon.png")' ). Prevents creating the object over and over.
  static const QImage& GetCachedImageResource(const char* szIdentifier);

  /// \brief Returns a cached QPixmap that was created from an internal Qt resource (e.g. 'QPixmap(":QtNamespace/MyIcon.png")' ). Prevents creating the object over and over.
  static const QPixmap& GetCachedPixmapResource(const char* szIdentifier);

  /// \brief Adds the pattern to the gitignore file.
  ///
  /// If the gitignore file does not exist, it is created.
  /// If the pattern is already present in the file, it is not added again.
  static ezResult AddToGitIgnore(const char* szGitIgnoreFile, const char* szPattern);

private:
  ezQtColorDialog* m_pColorDlg;
  QByteArray m_ColorDlgGeometry;

  static ezMap<ezString, QIcon> s_IconsCache;
  static ezMap<ezString, QImage> s_ImagesCache;
  static ezMap<ezString, QPixmap> s_PixmapsCache;
  static bool s_bHeadless;

};

