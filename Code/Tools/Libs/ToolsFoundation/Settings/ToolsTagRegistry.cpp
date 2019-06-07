#include <ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

struct TagComparer
{
  EZ_ALWAYS_INLINE bool Less(const ezToolsTag* a, const ezToolsTag* b) const
  {
    if (a->m_sCategory != b->m_sCategory)
      return a->m_sCategory < b->m_sCategory;

    return a->m_sName < b->m_sName;
    ;
  }
};
////////////////////////////////////////////////////////////////////////
// ezToolsTagRegistry public functions
////////////////////////////////////////////////////////////////////////

ezMap<ezString, ezToolsTag> ezToolsTagRegistry::m_NameToTags;

void ezToolsTagRegistry::Clear()
{
  for (auto it = m_NameToTags.GetIterator(); it.IsValid();)
  {
    if (!it.Value().m_bBuiltInTag)
    {
      it = m_NameToTags.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void ezToolsTagRegistry::WriteToDDL(ezStreamWriter& stream)
{
  ezOpenDdlWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);

  for (auto it = m_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    writer.BeginObject("Tag");

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::String, "Name");
    writer.WriteString(it.Value().m_sName);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::String, "Category");
    writer.WriteString(it.Value().m_sCategory);
    writer.EndPrimitiveList();

    writer.EndObject();
  }
}

ezStatus ezToolsTagRegistry::ReadFromDDL(ezStreamReader& stream)
{
  ezOpenDdlReader reader;
  if (reader.ParseDocument(stream).Failed())
  {
    return ezStatus("Failed to read data from ToolsTagRegistry stream!");
  }

  // Makes sure not to remove the built-in tags
  Clear();

  const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const ezOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const ezOpenDdlReaderElement* pName = pTags->FindChildOfType(ezOpenDdlPrimitiveType::String, "Name");
    const ezOpenDdlReaderElement* pCategory = pTags->FindChildOfType(ezOpenDdlPrimitiveType::String, "Category");

    if (!pName || !pCategory)
    {
      ezLog::Error("Incomplete tag declaration!");
      continue;
    }

    ezToolsTag tag;
    tag.m_sName = pName->GetPrimitivesString()[0];
    tag.m_sCategory = pCategory->GetPrimitivesString()[0];

    if (!ezToolsTagRegistry::AddTag(tag))
    {
      ezLog::Error("Failed to add tag '{0}'", tag.m_sName);
    }
  }

  return ezStatus(EZ_SUCCESS);
}

bool ezToolsTagRegistry::AddTag(const ezToolsTag& tag)
{
  if (tag.m_sName.IsEmpty())
    return false;

  auto it = m_NameToTags.Find(tag.m_sName);
  if (it.IsValid())
  {
    if (tag.m_bBuiltInTag)
    {
      // Make sure to pass this on, as it is not stored in the DDL file (because we don't want to rely on that)
      it.Value().m_bBuiltInTag = true;
    }

    return true;
  }
  else
  {
    m_NameToTags[tag.m_sName] = tag;
    return true;
  }
}

bool ezToolsTagRegistry::RemoveTag(const char* szName)
{
  auto it = m_NameToTags.Find(szName);
  if (it.IsValid())
  {
    m_NameToTags.Remove(it);
    return true;
  }
  else
  {
    return false;
  }
}

void ezToolsTagRegistry::GetAllTags(ezHybridArray<const ezToolsTag*, 16>& out_tags)
{
  out_tags.Clear();
  for (auto it = m_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    out_tags.PushBack(&it.Value());
  }

  out_tags.Sort(TagComparer());
}

void ezToolsTagRegistry::GetTagsByCategory(const ezArrayPtr<ezStringView>& categories, ezHybridArray<const ezToolsTag*, 16>& out_tags)
{
  out_tags.Clear();
  for (auto it = m_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    if (std::any_of(cbegin(categories), cend(categories), [&it](const ezStringView& cat) { return it.Value().m_sCategory == cat; }))
    {
      out_tags.PushBack(&it.Value());
    }
  }
  out_tags.Sort(TagComparer());
}
