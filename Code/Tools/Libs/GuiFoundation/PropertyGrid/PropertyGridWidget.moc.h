#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

class QSpacerItem;
class QVBoxLayout;
class QScrollArea;

class ezQtGroupBoxBase;
class ezDocument;
class ezDocumentObjectManager;
class ezCommandHistory;
class ezObjectAccessorBase;
struct ezDocumentObjectPropertyEvent;
struct ezPropertyMetaStateEvent;
struct ezObjectAccessorChangeEvent;
struct ezPropertyDefaultEvent;
struct ezContainerElementMetaStateEvent;

class EZ_GUIFOUNDATION_DLL ezQtPropertyGridWidget : public QWidget
{
  Q_OBJECT
public:
  ezQtPropertyGridWidget(QWidget* pParent, ezDocument* pDocument = nullptr, bool bBindToSelectionManager = true);
  ~ezQtPropertyGridWidget();

  void SetDocument(ezDocument* pDocument, bool bBindToSelectionManager = true);

  void ClearSelection();
  void SetSelectionIncludeExcludeProperties(const char* szIncludeProperties = nullptr, const char* szExcludeProperties = nullptr);
  void SetSelection(const ezDeque<const ezDocumentObject*>& selection);
  const ezDocument* GetDocument() const;
  const ezDocumentObjectManager* GetObjectManager() const;
  ezCommandHistory* GetCommandHistory() const;
  ezObjectAccessorBase* GetObjectAccessor() const;

  static ezRttiMappedObjectFactory<ezQtPropertyWidget>& GetFactory();
  static ezQtPropertyWidget* CreateMemberPropertyWidget(const ezAbstractProperty* pProp);
  static ezQtPropertyWidget* CreatePropertyWidget(const ezAbstractProperty* pProp);

  void SetCollapseState(ezQtGroupBoxBase* pBox);

Q_SIGNALS:
  void ExtendContextMenu(QMenu& ref_menu, ezQtPropertyWidget* pPropWidget);

public Q_SLOTS:
  void OnCollapseStateChanged(bool bCollapsed);

private:
  static ezRttiMappedObjectFactory<ezQtPropertyWidget> s_Factory;
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, PropertyGrid);

private:
  void ObjectAccessorChangeEventHandler(const ezObjectAccessorChangeEvent& e);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void FactoryEventHandler(const ezRttiMappedObjectFactory<ezQtPropertyWidget>::Event& e);
  void TypeEventHandler(const ezPhantomRttiManagerEvent& e);
  ezUInt32 GetGroupBoxHash(ezQtGroupBoxBase* pBox) const;

private:
  ezDocument* m_pDocument;
  bool m_bBindToSelectionManager = false;
  ezDeque<const ezDocumentObject*> m_Selection;
  ezMap<ezUInt32, bool> m_CollapseState;
  ezString m_sSelectionIncludeProperties;
  ezString m_sSelectionExcludeProperties;

  QVBoxLayout* m_pLayout;
  QScrollArea* m_pScroll;
  QWidget* m_pContent;
  QVBoxLayout* m_pContentLayout;

  ezQtTypeWidget* m_pTypeWidget;
  QSpacerItem* m_pSpacer;
};
