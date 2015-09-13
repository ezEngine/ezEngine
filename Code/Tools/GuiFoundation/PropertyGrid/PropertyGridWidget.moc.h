#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <QWidget>

class QSpacerItem;
class QVBoxLayout;
class QScrollArea;

class ezCollapsibleGroupBox;
class ezDocumentBase;
struct ezDocumentObjectPropertyEvent;

class EZ_GUIFOUNDATION_DLL ezPropertyGridWidget : public QWidget
{
  Q_OBJECT
public:
  ezPropertyGridWidget(ezDocumentBase* pDocument, QWidget* pParent);
  ~ezPropertyGridWidget();

  void ClearSelection();
  void SetSelection(const ezDeque<const ezDocumentObjectBase*>& selection);
  const ezDocumentBase* GetDocument() const;

  static ezRttiMappedObjectFactory<ezPropertyBaseWidget>& GetFactory();
  static ezPropertyBaseWidget* CreateMemberPropertyWidget(const ezAbstractProperty* pProp);
  static ezPropertyBaseWidget* CreatePropertyWidget(const ezAbstractProperty* pProp);

  void SetCollapseState(ezCollapsibleGroupBox* pBox);

public slots:
  void OnCollapseStateChanged(bool bCollapsed);

private:
  static ezRttiMappedObjectFactory<ezPropertyBaseWidget> s_Factory;
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, PropertyGrid);

private:
  void SelectionEventHandler(const ezSelectionManager::Event& e);
  void FactoryEventHandler(const ezRttiMappedObjectFactory<ezPropertyBaseWidget>::Event& e);
  void TypeEventHandler(const ezPhantomRttiManager::Event& e);
  ezUInt32 GetGroupBoxHash(ezCollapsibleGroupBox* pBox) const;

private:
  ezDocumentBase* m_pDocument;
  ezDeque<const ezDocumentObjectBase*> m_Selection;
  ezMap<ezUInt32, bool> m_CollapseState;

  QVBoxLayout* m_pLayout;
  QScrollArea* m_pScroll;
  QWidget* m_pContent;
  QVBoxLayout* m_pContentLayout;

  ezTypeWidget* m_pTypeWidget;
  QSpacerItem* m_pSpacer;
};


