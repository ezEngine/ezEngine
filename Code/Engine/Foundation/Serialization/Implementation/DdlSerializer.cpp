#include <Foundation/PCH.h>
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

  ezStringBuilder tmp;

  const auto& Nodes = pGraph->GetAllNodes();
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    writer.BeginObject("o");
    {
      const auto& node = *itNode.Value();

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
          /// \todo Remove this once spaces are not allowed in property names anymore
          tmp = it.Key();
          tmp.ReplaceAll(" ", "_");
          ezOpenDdlUtils::StoreVariant(writer, *it.Value(), tmp);
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
    if (pTypeVersion && pTypeVersion->GetPrimitivesType() == ezOpenDdlPrimitiveType::UInt32 && pTypeVersion->GetNumPrimitives() == 1)
    {
      const ezUInt32* pValues = pTypeVersion->GetPrimitivesUInt32();
      uiTypeVersion = pValues[0];
    }

    auto* pNode = pGraph->AddNode(guid, tmp, uiTypeVersion, tmp2);

    for (const ezOpenDdlReaderElement* pProp = pProps->GetFirstChild(); pProp != nullptr; pProp = pProp->GetSibling())
    {
      if (!pProp->HasName())
        continue;

      /// \todo Remove this bla bla
      tmp = pProp->GetName();
      tmp.ReplaceAll("_", " ");

      if (ezOpenDdlUtils::ConvertToVariant(pProp, varTmp).Failed())
        continue;

      pNode->AddProperty(tmp, varTmp);
    }
  }
}

ezResult ezAbstractGraphDdlSerializer::Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  ezOpenDdlReader reader;
  if (reader.ParseDocument(stream, 0, ezGlobalLog::GetOrCreateInstance()).Failed())
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

  const ezOpenDdlReaderElement* pTypes = reader.GetRootElement()->FindChildOfType("Types");
  if (pTypesGraph != nullptr && pTypes != nullptr)
  {
    ReadGraph(pTypesGraph, pTypes);
  }

  if (bApplyPatches)
    ezGraphVersioning::GetSingleton()->PatchGraph(pGraph);
  return EZ_SUCCESS;
}
