#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Time/Time.h>
#include <QString>
#include <QPixmap>
#include <QAbstractItemModel>

class EZ_GUIFOUNDATION_DLL QtImageCache : public QObject
{
  Q_OBJECT

public:
  static QtImageCache* GetInstance();

  static void SetFallbackImages(const char* szLoading, const char* szUnavailable);

  static const QPixmap* QueryPixmap(const char* szAbsolutePath, QModelIndex index = QModelIndex(), QVariant UserData1 = QVariant(), QVariant UserData2 = QVariant(), ezUInt32* out_pImageID = nullptr);
  static void InvalidateCache(const char* szAbsolutePath);

  static void SetMemoryUsageThreshold(ezUInt64 uiMemoryThreshold) { s_iMemoryUsageThreshold = (ezInt64) uiMemoryThreshold; }
  static void StopRequestProcessing(bool bPurgeExistingCache);
  static void EnableRequestProcessing();

signals:
  void ImageLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void ImageInvalidated(QString sPath, ezUInt32 uiImageID);

private:
  void EmitLoadedSignal(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);

  static void RunLoadingTask();
  static void LoadingTask(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  static void CleanupCache();

  struct Request
  {
    ezHashedString m_sPath;
    QModelIndex m_Index;
    QVariant m_UserData1;
    QVariant m_UserData2;

    bool operator< (const Request& rhs) const
    {
      if (m_sPath < rhs.m_sPath)
        return true;
      if (rhs.m_sPath < m_sPath)
        return false;
      if (m_Index < rhs.m_Index)
        return true;
      if (rhs.m_Index < m_Index)
        return false;
      if (m_UserData1 < rhs.m_UserData1)
        return true;
      if (rhs.m_UserData1 < m_UserData1)
        return false;
      if (m_UserData2 < rhs.m_UserData2)
        return true;
      if (rhs.m_UserData2 < m_UserData2)
        return false;

      return false;
    }
  };

  static ezMutex s_Mutex;
  static ezSet<Request> s_Requests;
  static bool s_bCacheEnabled;
  static bool s_bTaskRunning;
  static ezTime s_LastCleanupTime;
  static ezInt64 s_iMemoryUsageThreshold;
  static ezInt64 s_iCurrentMemoryUsage;
  static QPixmap* s_pImageLoading;
  static QPixmap* s_pImageUnavailable;
  static ezUInt32 s_uiCurImageID;

  struct CacheEntry
  {
    QPixmap m_Pixmap;
    ezTime m_LastAccess;
    ezUInt32 m_uiImageID;

    CacheEntry()
    {
      m_uiImageID = 0xFFFFFFFF;
    }
  };

  static ezMap<QString, CacheEntry> s_ImageCache;
};



