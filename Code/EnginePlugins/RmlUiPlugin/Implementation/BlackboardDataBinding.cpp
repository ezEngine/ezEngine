#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace ezRmlUiInternal
{
  BlackboardDataBinding::BlackboardDataBinding(const ezSharedPtr<ezBlackboard>& pBlackboard)
    : m_pBlackboard(pBlackboard)
  {
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  ezResult BlackboardDataBinding::Initialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard == nullptr)
      return EZ_FAILURE;

    const char* szModelName = m_pBlackboard->GetName();
    if (ezStringUtils::IsNullOrEmpty(szModelName))
    {
      ezLog::Error("Can't bind a blackboard without a valid name");
      return EZ_FAILURE;
    }

    Rml::DataModelConstructor constructor = ref_context.CreateDataModel(szModelName);
    if (!constructor)
    {
      return EZ_FAILURE;
    }

    for (auto it : m_pBlackboard->GetAllEntries())
    {
      m_EntryWrappers.emplace_back(*m_pBlackboard, it.Key(), it.Value().m_uiChangeCounter);
    }

    for (auto& wrapper : m_EntryWrappers)
    {
      constructor.BindFunc(
        wrapper.m_sName.GetData(),
        [&](Rml::Variant& out_value)
        { wrapper.GetValue(out_value); },
        [&](const Rml::Variant& value)
        { wrapper.SetValue(value); });
    }

    m_hDataModel = constructor.GetModelHandle();

    m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();

    return EZ_SUCCESS;
  }

  void BlackboardDataBinding::Deinitialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard != nullptr)
    {
      ref_context.RemoveDataModel(m_pBlackboard->GetName());
    }
  }

  bool BlackboardDataBinding::Update()
  {
    bool bUpdated = false;

    if (m_uiBlackboardChangeCounter != m_pBlackboard->GetBlackboardChangeCounter())
    {
      ezLog::Warning("Data Binding doesn't work with values that are registered or unregistered after setup");
      m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    }

    if (m_uiBlackboardEntryChangeCounter != m_pBlackboard->GetBlackboardEntryChangeCounter())
    {
      for (auto& wrapper : m_EntryWrappers)
      {
        auto pEntry = m_pBlackboard->GetEntry(wrapper.m_sName);

        if (pEntry != nullptr && wrapper.m_uiChangeCounter != pEntry->m_uiChangeCounter)
        {
          m_hDataModel.DirtyVariable(wrapper.m_sName.GetData());
          wrapper.m_uiChangeCounter = pEntry->m_uiChangeCounter;
          bUpdated = true;
        }
      }

      m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();
    }

    return bUpdated;
  }

  //////////////////////////////////////////////////////////////////////////

  void BlackboardDataBinding::EntryWrapper::SetValue(const Rml::Variant& value)
  {
    ezVariant::Type::Enum targetType = ezVariant::Type::Invalid;
    if (auto pEntry = m_Blackboard.GetEntry(m_sName))
    {
      targetType = pEntry->m_Value.GetType();
    }

    m_Blackboard.SetEntryValue(m_sName, ezRmlUiConversionUtils::ToVariant(value, targetType));
  }

  void BlackboardDataBinding::EntryWrapper::GetValue(Rml::Variant& out_value) const
  {
    out_value = ezRmlUiConversionUtils::ToVariant(m_Blackboard.GetEntryValue(m_sName));
  }

} // namespace ezRmlUiInternal
