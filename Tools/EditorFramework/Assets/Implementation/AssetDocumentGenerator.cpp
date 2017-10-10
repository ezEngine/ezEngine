#include <PCH.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentGenerator, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezAssetDocumentGenerator::ezAssetDocumentGenerator()
{

}

ezAssetDocumentGenerator::~ezAssetDocumentGenerator()
{

}

void ezAssetDocumentGenerator::ImportAssets(const ezHybridArray<ezString, 16>& filesToImport)
{
  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;

  for (ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezAssetDocumentGenerator>() || !pRtti->GetAllocator()->CanAllocate())
      continue;

    generators.PushBack(static_cast<ezAssetDocumentGenerator*>(pRtti->GetAllocator()->Allocate()));
  }

  for (const ezString& file : filesToImport)
  {
    ezHybridArray<ezAssetDocumentGenerator::Info, 16> importModes;

    for (ezAssetDocumentGenerator* pGen : generators)
    {
      pGen->GetImportModes(file, importModes);
    }

    importModes.Sort();

    if (!importModes.IsEmpty())
    {
      importModes[0].m_pGenerator->Generate(file, importModes[0]);
    }
  }

  for (ezAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }
  generators.Clear();
}
