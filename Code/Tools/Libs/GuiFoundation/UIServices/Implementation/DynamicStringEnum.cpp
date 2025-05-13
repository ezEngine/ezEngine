#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

ezMap<ezString, ezDynamicStringEnum> ezDynamicStringEnum::s_DynamicEnums;
ezDelegate<void(ezStringView sEnumName, ezDynamicStringEnum& e)> ezDynamicStringEnum::s_RequestUnknownCallback;

// static
ezDynamicStringEnum& ezDynamicStringEnum::GetDynamicEnum(ezStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  if (!bExisted && s_RequestUnknownCallback.IsValid())
  {
    s_RequestUnknownCallback(sEnumName, it.Value());
  }

  return it.Value();
}

// static
ezDynamicStringEnum& ezDynamicStringEnum::CreateDynamicEnum(ezStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  ezDynamicStringEnum& e = it.Value();
  e.Clear();
  e.SetStorageFile(nullptr);

  return e;
}

// static
void ezDynamicStringEnum::RemoveEnum(ezStringView sEnumName)
{
  s_DynamicEnums.Remove(sEnumName);
}

void ezDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void ezDynamicStringEnum::AddValidValue(ezStringView sValue, bool bSortValues /*= false*/)
{
  ezString sNewValue = sValue;

  if (!m_ValidValues.Contains(sNewValue))
    m_ValidValues.PushBack(sNewValue);

  if (bSortValues)
    SortValues();
}

void ezDynamicStringEnum::RemoveValue(ezStringView sValue)
{
  m_ValidValues.RemoveAndCopy(sValue);
}

bool ezDynamicStringEnum::IsValueValid(ezStringView sValue) const
{
  return m_ValidValues.Contains(sValue);
}

void ezDynamicStringEnum::SortValues()
{
  m_ValidValues.Sort();
}

void ezDynamicStringEnum::SetEditCommand(ezStringView sCmd, const ezVariant& value)
{
  m_sEditCommand = sCmd;
  m_EditCommandValue = value;
}

void ezDynamicStringEnum::ReadFromStorage()
{
  Clear();

  ezStringBuilder sFile, tmp;

  ezFileReader file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  sFile.ReadAll(file);

  ezHybridArray<ezStringView, 32> values;

  sFile.Split(false, values, "\n", "\r");

  for (auto val : values)
  {
    AddValidValue(val.GetData(tmp));
  }
}

void ezDynamicStringEnum::SaveToStorage()
{
  if (m_sStorageFile.IsEmpty())
    return;

  ezFileWriter file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  ezStringBuilder tmp;

  for (const auto& val : m_ValidValues)
  {
    tmp.Set(val, "\n");
    file.WriteBytes(tmp.GetData(), tmp.GetElementCount()).IgnoreResult();
  }
}
