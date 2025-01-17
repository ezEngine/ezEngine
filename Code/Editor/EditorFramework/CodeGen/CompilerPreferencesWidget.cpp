#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CompilerPreferencesWidget.moc.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtCompilerPreferencesWidget::ezQtCompilerPreferencesWidget()
  : ezQtPropertyTypeWidget(true)
{
  m_pCompilerPreset = new QComboBox();
  int counter = 0;
  for (auto& compiler : ezCppProject::GetMachineSpecificCompilers())
  {
    m_pCompilerPreset->addItem(ezMakeQString(compiler.m_sNiceName), counter);
    ++counter;
  }
  connect(m_pCompilerPreset, &QComboBox::currentIndexChanged, this, &ezQtCompilerPreferencesWidget::on_compiler_preset_changed);

  auto gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(0, 1);
  gridLayout->setColumnStretch(1, 0);
  gridLayout->setColumnMinimumWidth(1, 5);
  gridLayout->setColumnStretch(2, 2);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setSpacing(0);

  ezStringBuilder fmt;
  QLabel* versionText = new QLabel(ezMakeQString(
    ezFmt("This SDK was compiled with {} version {}. Select a compatible compiler.",
      ezCppProject::CompilerToString(ezCppProject::GetSdkCompiler()),
      ezCppProject::GetSdkCompilerMajorVersion())
      .GetText(fmt)));
  versionText->setWordWrap(true);
  gridLayout->addWidget(versionText, 0, 0, 1, 3);

  gridLayout->addWidget(new QLabel("Compiler Preset"), 1, 0);
  gridLayout->addWidget(m_pCompilerPreset, 1, 2);
  m_pGroupLayout->addLayout(gridLayout);
}

ezQtCompilerPreferencesWidget::~ezQtCompilerPreferencesWidget() = default;

void ezQtCompilerPreferencesWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtScopedUpdatesDisabled _(this);

  ezQtPropertyTypeWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    const auto& selection = m_pTypeWidget->GetSelection();

    EZ_ASSERT_DEBUG(selection.GetCount() == 1, "Expected exactly one object");
    auto pObj = selection[0].m_pObject;

    ezEnum<ezCompiler> m_Compiler;
    bool bIsCustomCompiler;
    ezString m_sCCompiler, m_sCppCompiler;

    {
      ezVariant varCompiler, varIsCustomCompiler, varCCompiler, varCppCompiler;

      m_pObjectAccessor->GetValueByName(pObj, "Compiler", varCompiler).AssertSuccess();
      m_pObjectAccessor->GetValueByName(pObj, "CustomCompiler", varIsCustomCompiler).AssertSuccess();
      m_pObjectAccessor->GetValueByName(pObj, "CCompiler", varCCompiler).AssertSuccess();
      m_pObjectAccessor->GetValueByName(pObj, "CppCompiler", varCppCompiler).AssertSuccess();

      m_Compiler.SetValue(static_cast<ezCompiler::StorageType>(varCompiler.Get<ezInt64>()));
      bIsCustomCompiler = varIsCustomCompiler.Get<decltype(bIsCustomCompiler)>();
      m_sCCompiler = varCCompiler.Get<decltype(m_sCCompiler)>();
      m_sCppCompiler = varCppCompiler.Get<decltype(m_sCppCompiler)>();
    }

    ezInt32 selectedIndex = -1;
    const auto& machineSpecificCompilers = ezCppProject::GetMachineSpecificCompilers();
    // first look for non custom compilers
    for (ezUInt32 i = 0; i < machineSpecificCompilers.GetCount(); ++i)
    {
      const auto& curCompiler = machineSpecificCompilers[i];
      if ((curCompiler.m_bIsCustom == false) &&
          (curCompiler.m_Compiler == m_Compiler) &&
          (curCompiler.m_sCCompiler == m_sCCompiler) &&
          (curCompiler.m_sCppCompiler == m_sCppCompiler))
      {
        selectedIndex = static_cast<int>(i);
        break;
      }
    }

    if (selectedIndex == -1)
    {
      // If we didn't find a system default compiler, look for custom compilers next
      for (ezUInt32 i = 0; i < machineSpecificCompilers.GetCount(); ++i)
      {
        const auto& curCompiler = machineSpecificCompilers[i];
        if (curCompiler.m_bIsCustom == true && curCompiler.m_Compiler == m_Compiler)
        {
          selectedIndex = static_cast<int>(i);
          break;
        }
      }
    }

    if (selectedIndex >= 0)
    {
      m_pCompilerPreset->blockSignals(true);
      m_pCompilerPreset->setCurrentIndex(selectedIndex);
      m_pCompilerPreset->blockSignals(false);
    }
  }
}

void ezQtCompilerPreferencesWidget::on_compiler_preset_changed(int index)
{
  auto compilerPresets = ezCppProject::GetMachineSpecificCompilers();

  if (index >= 0 && index < (int)compilerPresets.GetCount())
  {
    const auto& preset = compilerPresets[index];

    const auto& selection = m_pTypeWidget->GetSelection();
    EZ_ASSERT_DEV(selection.GetCount() == 1, "This Widget does not support multi selection");

    auto obj = selection[0].m_pObject;
    m_pObjectAccessor->StartTransaction("Change Compiler Preset");
    m_pObjectAccessor->SetValueByName(obj, "Compiler", preset.m_Compiler.GetValue()).AssertSuccess();
    m_pObjectAccessor->SetValueByName(obj, "CustomCompiler", preset.m_bIsCustom).AssertSuccess();
    m_pObjectAccessor->SetValueByName(obj, "CCompiler", preset.m_sCCompiler).AssertSuccess();
    m_pObjectAccessor->SetValueByName(obj, "CppCompiler", preset.m_sCppCompiler).AssertSuccess();
    m_pObjectAccessor->FinishTransaction();
  }
}

void ezCompilerPreferences_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  const ezRTTI* pRtti = ezGetStaticRTTI<ezCompilerPreferences>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  ezPropertyUiState::Visibility compilerFieldsVisibility = ezPropertyUiState::Default;

  bool bCustomCompiler = typeAccessor.GetValue("CustomCompiler").Get<bool>();
  if (!bCustomCompiler)
  {
    compilerFieldsVisibility = ezPropertyUiState::Disabled;
  }
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  auto compiler = typeAccessor.GetValue("Compiler").Get<ezInt64>();
  if (compiler == ezCompiler::Vs2022)
  {
    compilerFieldsVisibility = ezPropertyUiState::Invisible;
  }
#endif

  auto& props = *e.m_pPropertyStates;

  props["CCompiler"].m_Visibility = compilerFieldsVisibility;
  props["CppCompiler"].m_Visibility = compilerFieldsVisibility;
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  props["RcCompiler"].m_Visibility = ezPropertyUiState::Invisible;
#else
  props["RcCompiler"].m_Visibility = (compiler == ezCompiler::Vs2022) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
#endif
}
