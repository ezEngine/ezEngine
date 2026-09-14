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
    Rml::DataVariableType m_Type = Rml::DataVariableType::Scalar;

    /// Cached copy of the blackboard value. RmlUi holds raw pointers into this value (and into
    /// its nested elements), so it has to stay alive and at a stable address for as long as the
    /// data model exists. It is refreshed in BlackboardDataBinding::Update.
    ezVariant m_CachedValue;
  };

  //////////////////////////////////////////////////////////////////

  class VariantDefinitionSet;

  /// \brief Exposes an ezVariant that is nested inside an array or a dictionary. Read-only.
  class VariantVariableDefinition final : public Rml::VariableDefinition
  {
  public:
    VariantVariableDefinition(Rml::DataVariableType type, const VariantDefinitionSet& definitions);

    virtual bool Get(void* pPtr, Rml::Variant& out_variant) override;
    virtual bool Set(void* pPtr, const Rml::Variant& variant) override;

    virtual int Size(void* pPtr) override;
    virtual Rml::DataVariable Child(void* pPtr, const Rml::DataAddressEntry& address) override;
    virtual Rml::StringList ReflectMemberNames() override;

  private:
    const VariantDefinitionSet& m_Definitions;
  };

  //////////////////////////////////////////////////////////////////

  /// \brief Owns one VariantVariableDefinition per data variable type and hands out the matching
  /// one for a given value, so that nested arrays and dictionaries can be traversed.
  ///
  /// This indirection is needed because Rml::VariableDefinition::Type() is fixed at construction
  /// time, while the type of an ezVariant is only known at runtime.
  class VariantDefinitionSet
  {
  public:
    VariantDefinitionSet();

    /// \brief Returns a DataVariable that exposes the given value with the matching definition.
    /// The value has to outlive the returned DataVariable.
    Rml::DataVariable GetDefinition(const ezVariant& value) const;

  private:
    VariantVariableDefinition m_Scalar;
    VariantVariableDefinition m_Array;
    VariantVariableDefinition m_Struct;
  };

  //////////////////////////////////////////////////////////////////

  /// \brief Exposes a top level blackboard entry.
  class BlackboardVariableDefinition final : public Rml::VariableDefinition
  {
  public:
    BlackboardVariableDefinition(Rml::DataVariableType type, const VariantDefinitionSet& definitions);

    virtual bool Get(void* pPtr, Rml::Variant& out_variant) override;
    virtual bool Set(void* pPtr, const Rml::Variant& variant) override;

    virtual int Size(void* pPtr) override;
    virtual Rml::DataVariable Child(void* pPtr, const Rml::DataAddressEntry& address) override;
    virtual Rml::StringList ReflectMemberNames() override;

  private:
    const VariantDefinitionSet& m_Definitions;
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

    VariantDefinitionSet m_VariantDefinitions;
    BlackboardVariableDefinition m_ScalarDefinition;
    BlackboardVariableDefinition m_ArrayDefinition;
    BlackboardVariableDefinition m_StructDefinition;
  };
} // namespace ezRmlUiInternal
