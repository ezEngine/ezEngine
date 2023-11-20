#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/RttiConverter.h>

ezAiControllerDesc::ezAiControllerDesc() = default;
ezAiControllerDesc::~ezAiControllerDesc() = default;

void ezAiControllerDesc::AddSensor(ezUniquePtr<ezAiSensor>&& pSensor)
{
  EZ_ASSERT_DEV(!m_bFinalized, "ezAiControllerDesc can't be modified after Finalize()");

  m_Sensors.ExpandAndGetRef().m_pSensor = std::move(pSensor);
}

void ezAiControllerDesc::AddPerceptionGenerator(ezUniquePtr<ezAiPerceptionGenerator>&& pGenerator)
{
  EZ_ASSERT_DEV(!m_bFinalized, "ezAiControllerDesc can't be modified after Finalize()");

  m_PerceptionGenerators.ExpandAndGetRef().m_pGenerator = std::move(pGenerator);
}

void ezAiControllerDesc::AddBehavior(ezUniquePtr<ezAiBehavior>&& pBehavior)
{
  EZ_ASSERT_DEV(!m_bFinalized, "ezAiControllerDesc can't be modified after Finalize()");

  m_Behaviors.ExpandAndGetRef().m_pBehavior = std::move(pBehavior);
}

void ezAiControllerDesc::AddConsideration(ezUniquePtr<ezAiConsideration>&& pConsideration)
{
  EZ_ASSERT_DEV(!m_bFinalized, "ezAiControllerDesc can't be modified after Finalize()");

  m_Considerations.ExpandAndGetRef().m_pConsideration = std::move(pConsideration);
}

void ezAiControllerDesc::Finalize()
{
  EZ_ASSERT_DEV(!m_bFinalized, "ezAiControllerDesc::Finalize() was already called");

  for (auto& s : m_Sensors)
  {
    ValidateInputPins(s.m_pSensor.Borrow());
  }

  for (auto& g : m_PerceptionGenerators)
  {
    if (ezAiPerceptionGenerator::GetPerceptionType(g.m_pGenerator->GetDynamicRTTI()) == nullptr)
    {
      ezLog::Warning("{}: the type has no valid ezAiPerceptionTypeAttribute, behaviors can't connect to it.", g.m_pGenerator->GetDynamicRTTI()->GetTypeName());
    }

    ValidateInputPins(g.m_pGenerator.Borrow());
  }

  for (auto& b : m_Behaviors)
  {
    ValidateInputPins(b.m_pBehavior.Borrow());
  }

  for (auto& c : m_Considerations)
  {
    ValidateInputPins(c.m_pConsideration.Borrow());
  }

  for (auto& g : m_PerceptionGenerators)
  {
    g.m_pGenerator->OnFinalize(*this);
  }

  m_bFinalized = true;

  m_InstanceDataAllocator.ClearDescs();

  ezInstanceDataDesc desc;

  for (auto& s : m_Sensors)
  {
    if (s.m_pSensor->GetInstanceDataDesc(desc))
      s.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
  }

  for (auto& g : m_PerceptionGenerators)
  {
    if (g.m_pGenerator->GetInstanceDataDesc(desc))
      g.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
  }

  for (auto& b : m_Behaviors)
  {
    if (b.m_pBehavior->GetInstanceDataDesc(desc))
      b.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
  }

  for (auto& c : m_Considerations)
  {
    if (c.m_pConsideration->GetInstanceDataDesc(desc))
      c.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
  }
}

