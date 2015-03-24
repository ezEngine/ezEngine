#pragma once

#include <GuiFoundation/Basics.h>
#include <QObject>

class QAction;
class QMenu;
class ezAction;

class EZ_GUIFOUNDATION_DLL ezQtProxy : public QObject
{
  Q_OBJECT

public:
  ezQtProxy();
  virtual ~ezQtProxy();

  virtual void Update() = 0;

  virtual void SetAction(ezAction* pAction);
  ezAction* GetAction() { return m_pAction; }

protected:
  ezAction* m_pAction;
};

class EZ_GUIFOUNDATION_DLL ezQtActionProxy : public ezQtProxy
{
  Q_OBJECT

public:
  virtual QAction* GetQAction() = 0;

};

class EZ_GUIFOUNDATION_DLL ezQtMenuProxy : public ezQtProxy
{
  Q_OBJECT

public:
  ezQtMenuProxy();
  ~ezQtMenuProxy();

  virtual void Update() override;
  virtual void SetAction(ezAction* pAction) override;

  virtual QMenu* GetQMenu();

private:
  QMenu* m_pMenu;
};

class EZ_GUIFOUNDATION_DLL ezQtButtonProxy : public ezQtActionProxy
{
  Q_OBJECT

public:
  ezQtButtonProxy();
  ~ezQtButtonProxy();

  virtual void Update() override;
  virtual void SetAction(ezAction* pAction) override;

  virtual QAction* GetQAction() override;

private slots:
  void OnTriggered();

private:
  void StatusUpdateEventHandler(ezAction* pAction);

private:
  QAction* m_pQtAction;
};