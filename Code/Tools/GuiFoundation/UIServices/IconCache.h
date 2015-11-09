#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <QImage>
#include <QIcon>
#include <QPixmap>

/// \brief A simple cache to prevent creating QIcons, QPixmaps and QImages over and over (which can be very costly).
/// Use this to quickly get such an object out of an internal Qt resource
///
/// E.g. instead of returning something like 'QIcon(QStringLiteral(":QtNamespace/MyIcon.png"))' instead return 'ezQtIconCache::GetIcon(":QtNamespace/MyIcon.png")'
/// This way the QIcon is only created once, instead of every time it is queried.
class EZ_GUIFOUNDATION_DLL ezQtIconCache
{
public:

  static const QIcon& GetIcon(const char* szIdentifier);
  static const QImage& GetImage(const char* szIdentifier);
  static const QPixmap& GetPixmap(const char* szIdentifier);


private:
  static ezMap<ezString, QIcon> s_Icons;
  static ezMap<ezString, QImage> s_Images;
  static ezMap<ezString, QPixmap> s_Pixmaps;
};