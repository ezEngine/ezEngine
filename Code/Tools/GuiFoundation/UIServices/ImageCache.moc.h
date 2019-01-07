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
#include <Foundation/Configuration/Singleton.h>

/// \brief A singleton class that caches Qt images that are typically used for thumbnails.
///
/// When an image is not available right away, a fallback is returned and the requested image goes into a loading queue.
/// When an image was finished loading, a signal is emitted to inform users to update their UI.
class EZ_GUIFOUNDATION_DLL ezQtImageCache : public QObject
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtImageCache);

public:
  ezQtImageCache();

  /// \brief Specifies which images to return when a requested image is currently not available (loading) or could not be found (unavailable).
  void SetFallbackImages(const char* szLoading, const char* szUnavailable);

  /// \brief Queries an image by an absolute path. If the image is cached, it is returned right away.
  ///
  /// If the image is not cached, a temporary image is returned and it is queued for loading.
  /// Once it is finished loading, the ImageLoaded() signal is emitted and \a index, \a UserData1 and \a UserData2 are passed through.
  /// Additionally an ImageID may be returned through \a out_pImageID. This can be used to identify an image when it is invalidated through the ImageInvalidated() signal.
  const QPixmap* QueryPixmap(const char* szAbsolutePath, QModelIndex index = QModelIndex(), QVariant UserData1 = QVariant(), QVariant UserData2 = QVariant(), ezUInt32* out_pImageID = nullptr);

  /// \brief Same as QueryPixmap(), but first \a szType is used to call QueryTypeImage() and check whether a type specific image was registerd. If yes, that is used instead of szAbsolutePath.
  const QPixmap* QueryPixmapForType(const char* szType, const char* szAbsolutePath, QModelIndex index = QModelIndex(), QVariant UserData1 = QVariant(), QVariant UserData2 = QVariant(), ezUInt32* out_pImageID = nullptr);

  /// \brief Invalidate the cached image with the given path. This is typically done when a thumbnail was just written to disk, to inform this system to reload the latest image from disk.
  void InvalidateCache(const char* szAbsolutePath);

  /// \brief When this threshold is reached, images that haven't been requested in a while are being evicted from the cache.
  void SetMemoryUsageThreshold(ezUInt64 uiMemoryThreshold) { m_iMemoryUsageThreshold = (ezInt64) uiMemoryThreshold; }

  /// \brief Called whenever the application should stop or pause further image loading, e.g. before shutdown or during project loading.
  void StopRequestProcessing(bool bPurgeExistingCache);

  /// \brief Re-enables image loading if it was previously stopped.
  void EnableRequestProcessing();

  /// \brief Registers a pixmap to be used when an image for a certain type is requested. See QueryPixmapForType.
  void RegisterTypeImage(const char* szType, QPixmap pixmap);

  /// \brief Returns a pixmap or nullptr that was registered with RegisterTypeImage()
  const QPixmap* QueryTypeImage(const char* szType) const;

Q_SIGNALS:
  void ImageLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void ImageInvalidated(QString sPath, unsigned int uiImageID);

private:
  void EmitLoadedSignal(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);

  void RunLoadingTask();
  static void LoadingTask(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void CleanupCache();

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

  mutable ezMutex m_Mutex;
  ezSet<Request> m_Requests;
  bool m_bCacheEnabled;
  bool m_bTaskRunning;
  ezTime m_LastCleanupTime;
  ezInt64 m_iMemoryUsageThreshold;
  ezInt64 m_iCurrentMemoryUsage;
  QPixmap* m_pImageLoading;
  QPixmap* m_pImageUnavailable;
  ezUInt32 m_uiCurImageID;

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

  ezMap<QString, CacheEntry> m_ImageCache;
  ezMap<QString, QPixmap> m_TypeIamges;
};



