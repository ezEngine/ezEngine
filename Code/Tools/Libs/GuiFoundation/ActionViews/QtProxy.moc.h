#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QWidget>
#include <QWidgetAction>

class QAction;
class QMenu;
class QLabel;
class QSlider;
class ezAction;

/// \brief Glue class that maps ezActions to QActions. QActions are only created if the ezAction is actually mapped somewhere. Document and Global actions are manually executed and don't solely rely on Qt's ShortcutContext setting to prevent ambiguous action shortcuts.
class EZ_GUIFOUNDATION_DLL ezQtProxy : public QObject
{
  Q_OBJECT

public:
  ezQtProxy();
  virtual ~ezQtProxy();

  virtual void Update() = 0;

  virtual void SetAction(ezAction* pAction);
  ezAction* GetAction() { return m_pAction; }

  /// \brief Converts the QKeyEvent into a shortcut and tries to find a matching action in the document and global action list.
  ///
  /// Document actions are not mapped as ShortcutContext::WindowShortcut because docking allows for multiple documents to be mapped into the same window. Instead, ShortcutContext::WidgetWithChildrenShortcut is used to prevent ambiguous action shortcuts and the actions are executed manually via filtering QEvent::ShortcutOverride at the dock widget level.
  /// The function always has to be called two times:
  /// A: QEvent::ShortcutOverride: Only check with bTestOnly = true that we want to override the shortcut. This will instruct Qt to send the event as a regular key press event to the widget that accepted the override.
  /// B: QEvent::keyPressEvent: Execute the actual action with bTestOnly = false;
  ///
  /// \param pDocument The document for which matching actions should be searched for. If null, only global actions are searched.
  /// \param pEvent The key event that should be converted into a shortcut.
  /// \param bTestOnly Accept the event and return true but don't execute the action. Use this inside QEvent::ShortcutOverride.
  /// \return Whether the key event was consumed and an action executed.
  static bool TriggerDocumentAction(ezDocument* pDocument, QKeyEvent* pEvent, bool bTestOnly);

  static ezRttiMappedObjectFactory<ezQtProxy>& GetFactory();
  static QSharedPointer<ezQtProxy> GetProxy(ezActionContext& ref_context, ezActionDescriptorHandle hAction);

protected:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, QtProxies);
  static ezRttiMappedObjectFactory<ezQtProxy> s_Factory;
  static ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>> s_GlobalActions;
  static ezMap<const ezDocument*, ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>>> s_DocumentActions;
  static ezMap<QWidget*, ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>>> s_WindowActions;
  static QObject* s_pSignalProxy;

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
  virtual void Update() override {}
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
  void StatusUpdateEventHandler(ezAction* pAction);

protected:
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

private Q_SLOTS:
  void OnTriggered();

private:
  void StatusUpdateEventHandler(ezAction* pAction);

private:
  QPointer<QAction> m_pQtAction;
};


class EZ_GUIFOUNDATION_DLL ezQtDynamicMenuProxy : public ezQtMenuProxy
{
  Q_OBJECT

public:
  virtual void SetAction(ezAction* pAction) override;

private Q_SLOTS:
  void SlotMenuAboutToShow();
  void SlotMenuEntryTriggered();

private:
  ezHybridArray<ezDynamicMenuAction::Item, 16> m_Entries;
};

class EZ_GUIFOUNDATION_DLL ezQtDynamicActionAndMenuProxy : public ezQtDynamicMenuProxy
{
  Q_OBJECT

public:
  ezQtDynamicActionAndMenuProxy();
  ~ezQtDynamicActionAndMenuProxy();

  virtual void Update() override;
  virtual void SetAction(ezAction* pAction) override;
  virtual QAction* GetQAction();

private Q_SLOTS:
  void OnTriggered();

private:
  QPointer<QAction> m_pQtAction;
};


class EZ_GUIFOUNDATION_DLL ezQtLabeledSlider : public QWidget
{
  Q_OBJECT

public:
  ezQtLabeledSlider(QWidget* pParent);

  QLabel* m_pLabel;
  QSlider* m_pSlider;
};


class EZ_GUIFOUNDATION_DLL ezQtSliderWidgetAction : public QWidgetAction
{
  Q_OBJECT

public:
  ezQtSliderWidgetAction(QWidget* pParent);
  void setMinimum(int value);
  void setMaximum(int value);
  void setValue(int value);

Q_SIGNALS:
  void valueChanged(int value);

private Q_SLOTS:
  void OnValueChanged(int value);

protected:
  virtual QWidget* createWidget(QWidget* parent) override;
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  ezInt32 m_iMinimum;
  ezInt32 m_iMaximum;
  ezInt32 m_iValue;
};

class EZ_GUIFOUNDATION_DLL ezQtSliderProxy : public ezQtActionProxy
{
  Q_OBJECT

public:
  ezQtSliderProxy();
  ~ezQtSliderProxy();

  virtual void Update() override;
  virtual void SetAction(ezAction* pAction) override;

  virtual QAction* GetQAction() override;

private Q_SLOTS:
  void OnValueChanged(int value);

private:
  void StatusUpdateEventHandler(ezAction* pAction);

private:
  QPointer<ezQtSliderWidgetAction> m_pQtAction;
};
