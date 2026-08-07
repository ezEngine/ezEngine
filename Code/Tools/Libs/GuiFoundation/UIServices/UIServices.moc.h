#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QApplication>
#include <QMessageBox>

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
      ShowDocumentTemporaryStatusBarText,
      ShowDocumentPermanentStatusBarText,
      ShowGlobalStatusBarText,
      ClickedDocumentPermanentStatusBarText,
      CheckForUpdates,
      GotoLinkTarget,
    };

    enum TextType
    {
      Info,
      Warning,
      Error
    };

    Type m_Type;
    ezString m_sText;
    ezTime m_Time;
    TextType m_TextType = TextType::Info;
  };

  static ezEvent<const ezQtUiServices::Event&, ezMutex> s_Events;

  struct TickEvent
  {
    enum class Type
    {
      BeforeFrame, ///< Check for any request for a frame.
      StartFrame,
      EndFrame,
    };

    Type m_Type;
    ezUInt32 m_uiFrame = 0;
    ezTime m_Time;
    double m_fRefreshRate = 60.0;
    mutable ezUInt32 m_uiFrameRequest = 0;     ///< Only valid for Type::BeforeFrame. Increased by an event handler to request a frame. If zero after the broadcast, no frame is started.
    mutable ezUInt32 m_uiForceCancelFrame = 0; ///< Only valid for Type::BeforeFrame. Increased by an event handler to cancel the frame. Regardless of m_uiFrameRequest, no frame is started if != zero.
  };

  static ezEvent<const ezQtUiServices::TickEvent&> s_TickEvent;

