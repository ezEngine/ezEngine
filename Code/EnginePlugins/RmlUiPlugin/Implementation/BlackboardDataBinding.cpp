#include <RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace ezRmlUiInternal
{
  BlackboardDataBinding::BlackboardDataBinding(ezBlackboard& blackboard, const char* szModelName)
    : m_Blackboard(blackboard)
  {
    m_sModelName.Assign(szModelName);
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  ezResult BlackboardDataBinding::Setup(Rml::Context& context)
  {
    Rml::DataModelConstructor constructor = context.CreateDataModel(m_sModelName.GetData());
    if (!constructor)
    {
      return EZ_FAILURE;
    }

    for (auto it : m_Blackboard.GetAllEntries())
    {
      m_EntryWrapper.emplace_back(m_Blackboard, it.Key());
    }

    for (auto& wrapper : m_EntryWrapper)
    {
      constructor.BindFunc(wrapper.m_sName.GetData(),
        [&](Rml::Variant& out_Value) { wrapper.GetValue(out_Value); },
        [&](const Rml::Variant& value) { wrapper.SetValue(value); });
    }

    m_hDataModel = constructor.GetModelHandle();

    return EZ_SUCCESS;
  }

  void BlackboardDataBinding::Update() { m_hDataModel.Update(); }

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

  void BlackboardDataBinding::EntryWrapper::GetValue(Rml::Variant& out_Value) const
  {
    out_Value = ezRmlUiConversionUtils::ToVariant(m_Blackboard.GetEntryValue(m_sName));
  }

} // namespace ezRmlUiInternal
