#include <PCH.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlReader.h>


static void WriteGraph(ezOpenDdlWriter &writer, const ezAbstractObjectGraph* pGraph, const char* szName)
{
  ezMap<const char*, const ezVariant*, CompareConstChar> SortedProperties;

  writer.BeginObject(szName);

  const auto& Nodes = pGraph->GetAllNodes();
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();

    // the object type is mostly ignored, but we need to tag the Header specifically, so that we can easily skip everything else when we only need this
    if (ezStringUtils::IsEqual(node.GetType(), "ezAssetDocumentInfo"))
      writer.BeginObject("AssetInfo");
    else
      writer.BeginObject("o");

    {

      ezOpenDdlUtils::StoreUuid(writer, node.GetGuid() ,"id");
      ezOpenDdlUtils::StoreString(writer, node.GetType(), "t");
      ezOpenDdlUtils::StoreUInt32(writer, node.GetTypeVersion(), "v");

      if (!ezStringUtils::IsNullOrEmpty(node.GetNodeName()))
        ezOpenDdlUtils::StoreString(writer, node.GetNodeName(), "n");

      writer.BeginObject("p");
      {
        for (const auto& prop : node.GetProperties())
          SortedProperties[prop.m_szPropertyName] = &prop.m_Value;

        for (auto it = SortedProperties.GetIterator(); it.IsValid(); ++it)
        {
          ezOpenDdlUtils::StoreVariant(writer, *it.Value(), it.Key());
        }

        SortedProperties.Clear();
      }
      writer.EndObject();
    }
    writer.EndObject();
  }

  writer.EndObject();
}

void ezAbstractGraphDdlSerializer::Write(ezStreamWriter& stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph, bool bCompactMmode, ezOpenDdlWriter::TypeStringMode typeMode)
{
  ezOpenDdlWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetCompactMode(bCompactMmode);
  writer.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Exact);
  writer.SetPrimitiveTypeStringMode(typeMode);

  if (typeMode != ezOpenDdlWriter::TypeStringMode::Compliant)
    writer.SetIndentation(-1);

  WriteGraph(writer, pGraph, "Objects");
  if (pTypesGraph)
  {
    WriteGraph(writer, pTypesGraph, "Types");
  }
}


static void ReadGraph(ezAbstractObjectGraph* pGraph, const ezOpenDdlReaderElement* pRoot)
{
  ezStringBuilder tmp, tmp2;
  ezVariant varTmp;

  for (const ezOpenDdlReaderElement* pObject = pRoot->GetFirstChild(); pObject != nullptr; pObject = pObject->GetSibling())
  {
    const ezOpenDdlReaderElement* pGuid = pObject->FindChildOfType(ezOpenDdlPrimitiveType::Custom, "id");
    const ezOpenDdlReaderElement* pType = pObject->FindChildOfType(ezOpenDdlPrimitiveType::String, "t");
    const ezOpenDdlReaderElement* pTypeVersion = pObject->FindChildOfType(ezOpenDdlPrimitiveType::UInt32, "v");
    const ezOpenDdlReaderElement* pName = pObject->FindChildOfType(ezOpenDdlPrimitiveType::String, "n");
    const ezOpenDdlReaderElement* pProps = pObject->FindChildOfType("p");

    if (pGuid == nullptr || pType == nullptr || pProps == nullptr)
    {
      EZ_REPORT_FAILURE("Object contains invalid elements");
      continue;
    }

    ezUuid guid;
    if (ezOpenDdlUtils::ConvertToUuid(pGuid, guid).Failed())
    {
      EZ_REPORT_FAILURE("Object has an invalid guid");
      continue;
    }

    tmp = pType->GetPrimitivesString()[0];

    if (pName)
      tmp2 = pName->GetPrimitivesString()[0];
    else
      tmp2.Clear();

    ezUInt32 uiTypeVersion = 0;
    if (pTypeVersion)
    {
      uiTypeVersion = pTypeVersion->GetPrimitivesUInt32()[0];
    }

    auto* pNode = pGraph->AddNode(guid, tmp, uiTypeVersion, tmp2);

    for (const ezOpenDdlReaderElement* pProp = pProps->GetFirstChild(); pProp != nullptr; pProp = pProp->GetSibling())
    {
      if (!pProp->HasName())
        continue;

      if (ezOpenDdlUtils::ConvertToVariant(pProp, varTmp).Failed())
        continue;

      pNode->AddProperty(pProp->GetName(), varTmp);
    }
  }
}