void ezAiControllerDesc::ValidateInputPins(ezReflectedClass* pNode) const
{
  const ezRTTI* pNodeRtti = pNode->GetDynamicRTTI();

  ezTempHybridArray<const ezAbstractProperty*, 16> properties;
  pNodeRtti->GetAllProperties(properties);

  for (const ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom<ezAiInputPin>())
      continue;

    ezAiInputPin* pPin = static_cast<ezAiInputPin*>(static_cast<const ezAbstractMemberProperty*>(pProp)->GetPropertyPointer(pNode));
    if (pPin == nullptr || pPin->m_Connections.IsEmpty())
      continue;

    const ezAiInputPinAttribute* pAttr = pProp->GetAttributeByType<ezAiInputPinAttribute>();
    const ezRTTI* pAccepted = pAttr ? pAttr->GetNodeType() : nullptr;

    // the accepted type selects the array that the indices refer to
    ezTempHybridArray<const ezReflectedClass*, 16> nodes;
    if (pAccepted != nullptr && pAccepted->IsDerivedFrom<ezAiSensor>())
    {
      for (const auto& s : m_Sensors)
        nodes.PushBack(s.m_pSensor.Borrow());
    }
    else if (pAccepted != nullptr && (pAccepted->IsDerivedFrom<ezAiPerceptionGenerator>() || pAccepted->IsDerivedFrom<ezAiPerception>()))
    {
      for (const auto& g : m_PerceptionGenerators)
        nodes.PushBack(g.m_pGenerator.Borrow());
    }
    else if (pAccepted != nullptr && pAccepted->IsDerivedFrom<ezAiConsideration>())
    {
      for (const auto& c : m_Considerations)
        nodes.PushBack(c.m_pConsideration.Borrow());
    }
    else
    {
      ezLog::Warning("{}::{}: the input pin has no valid ezAiInputPinAttribute, its connections are ignored.", pNodeRtti->GetTypeName(), pProp->GetPropertyName());
      pPin->m_Connections.Clear();
      continue;
    }

    for (ezUInt32 i = pPin->m_Connections.GetCount(); i > 0; --i)
    {
      const ezUInt32 uiIndex = pPin->m_Connections[i - 1];
      if (uiIndex >= nodes.GetCount() || !pAttr->Accepts(nodes[uiIndex]->GetDynamicRTTI()))
      {
        ezLog::Warning("{}::{}: connection {} is invalid or not of type '{}' and is ignored.", pNodeRtti->GetTypeName(), pProp->GetPropertyName(), uiIndex, pAccepted->GetTypeName());
        pPin->m_Connections.RemoveAtAndCopy(i - 1);
      }
    }
  }
}

ezBlob ezAiControllerDesc::AllocateInstanceData() const
{
  EZ_ASSERT_DEV(m_bFinalized, "ezAiControllerDesc::Finalize() has to be called first");
  return m_InstanceDataAllocator.AllocateAndConstruct();
}

void ezAiControllerDesc::DeallocateInstanceData(ezBlob& ref_blob) const
{
  m_InstanceDataAllocator.DestructAndDeallocate(ref_blob);
}

namespace
{
  constexpr ezUInt8 s_uiControllerDescVersion = 1;
  constexpr ezStringView s_PropNames[] = {"Sensors"_ezsv, "PerceptionGenerators"_ezsv, "Behaviors"_ezsv, "Considerations"_ezsv};

  /// Calls func(ezUniquePtr<T>&&) for every valid object that is listed in the given property of the root node.
  template <typename T, typename FUNC>
  void ReadObjects(const ezAbstractObjectGraph& graph, ezRttiConverterReader& ref_reader, const ezAbstractObjectNode& root, ezStringView sPropName, FUNC func)
  {
    const ezAbstractObjectNode::Property* pProp = root.FindProperty(sPropName);
    if (pProp == nullptr || !pProp->m_Value.IsA<ezVariantArray>())
      return;

    for (const ezVariant& guid : pProp->m_Value.Get<ezVariantArray>())
    {
      const ezAbstractObjectNode* pNode = guid.IsA<ezUuid>() ? graph.GetNode(guid.Get<ezUuid>()) : nullptr;
      if (pNode == nullptr)
      {
        ezLog::Warning("AI controller desc: '{}' references a missing object", sPropName);
        continue;
      }

      const ezRTTI* pRtti = ezRTTI::FindTypeByName(pNode->GetType());
      if (pRtti == nullptr || !pRtti->IsDerivedFrom<T>())
      {
        ezLog::Warning("AI controller desc: type '{}' is unknown or not derived from '{}'", pNode->GetType(), ezGetStaticRTTI<T>()->GetTypeName());
        continue;
      }

      ezInternal::NewInstance<void> pObject = ref_reader.CreateObjectFromNode(pNode);
      if (pObject.m_pInstance == nullptr)
      {
        ezLog::Warning("AI controller desc: type '{}' can't be instantiated", pNode->GetType());
        continue;
      }

      func(ezUniquePtr<T>(pObject.Cast<T>()));
    }
  }
} // namespace

