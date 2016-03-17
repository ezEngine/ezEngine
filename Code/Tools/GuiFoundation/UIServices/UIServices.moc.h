#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Settings/Settings.h>
#include <ToolsFoundation/Basics/Status.h>
#include <Foundation/Communication/Event.h>
#include <QApplication>
#include <QMessageBox>
#include <Foundation/Configuration/Singleton.h>

class QColorDialog;

class EZ_GUIFOUNDATION_DLL ezUIServices : public QObject
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezUIServices);

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
  ezUIServices();

  void ShowColorDialog(const ezColor& color, bool bAlpha, QWidget* pParent, const char* slotCurColChanged, const char* slotAccept, const char* slotReject);

  static void SetApplicationName(const char* szTitle) { s_sApplicationName = szTitle; }
  static const char* GetApplicationName() { return s_sApplicationName ; }

  static void MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg = "", bool bOnlySuccessMsgIfDetails = true);
  static void MessageBoxInformation(const char* szMsg);
  static void MessageBoxWarning(const char* szMsg);
  static QMessageBox::StandardButton MessageBoxQuestion(const char* szMsg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);

  /// \brief Use this if you need to display a status bar message in any/all documents. Go directly through the document, if you only want to show a message in a single document window.
  static void ShowAllDocumentsStatusBarMessage(const char* szMsg, ezTime timeOut);

  /// \brief Shows a 'critical' message in all container windows (in red), which does not disappear, until it is replaced with another (empty) string.
  static void ShowGlobalStatusBarMessage(const char* szMsg);

  /// \brief Opens the given file or folder in the Explorer
  static void OpenInExplorer(const char* szPath);

  void LoadState();
  void SaveState();

  /// \brief Returns a cached QIcon that was created from an internal Qt resource (e.g. 'QIcon(":QtNamespace/MyIcon.png")' ). Prevents creating the object over and over.
  static const QIcon& GetCachedIconResource(const char* szIdentifier);

  /// \brief Returns a cached QImage that was created from an internal Qt resource (e.g. 'QImage(":QtNamespace/MyIcon.png")' ). Prevents creating the object over and over.
  static const QImage& GetCachedImageResource(const char* szIdentifier);

  /// \brief Returns a cached QPixmap that was created from an internal Qt resource (e.g. 'QPixmap(":QtNamespace/MyIcon.png")' ). Prevents creating the object over and over.
  static const QPixmap& GetCachedPixmapResource(const char* szIdentifier);

private slots:
  void SlotColorDialogClosed();

private:
  static ezString s_sApplicationName;
  QColorDialog* m_pColorDlg;
  QPoint m_ColorDlgPos;

  static ezMap<ezString, QIcon> s_IconsCache;
  static ezMap<ezString, QImage> s_ImagesCache;
  static ezMap<ezString, QPixmap> s_PixmapsCache;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezUIServices);
};

