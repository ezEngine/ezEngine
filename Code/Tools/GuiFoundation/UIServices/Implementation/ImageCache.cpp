#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Threading/Lock.h>
#include <QPixmap>
#include <QtConcurrent/qtconcurrentrun.h>

ezMutex QtImageCache::s_Mutex;
ezSet<QtImageCache::Request> QtImageCache::s_Requests;
bool QtImageCache::s_bCacheEnabled = true;
bool QtImageCache::s_bTaskRunning = false;
ezMap<QString, QtImageCache::CacheEntry> QtImageCache::s_ImageCache;
ezTime QtImageCache::s_LastCleanupTime;
ezInt64 QtImageCache::s_iMemoryUsageThreshold = 30 * 1024 * 1024; // 30 MB
ezInt64 QtImageCache::s_iCurrentMemoryUsage = 0;
QPixmap* QtImageCache::s_pImageLoading = NULL;
QPixmap* QtImageCache::s_pImageUnavailable = NULL;
ezUInt32 QtImageCache::s_uiCurImageID = 1;

static QtImageCache g_ImageCacheSingleton;

QtImageCache* QtImageCache::GetInstance()
{
  return &g_ImageCacheSingleton;
}

void QtImageCache::SetFallbackImages(const char* szLoading, const char* szUnavailable)
{
  delete s_pImageLoading;
  s_pImageLoading = new QPixmap(szLoading);

  delete s_pImageUnavailable;
  s_pImageUnavailable = new QPixmap(szUnavailable);
}

void QtImageCache::InvalidateCache(const char* szAbsolutePath)
{
  ezStringBuilder sCleanPath = szAbsolutePath;
  sCleanPath.MakeCleanPath();

  const QString sPath = QString::fromUtf8(sCleanPath.GetData());

  EZ_LOCK(s_Mutex);

  auto e = s_ImageCache.Find(sPath);

  if (!e.IsValid())
    return;

  ezUInt32 id = e.Value().m_uiImageID;
  s_ImageCache.Remove(e);

  emit g_ImageCacheSingleton.ImageInvalidated(sPath, id);
}

const QPixmap* QtImageCache::QueryPixmap(const char* szAbsolutePath, QModelIndex index, QVariant UserData1, QVariant UserData2, ezUInt32* out_pImageID)
{
  if (out_pImageID)
    *out_pImageID = 0;

  if (s_pImageLoading == NULL)
    SetFallbackImages(":/GuiFoundation/ThumbnailLoading.png", ":/GuiFoundation/ThumbnailUnavailable.png");

  ezStringBuilder sCleanPath = szAbsolutePath;
  sCleanPath.MakeCleanPath();

  const QString sPath = QString::fromUtf8(sCleanPath.GetData());

  EZ_LOCK(s_Mutex);

  CleanupCache();

  auto itEntry = s_ImageCache.Find(sPath);

  if (itEntry.IsValid())
  {
    if (out_pImageID)
      *out_pImageID = itEntry.Value().m_uiImageID;

    itEntry.Value().m_LastAccess = ezTime::Now();
    return &itEntry.Value().m_Pixmap;
  }

  // do not queue any further requests, when the cache is disabled
  if (!s_bCacheEnabled)
    return s_pImageLoading;

  ezHashedString sHashed;
  sHashed.Assign(sCleanPath.GetData());

  Request r;
  r.m_sPath = sHashed;
  r.m_Index = index;
  r.m_UserData1 = UserData1;
  r.m_UserData2 = UserData2;

  // we could / should implement prioritization here
  s_Requests.Insert(r);

  RunLoadingTask();

  return s_pImageLoading;
}

