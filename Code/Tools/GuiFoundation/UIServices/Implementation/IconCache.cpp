#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/IconCache.h>

ezMap<ezString, QIcon> ezQtIconCache::s_Icons;
ezMap<ezString, QImage> ezQtIconCache::s_Images;
ezMap<ezString, QPixmap> ezQtIconCache::s_Pixmaps;

const QIcon& ezQtIconCache::GetIcon(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_Icons;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QIcon(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

const QImage& ezQtIconCache::GetImage(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_Images;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QImage(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

const QPixmap& ezQtIconCache::GetPixmap(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_Pixmaps;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QPixmap(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}


