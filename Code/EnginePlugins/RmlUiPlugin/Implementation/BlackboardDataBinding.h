#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <RmlUiPlugin/RmlUiDataBinding.h>

class ezBlackboard;

namespace ezRmlUiInternal
{
  class BlackboardDataBinding final : public ezRmlUiDataBinding
  {
  public:
    BlackboardDataBinding(const ezSharedPtr<ezBlackboard>& pBlackboard);
    ~BlackboardDataBinding();

    virtual ezResult Initialize(Rml::Context& ref_context) override;
    virtual void Deinitialize(Rml::Context& ref_context) override;
    virtual bool Update() override;

  private:
    ezSharedPtr<ezBlackboard> m_pBlackboard;

    Rml::DataModelHandle m_hDataModel;

    struct EntryWrapper
    {
      EntryWrapper(ezBlackboard& ref_blackboard, const ezHashedString& sName, ezUInt32 uiChangeCounter)
        : m_Blackboard(ref_blackboard)
        , m_sName(sName)
        , m_uiChangeCounter(uiChangeCounter)
      {
      }

      void SetValue(const Rml::Variant& value);
      void GetValue(Rml::Variant& out_value) const;

      ezBlackboard& m_Blackboard;
      ezHashedString m_sName;
      ezUInt32 m_uiChangeCounter;
    };

    Rml::Vector<EntryWrapper> m_EntryWrappers;

    ezUInt32 m_uiBlackboardChangeCounter = 0;
    ezUInt32 m_uiBlackboardEntryChangeCounter = 0;
  };
} // namespace ezRmlUiInternal
