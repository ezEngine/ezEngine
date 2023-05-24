#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Blackboard.h>
#include <Core/Utils/Blackboard.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Blackboard, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetOrCreateGlobal, In, "Name"),
    EZ_SCRIPT_FUNCTION_PROPERTY(FindGlobal, In, "Name"),

    EZ_SCRIPT_FUNCTION_PROPERTY(RegisterEntry, In, "Blackboard", In, "Name", In, "InitialValue", In, "Save", In, "OnChangeEvent"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetEntryValue, In, "Blackboard", In, "Name", In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Blackboard", In, "Name", In, "Fallback")->AddFlags(ezPropertyFlags::Const),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("Blackboard"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
ezBlackboard* ezScriptExtensionClass_Blackboard::GetOrCreateGlobal(ezStringView sName)
{
  ezHashedString sNameHashed;
  sNameHashed.Assign(sName);

  return ezBlackboard::GetOrCreateGlobal(sNameHashed).Borrow();
}

// static
ezBlackboard* ezScriptExtensionClass_Blackboard::FindGlobal(ezStringView sName)
{
  return ezBlackboard::FindGlobal(ezTempHashedString(sName));
}

// static
void ezScriptExtensionClass_Blackboard::RegisterEntry(ezBlackboard* pBoard, ezStringView sName, const ezVariant& initialValue, bool bSave, bool bOnChangeEvent)
{
  ezHashedString sNameHashed;
  sNameHashed.Assign(sName);

  ezBitflags<ezBlackboardEntryFlags> flags;
  flags.AddOrRemove(ezBlackboardEntryFlags::Save, bSave);
  flags.AddOrRemove(ezBlackboardEntryFlags::OnChangeEvent, bOnChangeEvent);

  pBoard->RegisterEntry(sNameHashed, initialValue, flags);
}

// static
bool ezScriptExtensionClass_Blackboard::SetEntryValue(ezBlackboard* pBoard, ezStringView sName, const ezVariant& value)
{
  return pBoard->SetEntryValue(ezTempHashedString(sName), value).Succeeded();
}

// static
ezVariant ezScriptExtensionClass_Blackboard::GetEntryValue(ezBlackboard* pBoard, ezStringView sName, const ezVariant& fallback)
{
  return pBoard->GetEntryValue(ezTempHashedString(sName), fallback);
}
