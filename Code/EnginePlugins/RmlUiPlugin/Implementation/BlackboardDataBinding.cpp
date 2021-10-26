#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace ezRmlUiInternal
{
  BlackboardDataBinding::BlackboardDataBinding(ezBlackboard& blackboard)
    : m_Blackboard(blackboard)
  {
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  ezResult BlackboardDataBinding::Initialize(Rml::Context& context)
  {
    const char* szModelName = m_Blackboard.GetName();
    if (ezStringUtils::IsNullOrEmpty(szModelName))
    {
      ezLog::Error("Can't bind a blackboard without a valid name");
      return EZ_FAILURE;
    }

    Rml::DataModelConstructor constructor = context.CreateDataModel(szModelName);
    if (!constructor)
    {
      return EZ_FAILURE;
    }

    for (auto it : m_Blackboard.GetAllEntries())
    {
      m_EntryWrappers.emplace_back(m_Blackboard, it.Key(), it.Value().m_uiChangeCounter);
    }

    for (auto& wrapper : m_EntryWrappers)
    {
      constructor.BindFunc(
        wrapper.m_sName.GetData(),
        [&](Rml::Variant& out_Value) { wrapper.GetValue(out_Value); },
        [&](const Rml::Variant& value) { wrapper.SetValue(value); });
    }

    m_hDataModel = constructor.GetModelHandle();

    m_uiBlackboardChangeCounter = m_Blackboard.GetBlackboardChangeCounter();
    m_uiBlackboardEntryChangeCounter = m_Blackboard.GetBlackboardEntryChangeCounter();

    return EZ_SUCCESS;
  }

  void BlackboardDataBinding::Deinitialize(Rml::Context& context)
  {
    context.RemoveDataModel(m_Blackboard.GetName());
  }

  void BlackboardDataBinding::Update()
  {
    if (m_uiBlackboardChangeCounter != m_Blackboard.GetBlackboardChangeCounter())
    {
      ezLog::Warning("Data Binding doesn't work with values that are registered or unregistered after setup");
      m_uiBlackboardChangeCounter = m_Blackboard.GetBlackboardChangeCounter();
    }

    if (m_uiBlackboardEntryChangeCounter != m_Blackboard.GetBlackboardEntryChangeCounter())
    {
      for (auto& wrapper : m_EntryWrappers)
      {
        auto pEntry = m_Blackboard.GetEntry(wrapper.m_sName);

        if (pEntry != nullptr && wrapper.m_uiChangeCounter != pEntry->m_uiChangeCounter)
        {
          m_hDataModel.DirtyVariable(wrapper.m_sName.GetData());
          wrapper.m_uiChangeCounter = pEntry->m_uiChangeCounter;
        }
      }

      m_uiBlackboardEntryChangeCounter = m_Blackboard.GetBlackboardEntryChangeCounter();
    }
  }

  //////////////////////////////////////////////////////////////////////////

  void BlackboardDataBinding::EntryWrapper::SetValue(const Rml::Variant& value)
  {
    ezVariant::Type::Enum targetType = ezVariant::Type::Invalid;
    if (auto pEntry = m_Blackboard.GetEntry(m_sName))
    {
      targetType = pEntry->m_Value.GetType();
    }

    if (m_Blackboard.SetEntryValue(m_sName, ezRmlUiConversionUtils::ToVariant(value, targetType)).Failed())
    {
      ezLog::Error("RmlUI: Can't set blackboard entry '{}', because it doesn't exist.", m_sName);
    }
  }

  void BlackboardDataBinding::EntryWrapper::GetValue(Rml::Variant& out_Value) const
  {
    out_Value = ezRmlUiConversionUtils::ToVariant(m_Blackboard.GetEntryValue(m_sName));
  }

} // namespace ezRmlUiInternal
