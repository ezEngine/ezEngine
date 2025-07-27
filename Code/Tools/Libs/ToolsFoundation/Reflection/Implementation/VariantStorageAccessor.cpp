#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

ezVariantStorageAccessor::ezVariantStorageAccessor(ezStringView sProperty, ezVariant& value)
  : m_sProperty(sProperty)
  , m_Value(value)
{
}

ezVariantStorageAccessor::ezVariantStorageAccessor(ezStringView sProperty, const ezVariant& value)
  : m_sProperty(sProperty)
  , m_Value(const_cast<ezVariant&>(value))
{
}

ezVariant ezVariantStorageAccessor::GetValue(ezVariant index, ezStatus* pRes) const
{
  if (!index.IsValid())
    return m_Value;

  if (index.IsNumber())
  {
    if (!m_Value.IsA<ezVariantArray>())
    {
      if (pRes)
        *pRes = ezStatus(ezFmt("Index '{0}' for property '{1}' is invalid as the property is not an array.", index, m_sProperty));
      return ezVariant();
    }
    const ezVariantArray& values = m_Value.Get<ezVariantArray>();
    ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
    if (uiIndex < values.GetCount())
    {
      return values[uiIndex];
    }
  }
  else if (index.IsA<ezString>())
  {
    if (!m_Value.IsA<ezVariantDictionary>())
    {
      if (pRes)
        *pRes = ezStatus(ezFmt("Index '{0}' for property '{1}' is invalid as the property is not a dictionary.", index, m_sProperty));
      return ezVariant();
    }
    const ezVariantDictionary& values = m_Value.Get<ezVariantDictionary>();
    const ezString& sIndex = index.Get<ezString>();
    if (const ezVariant* pValue = values.GetValue(sIndex))
    {
      return *pValue;
    }
  }

  if (pRes)
    *pRes = ezStatus(ezFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, m_sProperty));
  return ezVariant();
}

