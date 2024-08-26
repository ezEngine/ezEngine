#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

void ezToolsSerializationUtils::SerializeTypes(const ezSet<const ezRTTI*>& types, ezAbstractObjectGraph& ref_typesGraph)
{
  ezRttiConverterContext context;
  ezRttiConverterWriter rttiConverter(&ref_typesGraph, &context, true, true);
  for (const ezRTTI* pType : types)
  {
    ezReflectedTypeDescriptor desc;
    if (pType->GetTypeFlags().IsSet(ezTypeFlags::Phantom))
    {
      ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pType, desc);
    }
    else
    {
      ezToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(pType, desc);
    }

    context.RegisterObject(ezUuid::MakeStableUuidFromString(pType->GetTypeName()), ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
    rttiConverter.AddObjectToGraph(ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
  }
}

void ezToolsSerializationUtils::CopyProperties(const ezDocumentObject* pSource, const ezDocumentObjectManager* pSourceManager, void* pTarget, const ezRTTI* pTargetType, FilterFunction propertFilter)
{
  ezAbstractObjectGraph graph;
  ezDocumentObjectConverterWriter writer(&graph, pSourceManager, [](const ezDocumentObject*, const ezAbstractProperty* p)
    { return p->GetAttributeByType<ezHiddenAttribute>() == nullptr; });
  ezAbstractObjectNode* pAbstractObj = writer.AddObjectToGraph(pSource);

  ezRttiConverterContext context;
  ezRttiConverterReader reader(&graph, &context);

  reader.ApplyPropertiesToObject(pAbstractObj, pTargetType, pTarget);
}