ezResult ezAiControllerDesc::Serialize(ezStreamWriter& inout_stream) const
{
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter writer(&graph, &context, false, true);

  ezAbstractObjectNode* pRoot = graph.AddNode(ezUuid::MakeUuid(), "ezAiControllerDesc", 1, "root");

  auto AddObjects = [&](ezStringView sPropName, auto objects, auto getObject)
  {
    ezVariantArray guids;
    for (const auto& obj : objects)
    {
      const ezReflectedClass* pObject = getObject(obj);
      const ezUuid guid = ezUuid::MakeUuid();
      context.RegisterObject(guid, pObject->GetDynamicRTTI(), const_cast<ezReflectedClass*>(pObject));
      writer.AddObjectToGraph(pObject->GetDynamicRTTI(), pObject);
      guids.PushBack(guid);
    }
    pRoot->AddProperty(sPropName, guids);
  };

  AddObjects(s_PropNames[0], GetSensors(), [](const Sensor& s)
    { return s.m_pSensor.Borrow(); });
  AddObjects(s_PropNames[1], GetPerceptionGenerators(), [](const PerceptionGenerator& g)
    { return g.m_pGenerator.Borrow(); });
  AddObjects(s_PropNames[2], GetBehaviors(), [](const Behavior& b)
    { return b.m_pBehavior.Borrow(); });
  AddObjects(s_PropNames[3], GetConsiderations(), [](const Consideration& c)
    { return c.m_pConsideration.Borrow(); });

  inout_stream << s_uiControllerDescVersion;
  ezAbstractGraphBinarySerializer::Write(inout_stream, &graph);
  return EZ_SUCCESS;
}

ezResult ezAiControllerDesc::Deserialize(ezStreamReader& inout_stream)
{
  EZ_ASSERT_DEV(!m_bFinalized && m_Sensors.IsEmpty() && m_PerceptionGenerators.IsEmpty() && m_Behaviors.IsEmpty() && m_Considerations.IsEmpty(), "ezAiControllerDesc::Deserialize() has to be called on an empty desc");

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  if (uiVersion != s_uiControllerDescVersion)
  {
    ezLog::Error("Unsupported AI controller desc version {}", uiVersion);
    return EZ_FAILURE;
  }

  ezAbstractObjectGraph graph;
  ezAbstractGraphBinarySerializer::Read(inout_stream, &graph);

  const ezAbstractObjectNode* pRoot = graph.GetNodeByName("root");
  if (pRoot == nullptr)
  {
    ezLog::Error("AI controller desc has no root node");
    return EZ_FAILURE;
  }

  ezRttiConverterContext context;
  ezRttiConverterReader reader(&graph, &context);

  ReadObjects<ezAiSensor>(graph, reader, *pRoot, s_PropNames[0], [&](ezUniquePtr<ezAiSensor>&& pSensor)
    { AddSensor(std::move(pSensor)); });

  ReadObjects<ezAiPerceptionGenerator>(graph, reader, *pRoot, s_PropNames[1], [&](ezUniquePtr<ezAiPerceptionGenerator>&& pGenerator)
    { AddPerceptionGenerator(std::move(pGenerator)); });

  ReadObjects<ezAiBehavior>(graph, reader, *pRoot, s_PropNames[2], [&](ezUniquePtr<ezAiBehavior>&& pBehavior)
    { AddBehavior(std::move(pBehavior)); });

  ReadObjects<ezAiConsideration>(graph, reader, *pRoot, s_PropNames[3], [&](ezUniquePtr<ezAiConsideration>&& pConsideration)
    { AddConsideration(std::move(pConsideration)); });

  return EZ_SUCCESS;
}
