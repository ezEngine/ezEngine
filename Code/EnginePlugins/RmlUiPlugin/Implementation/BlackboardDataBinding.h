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
      EntryWrapper(ezBlackboard& blackboard, const ezHashedString& sName)
        : m_Blackboard(blackboard)
        , m_sName(sName)
      {
      }

      void SetValue(const Rml::Variant& value);
      void GetValue(Rml::Variant& out_Value) const;

      ezBlackboard& m_Blackboard;
      ezHashedString m_sName;
    };

    Rml::Vector<EntryWrapper> m_EntryWrapper;
  };
} // namespace ezRmlUiInternal
