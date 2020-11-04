#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezSetBlackboardValueAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetBlackboardValueAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSetBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;      // [ property ]

  float m_fOnActivatedValue = 1.0f;   // [ property ]
  float m_fOnDeactivatedValue = 0.0f; // [ property ]
  bool m_bSetOnActivation = true;     // [ property ]
  bool m_bSetOnDeactivation = true;   // [ property ]

private:
  ezAnimGraphTriggerInputPin m_Active; // [ property ]
  ezHashedString m_sBlackboardEntry;
  bool m_bLastActiveState = false;
};

struct ezComparisonOperator
{
  using StorageType = ezUInt8;
  enum Enum
  {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Default = Equal
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezComparisonOperator);

class EZ_RENDERERCORE_DLL ezCheckBlackboardValueAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCheckBlackboardValueAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCheckBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;      // [ property ]

  float m_fReferenceValue = 1.0f;            // [ property ]
  ezEnum<ezComparisonOperator> m_Comparison; // [ property ]

private:
  ezAnimGraphTriggerOutputPin m_Active; // [ property ]

  ezHashedString m_sBlackboardEntry;
};
