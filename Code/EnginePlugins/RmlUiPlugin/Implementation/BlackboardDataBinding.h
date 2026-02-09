#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <RmlUiPlugin/RmlUiDataBinding.h>

class ezBlackboard;

namespace ezRmlUiInternal
{
  struct EntryInfo
  {
    ezBlackboard* m_pBlackboard = nullptr;
    ezHashedString m_sName;
    ezUInt32 m_uiChangeCounter = 0;
    bool m_bIsArray = false;
  };

  class VariantVariableDefinition final : public Rml::VariableDefinition
  {
  public:
    VariantVariableDefinition();

    virtual bool Get(void* pPtr, Rml::Variant& out_variant) override;
    virtual bool Set(void* pPtr, const Rml::Variant& variant) override;
  };

  //////////////////////////////////////////////////////////////////

  class BlackboardVariableDefinition final : public Rml::VariableDefinition
  {
  public:
    BlackboardVariableDefinition(bool bIsArray);

    virtual bool Get(void* pPtr, Rml::Variant& out_variant) override;
    virtual bool Set(void* pPtr, const Rml::Variant& variant) override;

    virtual int Size(void* pPtr) override;
    virtual Rml::DataVariable Child(void* pPtr, const Rml::DataAddressEntry& address) override;

  private:
    VariantVariableDefinition m_ScalarDefinition;
  };

  /////////////////////////////////////////////////////////////////

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
    ezUInt32 m_uiBlackboardChangeCounter = 0;
    ezUInt32 m_uiBlackboardEntryChangeCounter = 0;

    Rml::DataModelHandle m_hDataModel;

    ezDynamicArray<EntryInfo> m_EntryInfos;

    BlackboardVariableDefinition m_ScalarDefinition;
    BlackboardVariableDefinition m_ArrayDefinition;
  };
} // namespace ezRmlUiInternal
