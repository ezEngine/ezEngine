#pragma once

#include <Foundation/Containers/Set.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QObject>

class ezQtSearchableMenu;
class ezRTTI;
class QMenu;

class EZ_GUIFOUNDATION_DLL ezQtTypeMenu : public QObject
{
  Q_OBJECT

public:
  void FillMenu(QMenu* pMenu, const ezRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu);

  static bool s_bShowInDevelopmentFeatures;

Q_SIGNALS:
  void TypeSelected(QString sTypeName);

protected Q_SLOTS:
  void OnMenuAction();

private:
  QMenu* CreateCategoryMenu(const char* szCategory, ezMap<ezString, QMenu*>& existingMenus);

  QMenu* m_pMenu = nullptr;
  ezSet<const ezRTTI*> m_SupportedTypes;
  ezQtSearchableMenu* m_pSearchableMenu = nullptr;

  static ezString s_sLastMenuSearch;
};
