#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

ezString ToBinary(const ezUuid& guid)
{
	ezStringBuilder s, sResult;

	ezUInt8* pBytes = (ezUInt8*)&guid;

	for (ezUInt32 i = 0; i < sizeof(ezUuid); ++i)
	{
		s.Format("%02X", (ezUInt32)*pBytes);
		++pBytes;

		sResult.Append(s);
	}

	return sResult;
}


void ezSceneDocument::UpdatePrefabs()
{
  EZ_LOCK(m_ObjectMetaData.GetMutex());

  // make sure the prefabs are updated
  m_CachedPrefabGraphs.Clear();

  GetCommandHistory()->StartTransaction();

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();
}

void ezSceneDocument::UpdatePrefabsRecursive(ezDocumentObject* pObject)
{
  auto ChildArray = pObject->GetChildren();

  ezStringBuilder sPrefabBase;

  for (auto pChild : ChildArray)
  {
    auto pMeta = m_ObjectMetaData.BeginReadMetaData(pChild->GetGuid());
    const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;
    const ezUuid PrefabSeed = pMeta->m_PrefabSeedGuid;
	sPrefabBase = pMeta->m_sBasePrefab;
	
    m_ObjectMetaData.EndReadMetaData();

    // if this is a prefab instance, update it
    if (PrefabAsset.IsValid())
    {
      UpdatePrefabObject(pChild, PrefabAsset, PrefabSeed, sPrefabBase);
    }
    else
    {
      // only recurse (currently), if no prefab was found
      // that means nested prefabs are currently not possible, that needs to be handled a bit differently later
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void ezSceneDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab)
{
  ezHybridArray<ezUuid, 16> NewObjects;

  ezInstantiatePrefabCommand inst;
  inst.m_bAllowPickedPosition = false;
  inst.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid = PrefabSeed;
  inst.m_sJsonGraph = GetCachedPrefabGraph(PrefabAsset);

  NewObjects.Clear();
  void* pArray = &NewObjects;
  memcpy(&inst.m_pCreatedRootObjects, &pArray, sizeof(void*)); /// \todo HACK-o-rama

  // prepare the original prefab as a graph
  ezAbstractObjectGraph graphBasePrefab;
  {
	  ezMemoryStreamStorage storage;
	  ezMemoryStreamWriter stringWriter(&storage);
	  ezMemoryStreamReader stringReader(&storage);
	  stringWriter.WriteBytes(szBasePrefab, ezStringUtils::GetStringElementCount(szBasePrefab));

	  ezAbstractGraphJsonSerializer::Read(stringReader, &graphBasePrefab);
  }

  {
	ezAbstractObjectGraph graphNewPrefab, graphHeader;

	// read the new template as a graph
	{
		ezMemoryStreamStorage storage;
		ezMemoryStreamWriter stringWriter(&storage);
		ezMemoryStreamReader stringReader(&storage);
		stringWriter.WriteBytes(inst.m_sJsonGraph.GetData(), inst.m_sJsonGraph.GetElementCount());
		ezAbstractGraphJsonSerializer::Read(stringReader, &graphNewPrefab);
	}

	// copy the header, to be able to insert it into the result later
    auto pHeader = graphNewPrefab.GetNodeByName("Header");
	{
		EZ_ASSERT_DEBUG(pHeader, "header is missing");
		graphHeader.CopyNodeIntoGraph(pHeader);
		graphNewPrefab.RemoveNode(pHeader->GetGuid());
	}

	// prepare the current state as a graph
	ezAbstractObjectGraph graphCurrentInstance;
	{
		ezDocumentObjectConverterWriter writer(&graphCurrentInstance, GetObjectManager(), true, true);
		writer.AddObjectToGraph(pObject);
		graphCurrentInstance.ReMapNodeGuids(PrefabSeed, true);
		// just take the entire ObjectTree node as is
		graphCurrentInstance.CopyNodeIntoGraph(graphNewPrefab.GetNodeByName("ObjectTree"));
	}


	// debug output
	{
		{
			ezFileWriter file;
			file.Open("D:\\Prefab - base.txt");
			file.WriteBytes(szBasePrefab, ezStringUtils::GetStringElementCount(szBasePrefab));
		}

		{
			ezFileWriter file;
			file.Open("D:\\Prefab - org.txt");
			ezAbstractGraphJsonSerializer::Write(file, &graphCurrentInstance, ezJSONWriter::WhitespaceMode::LessIndentation);
		}

		{
			ezFileWriter file2;
			file2.Open("D:\\Prefab - template.txt");
			ezAbstractGraphJsonSerializer::Write(file2, &graphNewPrefab, ezJSONWriter::WhitespaceMode::LessIndentation);
		}
	}

    ezDeque<ezAbstractGraphDiffOperation> DiffToBase;
    graphCurrentInstance.CreateDiffWithBaseGraph(graphBasePrefab, DiffToBase);

	// debug output
	{
		ezFileWriter file3;
		file3.Open("D:\\Prefab - diff.txt");

		ezStringBuilder sDiff;
		for (const auto& diff : DiffToBase)
		{
			ezStringBuilder Data = ToBinary(diff.m_Node);

			switch (diff.m_Operation)
			{
			case ezAbstractGraphDiffOperation::Op::NodeAdd:
				{
					sDiff.AppendFormat("<add> - {%s} (%s)\n", Data.GetData(), diff.m_sProperty.GetData());
				}
				break;

			case ezAbstractGraphDiffOperation::Op::NodeDelete:
				{
					sDiff.AppendFormat("<del> - {%s}\n", Data.GetData());
				}
				break;

			case ezAbstractGraphDiffOperation::Op::PropertySet:
				if (diff.m_Value.CanConvertTo<ezString>())
					sDiff.AppendFormat("<set> - {%s} - \"%s\" = %s\n", Data.GetData(), diff.m_sProperty.GetData(), diff.m_Value.ConvertTo<ezString>().GetData());
				else
					sDiff.AppendFormat("<set> - {%s} - \"%s\" = xxx\n", Data.GetData(), diff.m_sProperty.GetData());
				break;

			}
		}

		file3.WriteBytes(sDiff.GetData(), sDiff.GetElementCount());
	}

	{
		graphNewPrefab.CopyNodeIntoGraph(graphHeader.GetNodeByName("Header"));
		graphNewPrefab.ApplyDiff(DiffToBase);

		ezMemoryStreamStorage stor;
		ezMemoryStreamWriter sw(&stor);

		ezAbstractGraphJsonSerializer::Write(sw, &graphNewPrefab, ezJSONWriter::WhitespaceMode::LessIndentation);

		ezStringBuilder sNewGraph;
		sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize());

		inst.m_sJsonGraph = sNewGraph;
	}

	// debug output
	{
      ezFileWriter file4;
      file4.Open("D:\\Prefab - result.txt");
      file4.WriteBytes(inst.m_sJsonGraph.GetData(), inst.m_sJsonGraph.GetElementCount());
    }
  }

  ezRemoveObjectCommand rm;
  rm.m_Object = pObject->GetGuid();

  // remove current object
  GetCommandHistory()->AddCommand(rm);

  // instantiate prefab again
  GetCommandHistory()->AddCommand(inst);

  // pass the prefab meta data to the new instance
  for (const auto& guid : NewObjects)
  {
    auto pMeta = m_ObjectMetaData.BeginModifyMetaData(guid);
    pMeta->m_CreateFromPrefab = PrefabAsset;
    pMeta->m_PrefabSeedGuid = PrefabSeed;
	pMeta->m_sBasePrefab = GetCachedPrefabGraph(PrefabAsset);

    m_ObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::PrefabFlag);
  }
}