ezResult ezAbstractGraphDdlSerializer::Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  ezOpenDdlReader reader;
  if (reader.ParseDocument(stream, 0, ezLog::GetThreadLocalLogSystem()).Failed())
  {
    ezLog::Error("Failed to parse DDL graph");
    return EZ_FAILURE;
  }

  const ezOpenDdlReaderElement* pObjects = reader.GetRootElement()->FindChildOfType("Objects");
  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    ezLog::Error("DDL graph does not contain an 'Objects' root object");
    return EZ_FAILURE;
  }

  ezAbstractObjectGraph* pTempTypesGraph = pTypesGraph;
  if (pTempTypesGraph == nullptr)
  {
    pTempTypesGraph = EZ_DEFAULT_NEW(ezAbstractObjectGraph);
  }
  const ezOpenDdlReaderElement* pTypes = reader.GetRootElement()->FindChildOfType("Types");
  if (pTypes != nullptr)
  {
    ReadGraph(pTempTypesGraph, pTypes);
  }

  if (bApplyPatches)
  {
    if (pTempTypesGraph)
      ezGraphVersioning::GetSingleton()->PatchGraph(pTempTypesGraph);
    ezGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTempTypesGraph);
  }

  if (pTypesGraph == nullptr)
    EZ_DEFAULT_DELETE(pTempTypesGraph);

  return EZ_SUCCESS;
}

// This is a handcrafted DDL reader that ignores everything that is not an 'AssetInfo' object
// The purpose is to speed up reading asset information by skipping everything else
//
// The reader 'knows' the file format details and uses them.
// Top-level (ie. depth 0) there is an "Objects" object -> we need to enter that
// Inside that (depth 1) there is the "AssetInfo" object -> need to enter that as well
// All objects inside that must be stored
// Once the AssetInfo object is left everything else can be skipped
class HeaderReader : public ezOpenDdlReader
{
public:
  HeaderReader()
  {
    m_iDepth = 0;
  }

protected:
  ezInt32 m_iDepth;

  virtual void OnBeginObject(const char* szType, const char* szName, bool bGlobalName) override
  {
    // not yet entered the "Objects" group
    if (m_iDepth == 0 && ezStringUtils::IsEqual(szType, "Objects"))
    {
      ++m_iDepth;

      ezOpenDdlReader::OnBeginObject(szType, szName, bGlobalName);
      return;
    }

    // not yet entered the "AssetInfo" group, but inside "Objects"
    if (m_iDepth == 1 && ezStringUtils::IsEqual(szType, "AssetInfo"))
    {
      ++m_iDepth;

      ezOpenDdlReader::OnBeginObject(szType, szName, bGlobalName);
      return;
    }

    // inside "AssetInfo"
    if (m_iDepth > 1)
    {
      ++m_iDepth;
      ezOpenDdlReader::OnBeginObject(szType, szName, bGlobalName);
      return;
    }

    // ignore everything else
    SkipRestOfObject();
  }


  virtual void OnEndObject() override
  {
    --m_iDepth;

    if (m_iDepth <= 1)
    {
      // we were inside "AssetInfo" or "Objects" and returned from it, so now skip the rest
      m_iDepth = -1;
      StopParsing();
    }

    ezOpenDdlReader::OnEndObject();
  }

};

ezResult ezAbstractGraphDdlSerializer::ReadHeader(ezStreamReader& stream, ezAbstractObjectGraph* pGraph)
{
  HeaderReader reader;
  if (reader.ParseDocument(stream, 0, ezLog::GetThreadLocalLogSystem()).Failed())
  {
    EZ_REPORT_FAILURE("Failed to parse DDL graph");
    return EZ_FAILURE;
  }

  const ezOpenDdlReaderElement* pObjects = reader.GetRootElement()->FindChildOfType("Objects");
  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    EZ_REPORT_FAILURE("DDL graph does not contain an 'Objects' root object");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_DdlSerializer);