public:
  ezQtUiServices();

  static const char* GetOwnVersionString();

  /// True if the application doesn't show any window and only works in the background
  static bool IsHeadless();

  /// Set to true if the application doesn't show any window and only works in the background
  static void SetHeadless(bool bHeadless);

  /// True if no user is present to interact with the application. Always true in headless mode.
  static bool IsUnattended();

  /// Set if the application runs without a user present, e.g. driven by a script or an AI agent.
  ///
  /// The UI is still shown, but everything that would block waiting for input must not be displayed. The
  /// message box functions below therefore only log their message and return their unattended answer, and
  /// ezQtDialog::exec() returns without showing anything. Code that opens a modal window in some other way
  /// (QFileDialog, QInputDialog, ...) has to check this itself, otherwise it stalls the application
  /// indefinitely.
  ///
  /// This is one-way, for a process that is unattended for its entire runtime (the '-unattended' command
  /// line option). To make only part of a normal editor session unattended, use ezQtScopedUnattended.
  static void SetUnattended();

  /// Records that a dialog or message box was not shown, because IsUnattended() is set.
  ///
  /// Whoever triggered the operation would otherwise have no way to tell a suppressed dialog apart from
  /// the operation simply doing nothing. sDescription should identify what was suppressed well enough to
  /// act on it, i.e. the dialog's class name or the text of the message box.
  ///
  /// Called by ezQtDialog and the MessageBox* functions; call it manually when suppressing a modal window
  /// that goes through neither.
  static void ReportSuppressedDialog(ezStringView sDescription);

  /// Checks whether a modal window may be opened at all, and records its suppression when it may not.
  ///
  /// For modal windows that do not go through ezQtDialog and therefore have to check for themselves:
  /// QFileDialog, QInputDialog, QMessageBox used directly, and anything else that enters a nested event
  /// loop. Returns true when the window must NOT be shown - the caller then continues as if the user had
  /// cancelled it, which for a file picker means an empty selection.
  ///
  /// Only worth adding where an automated caller can actually reach the window, i.e. in code triggered by
  /// a global ezAction. A picker that only opens from a button inside another dialog is already covered,
  /// because that dialog never opens.
  static bool SuppressModalWindow(ezStringView sDescription);

  /// Records a failed assert instead of letting it open its dialog, and returns whether it was handled.
  ///
  /// Installed as the global assert handler by Init(). While a user is present nothing changes - the
  /// assert goes to the previous handler, which breaks into the debugger or shows the usual dialog. It
  /// is only unattended that an assert has nowhere to go: the dialog blocks the main thread, so the
  /// application stops answering entirely and whoever triggered the assert never learns that it fired.
  ///
  /// The failed condition is logged and collected like a suppressed dialog, and execution continues past
  /// an assert that was meant to stop it. That is a deliberate trade: continuing is not safe, but the
  /// caller does get told, whereas a hung process tells nobody anything. Anything reporting these should
  /// say that the application's state is now questionable and that it should be restarted.
  static void ReportFailedAssert(ezStringView sReport);

  static ezArrayPtr<const ezString> GetFailedAsserts();

  static void ClearFailedAsserts();

  /// The dialogs suppressed since the last ClearSuppressedDialogs(), oldest first.
  ///
  /// The list has an upper bound, so that a loop opening dialogs cannot grow it without limit. Once that
  /// is reached, further entries are dropped, not rotated.
  static ezArrayPtr<const ezString> GetSuppressedDialogs();

  static void ClearSuppressedDialogs();

  /// Shows a non-modal color dialog. The Qt slots are called when the selected color is changed or when the dialog is closed and the result
  /// accepted or rejected.
  void ShowColorDialog(const ezColor& color, bool bAlpha, bool bHDR, QWidget* pParent, const char* szSlotCurColChanged, const char* szSlotAccept, const char* szSlotReject);

  /// Might show a message box depending on the given status. If the status is 'failure' the szFailureMsg is shown, including the message in
  /// ezStatus. If the status is success a message box with text szSuccessMsg is shown, but only if the status message is not empty or if
  /// bOnlySuccessMsgIfDetails is false.
  static void MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg = "", bool bOnlySuccessMsgIfDetails = true);

  /// Shows an information message box
  static void MessageBoxInformation(const ezFormatString& msg, ezStringView sDontShowAgainID = {});

  /// Shows an warning message box
  static void MessageBoxWarning(const ezFormatString& msg);

  /// Shows a question message box and returns which button the user pressed.
  ///
  /// \param defaultButton The button that is preselected for the user. This is typically the option that prevents
  ///        accidents, ie. the one that does the least harm when someone confirms the dialog without reading it.
  /// \param unattendedButton The answer that is returned, without showing anything, when no user is present
  ///        (see IsUnattended()). This is usually *not* the same as defaultButton: an automated caller needs the
  ///        answer that lets the operation proceed, otherwise scripted work silently does nothing. Use the
  ///        accident-preventing answer here only when proceeding would destroy data that cannot be recovered.
  static QMessageBox::StandardButton MessageBoxQuestion(const ezFormatString& msg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, QMessageBox::StandardButton unattendedButton);

  /// Use this if you need to display a status bar message in any/all documents. Go directly through the document, if you only want to show a
  /// message in a single document window.
  static void ShowAllDocumentsTemporaryStatusBarMessage(const ezFormatString& msg, ezTime timeOut);

  static void ShowAllDocumentsPermanentStatusBarMessage(const ezFormatString& msg, Event::TextType type);

  /// Shows a 'critical' message in all container windows (in red), which does not disappear, until it is replaced with another (empty) string.
  static void ShowGlobalStatusBarMessage(const ezFormatString& msg);

  /// Opens the given file in the program that is registered in the OS to handle that file type.
  static ezResult OpenFileInDefaultProgram(ezStringView sPath);

  /// Open the given file in Visual Studio
  static ezResult OpenInVisualStudio(ezStringView sPath);

  /// Open the given file in Jetbrains Rider
  static ezResult OpenInRider(ezStringView sPath);

  /// Opens the given file or folder in the Explorer
  static void OpenInExplorer(ezStringView sPath, bool bIsFile);

  /// Shows the "Open With" dialog
  static void OpenWith(ezStringView sPath);

  /// Attempts to launch Visual Studio Code with the given command line
  static ezStatus OpenInVsCode(const QStringList& arguments);

  /// Loads some global state used by ezQtUiServices from the registry. E.g. the last position of the color dialog.
  void LoadState();

  /// Saves some global state used by ezQtUiServices to the registry.
  void SaveState();

  /// Returns a cached QIcon that was created from an internal Qt resource (e.g. 'QIcon(":QtNamespace/MyIcon.png")' ). Prevents creating the
  /// object over and over.
  ///
  /// If svgTintColor is a non-zero color, and sIdentifier points to an .SVG file, then the first time the icon is requested with that color,
  /// a copy is made, and the SVG content is modified such that white ("#FFFFFF") gets replaced by the requested color.
  /// Thus multiple tints of the same icon can be created for different use cases.
  /// Usually this is used to get different shades of the same icon, such that it looks good on the target background.
  static const QIcon& GetCachedIconResource(ezStringView sIdentifier, ezColor svgTintColor = ezColor::MakeZero());

  /// Returns a cached QImage that was created from an internal Qt resource (e.g. 'QImage(":QtNamespace/MyIcon.png")' ). Prevents creating the
  /// object over and over.
  static const QImage& GetCachedImageResource(const char* szIdentifier);

  /// Returns a cached QPixmap that was created from an internal Qt resource (e.g. 'QPixmap(":QtNamespace/MyIcon.png")' ). Prevents creating
  /// the object over and over.
  static const QPixmap& GetCachedPixmapResource(const char* szIdentifier);

  /// Adds the pattern to the gitignore file.
  ///
  /// If the gitignore file does not exist, it is created.
  /// If the pattern is already present in the file, it is not added again.
  static ezResult AddToGitIgnore(const char* szGitIgnoreFile, const char* szPattern);

  /// Raises the 'CheckForUpdates' event
  static void CheckForUpdates();

  /// For opening internal links to documents, object references, etc.
  ///
  /// Used for instance by log windows to support embedded links.
  /// Can be links to assets, document or even objects inside scenes.
  /// Could also be external URLs.
  /// What's supported is determined by whoever handles the request.
  /// The link target should look like this: "scheme:string".
  /// The "scheme:" allows handlers to distinguish the format of the rest of the string and whether it supports it.
  static void GotoLinkTarget(ezStringView sLinkTarget);

  void Init();

