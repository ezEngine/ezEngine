#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

#include <QObject>
#include <QPointer>
#include <QSharedPointer>

class QAction;
class QMenu;
class ezAction;

class EZ_GUIFOUNDATION_DLL ezQtProxy : public QObject
{
  Q_OBJECT

public:
  ezQtProxy();
  virtual ~ezQtProxy();

  virtual void Update(bool bSetShortcut) = 0;

  virtual void SetAction(ezAction* pAction, bool bSetShortcut);
  ezAction* GetAction() { return m_pAction; }

  static ezRttiMappedObjectFactory<ezQtProxy>& GetFactory();
  static QSharedPointer<ezQtProxy> GetProxy(ezActionContext& context, ezActionDescriptorHandle hAction);

protected:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, QtProxies);
  static ezRttiMappedObjectFactory<ezQtProxy> s_Factory;
  static ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>> s_GlobalActions;
  static ezMap<ezUuid, ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>> > s_DocumentActions;
  static ezMap<QWidget*, ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>> > s_WindowActions;

protected:
  ezAction* m_pAction;
};

class EZ_GUIFOUNDATION_DLL ezQtActionProxy : public ezQtProxy
{
  Q_OBJECT

public:
  virtual QAction* GetQAction() = 0;

};

class EZ_GUIFOUNDATION_DLL ezQtCategoryProxy : public ezQtProxy
{
  Q_OBJECT
public:
  virtual void Update(bool bSetShortcut) override {}
};

class EZ_GUIFOUNDATION_DLL ezQtMenuProxy : public ezQtProxy
{
  Q_OBJECT

public:
  ezQtMenuProxy();
  ~ezQtMenuProxy();

  virtual void Update(bool bSetShortcut) override;
  virtual void SetAction(ezAction* pAction, bool bSetShortcut) override;

  virtual QMenu* GetQMenu();

protected:
  QMenu* m_pMenu;
};

class EZ_GUIFOUNDATION_DLL ezQtButtonProxy : public ezQtActionProxy
{
  Q_OBJECT

public:
  ezQtButtonProxy();
  ~ezQtButtonProxy();

  virtual void Update(bool bSetShortcut) override;
  virtual void SetAction(ezAction* pAction, bool bSetShortcut) override;

  virtual QAction* GetQAction() override;

private slots:
  void OnTriggered();

private:
  void StatusUpdateEventHandler(ezAction* pAction);

private:
  QPointer<QAction> m_pQtAction;
};


class EZ_GUIFOUNDATION_DLL ezQtLRUMenuProxy : public ezQtMenuProxy
{
  Q_OBJECT

public:
  virtual void SetAction(ezAction* pAction, bool bSetShortcut) override;

private slots:
  void SlotMenuAboutToShow();
  void SlotMenuEntryTriggered();

private:
  ezHybridArray<ezLRUMenuAction::Item, 16> m_Entries;
};
