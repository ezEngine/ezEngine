#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>

class PageDownloader : public QObject
{
  Q_OBJECT

public:
  explicit PageDownloader(QUrl url);

  const QByteArray& GetDownloadedData() const { return m_DownloadedData; }

signals:
  void FinishedDownload();

private slots:
  void DownloadDone(QNetworkReply* pReply);

private:
  QNetworkAccessManager m_WebCtrl;
  QByteArray m_DownloadedData;
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
  QPointer<PageDownloader> m_VersionPage;
};
