#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RmlUiPlugin/RmlUiDataBinding.h>

class ezBlackboard;

namespace ezRmlUiInternal
{
  class BlackboardDataBinding final : public ezRmlUiDataBinding
  {
  public:
    BlackboardDataBinding(ezBlackboard& blackboard, const char* szModelName);
    ~BlackboardDataBinding();

    virtual ezResult Setup(Rml::Context& context) override;
    virtual void Update() override;

  private:
    ezBlackboard& m_Blackboard;
    ezHashedString m_sModelName;

    Rml::DataModelHandle m_hDataModel;

    struct EntryWrapper
    {
      EntryWrapper(ezBlackboard& blackboard, const ezHashedString& sName, ezUInt32 uiChangeCounter)
        : m_Blackboard(blackboard)
        , m_sName(sName)
        , m_uiChangeCounter(uiChangeCounter)
      {
      }

      void SetValue(const Rml::Variant& value);
      void GetValue(Rml::Variant& out_Value) const;

      ezBlackboard& m_Blackboard;
      ezHashedString m_sName;
      ezUInt32 m_uiChangeCounter;
    };

    Rml::Vector<EntryWrapper> m_EntryWrappers;

    ezUInt32 m_uiBlackboardChangeCounter = 0;
    ezUInt32 m_uiBlackboardEntryChangeCounter = 0;
  };
} // namespace ezRmlUiInternal
