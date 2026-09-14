#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace ezRmlUiInternal
{
  Rml::DataVariableType GetVariableType(const ezVariant& value)
  {
    if (value.IsA<ezVariantArray>())
      return Rml::DataVariableType::Array;

    if (value.IsA<ezVariantDictionary>())
      return Rml::DataVariableType::Struct;

    return Rml::DataVariableType::Scalar;
  }

  /// Shared implementation for looking up a child of an array or dictionary value.
  Rml::DataVariable GetChild(const ezVariant& value, Rml::DataVariableType type, const Rml::DataAddressEntry& address, const VariantDefinitionSet& definitions)
  {
    if (type == Rml::DataVariableType::Array)
    {
      if (!value.IsA<ezVariantArray>())
        return Rml::DataVariable();

      const ezVariantArray& a = value.Get<ezVariantArray>();

      const int index = address.index;
      const int count = static_cast<int>(a.GetCount());
      if (index < 0 || index >= count)
      {
        if (address.name == "size")
          return Rml::MakeLiteralIntVariable(count);

        ezLog::Warning("Data array index out of bounds.");
        return Rml::DataVariable();
      }

      return definitions.GetDefinition(a[index]);
    }

    if (type == Rml::DataVariableType::Struct)
    {
      if (!value.IsA<ezVariantDictionary>())
        return Rml::DataVariable();

      if (address.name.empty())
      {
        ezLog::Warning("Expected a dictionary member name but none was given.");
        return Rml::DataVariable();
      }

      const ezVariantDictionary& d = value.Get<ezVariantDictionary>();

      const ezStringView sName = ezRmlUiConversionUtils::ToStringView(address.name);
      const ezVariant* pElement = nullptr;
      if (!d.TryGetValue(sName, pElement))
      {
        ezLog::Warning("Member '{}' not found in variant dictionary.", sName);
        return Rml::DataVariable();
      }

      return definitions.GetDefinition(*pElement);
    }

    ezLog::Warning("Tried to get the child of a scalar type.");
    return Rml::DataVariable();
  }

  //////////////////////////////////////////////////////////////////

  VariantDefinitionSet::VariantDefinitionSet()
    : m_Scalar(Rml::DataVariableType::Scalar, *this)
    , m_Array(Rml::DataVariableType::Array, *this)
    , m_Struct(Rml::DataVariableType::Struct, *this)
  {
  }

  Rml::DataVariable VariantDefinitionSet::GetDefinition(const ezVariant& value) const
  {
    const VariantVariableDefinition* pDefinition = &m_Scalar;

    switch (GetVariableType(value))
    {
      case Rml::DataVariableType::Array:
        pDefinition = &m_Array;
        break;
      case Rml::DataVariableType::Struct:
        pDefinition = &m_Struct;
        break;
      default:
        break;
    }

    // The RmlUi interface is non-const throughout, but neither the definitions nor the value are modified through it.
    return Rml::DataVariable(const_cast<VariantVariableDefinition*>(pDefinition), const_cast<ezVariant*>(&value));
  }

  //////////////////////////////////////////////////////////////////

  VariantVariableDefinition::VariantVariableDefinition(Rml::DataVariableType type, const VariantDefinitionSet& definitions)
    : VariableDefinition(type)
    , m_Definitions(definitions)
  {
  }

  bool VariantVariableDefinition::Get(void* pPtr, Rml::Variant& out_variant)
  {
    ezVariant* pValue = static_cast<ezVariant*>(pPtr);
    out_variant = ezRmlUiConversionUtils::ToVariant(*pValue);

    return true;
  }

  bool VariantVariableDefinition::Set(void* pPtr, const Rml::Variant& variant)
  {
    ezLog::Warning("Can't set the value of an element inside a variant array or dictionary. Nested values are read-only.");

    return false;
  }

  int VariantVariableDefinition::Size(void* pPtr)
  {
    ezVariant* pValue = static_cast<ezVariant*>(pPtr);

    if (pValue->IsA<ezVariantArray>())
    {
      return static_cast<int>(pValue->Get<ezVariantArray>().GetCount());
    }

    return 0;
  }

  Rml::DataVariable VariantVariableDefinition::Child(void* pPtr, const Rml::DataAddressEntry& address)
  {
    ezVariant* pValue = static_cast<ezVariant*>(pPtr);

    return GetChild(*pValue, Type(), address, m_Definitions);
  }

  Rml::StringList VariantVariableDefinition::ReflectMemberNames()
  {
    // RmlUi does not pass the instance pointer here, so the concrete dictionary and therefore its
    // keys are unknown at this point. Member access via 'obj.key' works regardless, only iterating
    // over the members of a nested dictionary with 'data-for' is unsupported.
    if (Type() == Rml::DataVariableType::Struct)
    {
      ezLog::Warning("Iterating over the members of a variant dictionary is not supported. Access the members by name instead.");
      return Rml::StringList();
    }

    return VariableDefinition::ReflectMemberNames();
  }

  ////////////////////////////////////////////////////////////////

  BlackboardVariableDefinition::BlackboardVariableDefinition(Rml::DataVariableType type, const VariantDefinitionSet& definitions)
    : VariableDefinition(type)
    , m_Definitions(definitions)
  {
  }

  bool BlackboardVariableDefinition::Get(void* pPtr, Rml::Variant& out_variant)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    out_variant = ezRmlUiConversionUtils::ToVariant(pInfo->m_CachedValue);

    return true;
  }

  bool BlackboardVariableDefinition::Set(void* pPtr, const Rml::Variant& variant)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    if (Type() != Rml::DataVariableType::Scalar)
    {
      ezLog::Warning("Can't set the value of a variant array or dictionary. Only scalar entries are writable.");
      return false;
    }

    ezVariant::Type::Enum targetType = ezVariant::Type::Invalid;
    if (auto pEntry = pInfo->m_pBlackboard->GetEntry(pInfo->m_sName))
    {
      targetType = pEntry->m_Value.GetType();
    }

    pInfo->m_CachedValue = ezRmlUiConversionUtils::ToVariant(variant, targetType);

    pInfo->m_pBlackboard->SetEntryValue(pInfo->m_sName, pInfo->m_CachedValue);    

    return true;
  }

  int BlackboardVariableDefinition::Size(void* pPtr)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    if (pInfo->m_CachedValue.IsA<ezVariantArray>())
    {
      return static_cast<int>(pInfo->m_CachedValue.Get<ezVariantArray>().GetCount());
    }

    return 0;
  }

  Rml::DataVariable BlackboardVariableDefinition::Child(void* pPtr, const Rml::DataAddressEntry& address)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    return GetChild(pInfo->m_CachedValue, Type(), address, m_Definitions);
  }

  Rml::StringList BlackboardVariableDefinition::ReflectMemberNames()
  {
    // See VariantVariableDefinition::ReflectMemberNames.
    if (Type() == Rml::DataVariableType::Struct)
    {
      ezLog::Warning("Iterating over the members of a variant dictionary is not supported. Access the members by name instead.");
      return Rml::StringList();
    }

    return VariableDefinition::ReflectMemberNames();
  }

  //////////////////////////////////////////////////////////////////

  BlackboardDataBinding::BlackboardDataBinding(const ezSharedPtr<ezBlackboard>& pBlackboard)
    : m_pBlackboard(pBlackboard)
    , m_ScalarDefinition(Rml::DataVariableType::Scalar, m_VariantDefinitions)
    , m_ArrayDefinition(Rml::DataVariableType::Array, m_VariantDefinitions)
    , m_StructDefinition(Rml::DataVariableType::Struct, m_VariantDefinitions)
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
      auto type = it.Value().m_Value.GetType();
      if ((type >= ezVariantType::Invalid && type <= ezVariantType::Double) ||
          type == ezVariantType::String || type == ezVariantType::HashedString ||
          type == ezVariantType::VariantArray || type == ezVariantType::VariantDictionary)
      {
        auto& info = m_EntryInfos.ExpandAndGetRef();
        info.m_pBlackboard = m_pBlackboard;
        info.m_sName = it.Key();
        info.m_uiChangeCounter = it.Value().m_uiChangeCounter;
        info.m_CachedValue = it.Value().m_Value;
        info.m_Type = GetVariableType(info.m_CachedValue);
      }
    }

    for (auto& info : m_EntryInfos)
    {
      BlackboardVariableDefinition* pDefinition = &m_ScalarDefinition;
      if (info.m_Type == Rml::DataVariableType::Array)
      {
        pDefinition = &m_ArrayDefinition;
      }
      else if (info.m_Type == Rml::DataVariableType::Struct)
      {
        pDefinition = &m_StructDefinition;
      }

      constructor.BindCustomDataVariable(ezRmlUiConversionUtils::ToString(info.m_sName), Rml::DataVariable(pDefinition, &info));
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
      for (auto& info : m_EntryInfos)
      {
        auto pEntry = m_pBlackboard->GetEntry(info.m_sName);

        if (pEntry != nullptr && info.m_uiChangeCounter != pEntry->m_uiChangeCounter)
        {
          // Refresh the cached value before marking the variable dirty, so that RmlUi reads the
          // new value and any pointers it takes into nested elements stay valid.
          info.m_CachedValue = pEntry->m_Value;

          const Rml::DataVariableType newType = GetVariableType(info.m_CachedValue);
          if (newType != info.m_Type)
          {
            ezLog::Warning("Blackboard entry '{}' changed its data variable type after setup. This is not supported, the binding will keep using the original type.", info.m_sName);
          }

          m_hDataModel.DirtyVariable(ezRmlUiConversionUtils::ToString(info.m_sName));
          info.m_uiChangeCounter = pEntry->m_uiChangeCounter;
          bUpdated = true;
        }
      }

      m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();
    }

    return bUpdated;
  }

} // namespace ezRmlUiInternal