ezStatus ezVariantStorageAccessor::SetValue(const ezVariant& value, ezVariant index)
{
  if (!index.IsValid())
  {
    m_Value = value;
    return EZ_SUCCESS;
  }

  if (index.IsNumber() && m_Value.IsA<ezVariantArray>())
  {
    ezVariantArray& values = m_Value.GetWritable<ezVariantArray>();
    ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
    if (uiIndex >= values.GetCount())
    {
      return ezStatus(ezFmt("Index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));
    }
    values[uiIndex] = value;
    return EZ_SUCCESS;
  }
  else if (index.IsA<ezString>() && m_Value.IsA<ezVariantDictionary>())
  {
    ezVariantDictionary& values = m_Value.GetWritable<ezVariantDictionary>();
    const ezString& sIndex = index.Get<ezString>();
    if (!values.Contains(sIndex))
    {
      return ezStatus(ezFmt("Index '{0}' for property '{1}' is out of bounds.", sIndex, m_sProperty));
    }
    values[sIndex] = value;
    return EZ_SUCCESS;
  }
  return ezStatus(ezFmt("Index '{0}' for property '{1}' is invalid.", index, m_sProperty));
}

ezInt32 ezVariantStorageAccessor::GetCount() const
{
  if (m_Value.IsA<ezVariantArray>())
    return m_Value.Get<ezVariantArray>().GetCount();
  else if (m_Value.IsA<ezVariantDictionary>())
    return m_Value.Get<ezVariantDictionary>().GetCount();
  return 0;
}

ezStatus ezVariantStorageAccessor::GetKeys(ezDynamicArray<ezVariant>& out_keys) const
{
  if (m_Value.IsA<ezVariantArray>())
  {
    const ezVariantArray& values = m_Value.Get<ezVariantArray>();
    out_keys.Reserve(values.GetCount());
    for (ezUInt32 i = 0; i < values.GetCount(); ++i)
    {
      out_keys.PushBack(i);
    }
    return EZ_SUCCESS;
  }
  else if (m_Value.IsA<ezVariantDictionary>())
  {
    const ezVariantDictionary& values = m_Value.Get<ezVariantDictionary>();
    out_keys.Reserve(values.GetCount());
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(ezVariant(it.Key()));
    }
    return EZ_SUCCESS;
  }
  return ezStatus(ezFmt("Property '{0}' is not a container.", m_sProperty));
}

ezStatus ezVariantStorageAccessor::InsertValue(const ezVariant& index, const ezVariant& value)
{
  if (index.IsNumber() && m_Value.IsA<ezVariantArray>())
  {
    ezVariantArray& values = m_Value.GetWritable<ezVariantArray>();
    ezInt32 iIndex = index.ConvertTo<ezInt32>();
    const ezInt32 iCount = (ezInt32)values.GetCount();
    if (iIndex == -1)
    {
      iIndex = iCount;
    }
    if (iIndex > iCount)
      return ezStatus(ezFmt("InsertValue: index '{0}' for property '{1}' is out of bounds.", iIndex, m_sProperty));

    values.InsertAt(iIndex, value);
    return EZ_SUCCESS;
  }
  else if (index.IsA<ezString>() && m_Value.IsA<ezVariantDictionary>())
  {
    ezVariantDictionary& values = m_Value.GetWritable<ezVariantDictionary>();
    const ezString& sIndex = index.Get<ezString>();
    if (values.Contains(index.Get<ezString>()))
      return ezStatus(ezFmt("InsertValue: index '{0}' for property '{1}' already exists.", sIndex, m_sProperty));

    values.Insert(sIndex, value);
    return EZ_SUCCESS;
  }
  return ezStatus(ezFmt("InsertValue: Property '{0}' is not a container or index {1} is invalid.", m_sProperty, index));
}

ezStatus ezVariantStorageAccessor::RemoveValue(const ezVariant& index)
{
  if (index.IsNumber() && m_Value.IsA<ezVariantArray>())
  {
    ezVariantArray& values = m_Value.GetWritable<ezVariantArray>();
    const ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
    if (uiIndex > values.GetCount())
      return ezStatus(ezFmt("RemoveValue: index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));

    values.RemoveAtAndCopy(uiIndex);
    return EZ_SUCCESS;
  }
  else if (index.IsA<ezString>() && m_Value.IsA<ezVariantDictionary>())
  {
    ezVariantDictionary& values = m_Value.GetWritable<ezVariantDictionary>();
    const ezString& sIndex = index.Get<ezString>();
    if (!values.Contains(index.Get<ezString>()))
      return ezStatus(ezFmt("RemoveValue: index '{0}' for property '{1}' does not exists.", sIndex, m_sProperty));

    values.Remove(sIndex);
    return EZ_SUCCESS;
  }
  return ezStatus(ezFmt("RemoveValue: Property '{0}' is not a container or index '{1}' is invalid.", m_sProperty, index));
}

ezStatus ezVariantStorageAccessor::MoveValue(const ezVariant& oldIndex, const ezVariant& newIndex)
{
  if (m_Value.IsA<ezVariantArray>() && oldIndex.IsNumber() && newIndex.IsNumber())
  {
    ezVariantArray& values = m_Value.GetWritable<ezVariantArray>();
    ezUInt32 uiOldIndex = oldIndex.ConvertTo<ezUInt32>();
    ezUInt32 uiNewIndex = newIndex.ConvertTo<ezUInt32>();
    if (uiOldIndex < values.GetCount() && uiNewIndex <= values.GetCount())
    {
      ezVariant value = values[uiOldIndex];
      values.RemoveAtAndCopy(uiOldIndex);
      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }
      values.InsertAt(uiNewIndex, value);
      return EZ_SUCCESS;
    }
    else
    {
      return ezStatus(ezFmt("MoveValue: index '{0}' or '{1}' for property '{2}' is out of bounds.", uiOldIndex, uiNewIndex, m_sProperty));
    }
  }
  else if (m_Value.IsA<ezVariantDictionary>() && oldIndex.IsA<ezString>() && newIndex.IsA<ezString>())
  {
    ezVariantDictionary& values = m_Value.GetWritable<ezVariantDictionary>();
    const ezString& sOldIndex = oldIndex.Get<ezString>();
    const ezString& sNewIndex = newIndex.Get<ezString>();

    if (!values.Contains(sOldIndex))
      return ezStatus(ezFmt("MoveValue: old index '{0}' for property '{2}' does not exist.", sOldIndex, m_sProperty));
    else if (values.Contains(sNewIndex))
      return ezStatus(ezFmt("MoveValue: new index '{0}' for property '{2}' already exists.", sNewIndex, m_sProperty));

    values.Insert(sNewIndex, values[sOldIndex]);
    values.Remove(sOldIndex);
    return EZ_SUCCESS;
  }
  return ezStatus(ezFmt("MoveValue: Property '{0}' is not a container or index '{1}' or '{2}' is invalid.", m_sProperty, oldIndex, newIndex));
}
