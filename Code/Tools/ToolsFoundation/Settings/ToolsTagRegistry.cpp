#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Configuration/Startup.h>
ezMap<ezString, ezToolsTag> ezToolsTagRegistry::m_NameToTags;

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ToolsTagRegistry)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Core"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezToolsTagRegistry::Startup();
}

ON_CORE_SHUTDOWN
{
  ezToolsTagRegistry::Shutdown();
}

EZ_END_SUBSYSTEM_DECLARATION

struct TagComparer
{
  EZ_FORCE_INLINE bool Less(const ezToolsTag* a, const ezToolsTag* b) const
  {
    if (a->m_sCategory != b->m_sCategory)
      return a->m_sCategory < b->m_sCategory;

    return a->m_sName < b->m_sName;;
  }
};
////////////////////////////////////////////////////////////////////////
// ezToolsTagRegistry public functions
////////////////////////////////////////////////////////////////////////

void ezToolsTagRegistry::Clear()
{
  m_NameToTags.Clear();
}

void ezToolsTagRegistry::WriteToJSON(ezStreamWriter& stream)
{
  ezExtendedJSONWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(ezJSONWriter::WhitespaceMode::All);
  writer.BeginObject();
  writer.BeginArray("Tags");

  for (auto it = m_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    writer.BeginObject();

    writer.AddVariableString("Name", it.Value().m_sName);
    writer.AddVariableString("Category", it.Value().m_sCategory);

    writer.EndObject();
  }

  writer.EndArray();
  writer.EndObject();
}

ezStatus ezToolsTagRegistry::ReadFromJSON(ezStreamReader& stream)
{
  ezExtendedJSONReader reader;
  if (reader.Parse(stream).Failed())
  {
    return ezStatus("Failed to read json data from ToolsTagRegistry stream!");
  }
  m_NameToTags.Clear();

  const ezVariantDictionary& dict = reader.GetTopLevelObject();

  ezVariant* pTags = nullptr;
  if (!dict.TryGetValue("Tags", pTags))
  {
    return ezStatus("Failed to find 'Tags' json object in json root object!");
  }

  if (!pTags->CanConvertTo<ezVariantArray>())
  {
    return ezStatus("Failed to cast 'Tags' json object into a json array!");
  }
  const ezVariantArray& tags = pTags->Get<ezVariantArray>();

  for (const ezVariant& value : tags)
  {
    if (!value.CanConvertTo<ezVariantDictionary>())
    {
      ezLog::Error("Tag is not a json object!");
      continue;
    }
    const ezVariantDictionary& tagDict = value.Get<ezVariantDictionary>();
    
    ezVariant* pName = nullptr;
    ezVariant* pCategory = nullptr;
    tagDict.TryGetValue("Name", pName);
    tagDict.TryGetValue("Category", pCategory);
    if (!pName || !pCategory || !pName->IsA<ezString>() || !pCategory->IsA<ezString>())
    {
      ezLog::Error("Incomplete tag declaration!");
      continue;
    }

    ezToolsTag tag;
    tag.m_sName = pName->Get<ezString>();
    tag.m_sCategory = pCategory->Get<ezString>();
    if (!ezToolsTagRegistry::AddTag(tag))
    {
      ezLog::Error("Failed to add tag '%s'", tag.m_sName.GetData());
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
    return false;
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

void ezToolsTagRegistry::GetAllTags(ezHybridArray<ezToolsTag*, 16>& out_tags)
{
  out_tags.Clear();
  for (auto it = m_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    out_tags.PushBack(&it.Value());
  }

  out_tags.Sort(TagComparer());
}

void ezToolsTagRegistry::GetTagsByCategory(const ezArrayPtr<ezStringView>& categories, ezHybridArray<ezToolsTag*, 16>& out_tags)
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


////////////////////////////////////////////////////////////////////////
// ezToolsTagRegistry private functions
////////////////////////////////////////////////////////////////////////

void ezToolsTagRegistry::Startup()
{
}

void ezToolsTagRegistry::Shutdown()
{
  m_NameToTags.Clear();
}