void QtImageCache::RunLoadingTask()
{
  EZ_LOCK(s_Mutex);

  // if someone is already working
  if (s_bTaskRunning)
    return;

  // do not start another run, if the cache has been deactivated
  if (!s_bCacheEnabled)
    return;

  // if nothing is to do
  while (!s_Requests.IsEmpty())
  {
    auto it = s_Requests.GetIterator();
    Request req = it.Key();

    const QString sQtPath = QString::fromUtf8(req.m_sPath.GetData());

    // do not try to load something that has already been loaded in the mean time
    if (!s_ImageCache.Find(sQtPath).IsValid())
    {
      s_bTaskRunning = true;
      QtConcurrent::run(LoadingTask, sQtPath, req.m_Index, req.m_UserData1, req.m_UserData2);
      return;
    }
    else
    {
      s_Requests.Remove(it);

      // inform the requester that his request has been fulfilled
      g_ImageCacheSingleton.EmitLoadedSignal(sQtPath, req.m_Index, req.m_UserData1, req.m_UserData2);
    }
  }

  // if we fall through, the queue is now empty
}

void QtImageCache::StopRequestProcessing(bool bPurgeExistingCache)
{
  bool bTaskRunning = false;

  {
    EZ_LOCK(s_Mutex);

    bTaskRunning = s_bTaskRunning;

    s_bCacheEnabled = false;
    s_Requests.Clear();

    if (bPurgeExistingCache)
      s_ImageCache.Clear();
  }

  // make sure to wait till the loading task has stopped
  while (bTaskRunning)
  {
    {
      EZ_LOCK(s_Mutex);
      bTaskRunning = s_bTaskRunning;
    }

    ezThreadUtils::Sleep(100);
  }
}

void QtImageCache::EnableRequestProcessing()
{
  EZ_LOCK(s_Mutex);

  s_bCacheEnabled = true;
  RunLoadingTask();
}

void QtImageCache::EmitLoadedSignal(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  emit ImageLoaded(sPath, index, UserData1, UserData2);
}

void QtImageCache::LoadingTask(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  QImage Image;
  const bool bImageAvailable = Image.load(sPath);

  /// \todo Remove this Sleep (needed for testing)
  ezThreadUtils::Sleep(100);

  EZ_LOCK(s_Mutex);

  // remove the task from the queue
  {
    Request req;
    req.m_sPath.Assign(sPath.toUtf8().data());
    req.m_Index = index;
    req.m_UserData1 = UserData1;
    req.m_UserData2 = UserData2;

    s_Requests.Remove(req);
  }

  s_bTaskRunning = false;

  if (!s_bCacheEnabled)
    return;

  auto& entry = s_ImageCache[sPath];
  entry.m_uiImageID = ++s_uiCurImageID;

  s_iCurrentMemoryUsage -= entry.m_Pixmap.width() * entry.m_Pixmap.height() * 4;

  if (bImageAvailable)
    entry.m_Pixmap = QPixmap::fromImage(Image);
  else
  if (s_pImageUnavailable)
    entry.m_Pixmap = *s_pImageUnavailable;

  entry.m_LastAccess = ezTime::Now();

  s_iCurrentMemoryUsage += entry.m_Pixmap.width() * entry.m_Pixmap.height() * 4;
  
  // send event that something has been loaded
  g_ImageCacheSingleton.EmitLoadedSignal(sPath, index, UserData1, UserData2);

  // start the next task
  RunLoadingTask();
}

void QtImageCache::CleanupCache()
{
  EZ_LOCK(s_Mutex);

  if (s_iCurrentMemoryUsage < s_iMemoryUsageThreshold)
    return;

  const ezTime tNow = ezTime::Now();

  // do not clean up too often
  if (tNow - s_LastCleanupTime < ezTime::Seconds(10))
    return;

  s_LastCleanupTime = tNow;

  // purge everything older than 5 minutes, then 4 minutes, ...
  for (ezInt32 i = 5; i > 0; --i)
  {
    const ezTime tPurgeThreshold = ezTime::Seconds(60) * i;

    // purge ALL images that have not been accessed in a longer time
    for (auto it = s_ImageCache.GetIterator(); it.IsValid();)
    {
      if (tNow - it.Value().m_LastAccess > tPurgeThreshold)
      {
        // this image has not been accessed in a while, get rid of it

        s_iCurrentMemoryUsage -= it.Value().m_Pixmap.width() * it.Value().m_Pixmap.height() * 4;

        it = s_ImageCache.Remove(it);
      }
      else
        ++it;
    }

    // if we have reached the threshold, stop further purging
    if (s_iCurrentMemoryUsage < s_iMemoryUsageThreshold)
      return;
  }
}