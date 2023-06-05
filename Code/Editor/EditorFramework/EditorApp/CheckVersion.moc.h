#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Strings/String.h>

#include <Foundation/Types/UniquePtr.h>
#include <QObject>
#include <QProcess>

class PageDownloader : public QObject
{
  Q_OBJECT

public:
  explicit PageDownloader(const QString& sUrl);

  ezStringView GetDownloadedData() const { return m_sDownloadedPage; }

signals:
  void FinishedDownload();

private slots:
  void DownloadDone(int exitCode, QProcess::ExitStatus exitStatus);

private:
  ezUniquePtr<QProcess> m_pProcess;
  ezStringBuilder m_sDownloadedPage;
};

/// \brief Downloads a web page and checks whether the latest version online is newer than the current one
class ezQtVersionChecker : public QObject
{
  Q_OBJECT

public:
  ezQtVersionChecker();

  void Initialize();

  bool Check(bool bForce);

  const char* GetOwnVersion() const;
  const char* GetKnownLatestVersion() const;

  bool IsLatestNewer() const;

Q_SIGNALS:
  void VersionCheckCompleted(bool bNewRelease, bool bForced);

private slots:
  void PageDownloaded();

  ezResult StoreKnownVersion();

private:
  bool m_bRequireOnlineCheck = true;
  bool m_bForceCheck = false;
  bool m_bCheckInProgresss = false;
  ezString m_sConfigFile;
  ezString m_sKnownLatestVersion;
  ezUniquePtr<PageDownloader> m_pVersionPage;
};
