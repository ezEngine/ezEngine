#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/DebugAnimNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogAnimNode, 1, ezRTTINoAllocator)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new ezDefaultValueAttribute("Values: {0}/{1}-{3}/{4}")),

      EZ_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("NumberCount", m_uiNumberCount)->AddAttributes(new ezNoTemporaryTransactionsAttribute(), new ezDynamicPinAttribute(), new ezDefaultValueAttribute(1)),
      EZ_ARRAY_MEMBER_PROPERTY("InNumbers", m_InNumbers)->AddAttributes(new ezHiddenAttribute(), new ezDynamicPinAttribute("NumberCount")),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Debug"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Pink)),
      new ezTitleAttribute("Log: '{Text}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLogAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sText;
  stream << m_uiNumberCount;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_InNumbers));

  return EZ_SUCCESS;
}

ezResult ezLogAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;
  stream >> m_uiNumberCount;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_InNumbers));

  return EZ_SUCCESS;
}

static ezStringView BuildFormattedText(ezStringView sText, const ezVariantArray& params, ezStringBuilder& ref_sStorage)
{
  ezHybridArray<ezString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<ezString>());
  }

  ezHybridArray<ezStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  ezFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogInfoAnimNode, 1, ezRTTIDefaultAllocator<ezLogInfoAnimNode>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Log Info: '{Text}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLogInfoAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  ezVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  ezStringBuilder sStorage;
  ezLog::Info(BuildFormattedText(m_sText, params, sStorage));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogErrorAnimNode, 1, ezRTTIDefaultAllocator<ezLogErrorAnimNode>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Log Error: '{Text}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLogErrorAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  ezVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  ezStringBuilder sStorage;
  ezLog::Error(BuildFormattedText(m_sText, params, sStorage));
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_DebugAnimNodes);
