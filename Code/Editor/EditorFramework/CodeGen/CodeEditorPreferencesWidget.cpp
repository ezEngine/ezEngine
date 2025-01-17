#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CodeEditorPreferencesWidget.moc.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtCodeEditorPreferencesWidget::ezQtCodeEditorPreferencesWidget()
  : ezQtPropertyTypeWidget(true)
{
  m_pCodeEditor = new QComboBox();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  m_pCodeEditor->addItem(ezMakeQString("Visual Studio"), true);
#endif
  m_pCodeEditor->addItem(ezMakeQString("Custom"), false);

  connect(m_pCodeEditor, &QComboBox::currentIndexChanged, this, &ezQtCodeEditorPreferencesWidget::on_code_editor_changed);

  auto gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(0, 1);
  gridLayout->setColumnStretch(1, 0);
  gridLayout->setColumnMinimumWidth(1, 5);
  gridLayout->setColumnStretch(2, 2);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setSpacing(0);

  gridLayout->addWidget(new QLabel("Code Editor"), 1, 0);
  gridLayout->addWidget(m_pCodeEditor, 1, 2);
  m_pGroupLayout->addLayout(gridLayout);
}

ezQtCodeEditorPreferencesWidget::~ezQtCodeEditorPreferencesWidget() = default;

void ezQtCodeEditorPreferencesWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtScopedUpdatesDisabled _(this);

  ezQtPropertyTypeWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    const auto& selection = m_pTypeWidget->GetSelection();

    EZ_ASSERT_DEBUG(selection.GetCount() == 1, "Expected exactly one object");
    auto pObj = selection[0].m_pObject;

    ezVariant varIsVisualStudio;
    m_pObjectAccessor->GetValueByName(pObj, "IsVisualStudio", varIsVisualStudio).AssertSuccess();
    bool bIsVisualStudio = varIsVisualStudio.Get<decltype(bIsVisualStudio)>();

    m_pCodeEditor->blockSignals(true);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    m_pCodeEditor->setCurrentIndex(bIsVisualStudio ? 0 : 1);
#else
    m_pCodeEditor->setCurrentIndex(0);
#endif
    m_pCodeEditor->blockSignals(false);
  }
}

void ezQtCodeEditorPreferencesWidget::on_code_editor_changed(int index)
{
  const auto& selection = m_pTypeWidget->GetSelection();
  EZ_ASSERT_DEV(selection.GetCount() == 1, "This Widget does not support multi selection");

  const QVariant variant = m_pCodeEditor->currentData();
  if (variant.toBool())
  {
    auto obj = selection[0].m_pObject;
    m_pObjectAccessor->StartTransaction("Change Code Editor Preset");
    m_pObjectAccessor->SetValueByName(obj, "IsVisualStudio", true).AssertSuccess();
    m_pObjectAccessor->FinishTransaction();
    return;
  }

  auto obj = selection[0].m_pObject;
  m_pObjectAccessor->StartTransaction("Change Code Editor Preset");
  m_pObjectAccessor->SetValueByName(obj, "IsVisualStudio", false).AssertSuccess();

  ezVariant editorArgs;
  if (m_pObjectAccessor->GetValueByName(obj, "CodeEditorArgs", editorArgs).Succeeded() && editorArgs.Get<ezString>().IsEmpty())
  {
    m_pObjectAccessor->SetValueByName(obj, "CodeEditorArgs", "{file} {line}").AssertSuccess();
  }
  m_pObjectAccessor->FinishTransaction();
}

void ezCodeEditorPreferences_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  const ezRTTI* pRtti = ezGetStaticRTTI<ezCodeEditorPreferences>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  ezPropertyUiState::Visibility codeEditorFieldsVisibility = ezPropertyUiState::Default;

  ezStatus res;
  if (typeAccessor.GetValue("IsVisualStudio", ezVariant(), &res).Get<bool>() && res.Succeeded())
  {
    codeEditorFieldsVisibility = ezPropertyUiState::Invisible;
  }

  auto& props = *e.m_pPropertyStates;
  props["CodeEditorPath"].m_Visibility = codeEditorFieldsVisibility;
  props["CodeEditorArgs"].m_Visibility = codeEditorFieldsVisibility;
}
