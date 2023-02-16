#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

ezMap<ezString, ezDynamicStringEnum> ezDynamicStringEnum::s_DynamicEnums;
ezDelegate<void(const char* szEnumName, ezDynamicStringEnum& e)> ezDynamicStringEnum::s_RequestUnknownCallback;

void ezDynamicStringEnum::RemoveEnum(const char* szEnumName)
{
  s_DynamicEnums.Remove(szEnumName);
}

void ezDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void ezDynamicStringEnum::AddValidValue(const char* szNewName, bool bSortValues /*= false*/)
{
  ezString sName = szNewName;

  if (!m_ValidValues.Contains(sName))
    m_ValidValues.PushBack(sName);

  if (bSortValues)
    SortValues();
}

void ezDynamicStringEnum::RemoveValue(const char* szValue)
{
  m_ValidValues.RemoveAndCopy(szValue);
}

bool ezDynamicStringEnum::IsValueValid(const char* szValue) const
{
  return m_ValidValues.Contains(szValue);
}

void ezDynamicStringEnum::SortValues()
{
  m_ValidValues.Sort();
}

ezDynamicStringEnum& ezDynamicStringEnum::GetDynamicEnum(const char* szEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(szEnumName, &bExisted);

  if (!bExisted && s_RequestUnknownCallback.IsValid())
  {
    s_RequestUnknownCallback(szEnumName, it.Value());
  }

  return it.Value();
}

ezDynamicStringEnum& ezDynamicStringEnum::CreateDynamicEnum(const char* szEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(szEnumName, &bExisted);

  ezDynamicStringEnum& e = it.Value();
  e.Clear();
  e.SetStorageFile(nullptr);

  return e;
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