private Q_SLOTS:
  void TickEventHandler();

private:
  ezQtColorDialog* m_pColorDlg;
  QByteArray m_ColorDlgGeometry;

  static ezMap<ezString, QIcon> s_IconsCache;
  static ezMap<ezString, QImage> s_ImagesCache;
  static ezMap<ezString, QPixmap> s_PixmapsCache;
  static bool s_bHeadless;
  static bool s_bUnattended;
  static ezHybridArray<ezString, 4> s_SuppressedDialogs;
  static ezHybridArray<ezString, 4> s_FailedAsserts;
  static TickEvent s_LastTickEvent;
  bool m_bIsDrawingATM = false;

  friend class ezQtScopedUnattended;
};

/// Makes the enclosed code run as if no user were present, see ezQtUiServices::IsUnattended().
///
/// For automated calls into an editor that a user is otherwise sitting in front of - an MCP tool call,
/// for instance. Their dialogs must not block, while the same dialogs opened by the user's own menu
/// clicks still have to appear, so the state cannot be set globally.
///
/// Scopes nest and restore the previous state. Note that this does not undo ezQtUiServices::SetUnattended()
/// or headless mode: those stay in effect for the whole process.
class EZ_GUIFOUNDATION_DLL ezQtScopedUnattended
{
public:
  ezQtScopedUnattended();
  ~ezQtScopedUnattended();

private:
  bool m_bPrevUnattended;
};
