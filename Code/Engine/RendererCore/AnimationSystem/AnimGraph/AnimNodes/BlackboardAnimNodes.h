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

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSetBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  float m_fOnActivatedValue = 1.0f;   // [ property ]
  float m_fOnHoldValue = 1.0f;        // [ property ]
  float m_fOnDeactivatedValue = 0.0f; // [ property ]
  bool m_bSetOnActivation = true;     // [ property ]
  bool m_bSetOnHold = false;          // [ property ]
  bool m_bSetOnDeactivation = false;  // [ property ]

private:
  ezAnimGraphTriggerInputPin m_ActivePin; // [ property ]
  ezHashedString m_sBlackboardEntry;      // [ property ]

  struct InstanceData
  {
    bool m_bLastActiveState = false;
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezCheckBlackboardValueAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCheckBlackboardValueAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezCheckBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  float m_fReferenceValue = 1.0f;            // [ property ]
  ezEnum<ezComparisonOperator> m_Comparison; // [ property ]

private:
  ezAnimGraphTriggerOutputPin m_ActivePin;    // [ property ]
  ezAnimGraphTriggerOutputPin m_OnActivedPin; // [ property ]
  ezHashedString m_sBlackboardEntry;          // [ property ]

  struct InstanceData
  {
    bool m_bIsActive = false;
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezGetBlackboardNumberAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGetBlackboardNumberAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCheckBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  ezAnimGraphNumberOutputPin m_NumberPin; // [ property ]
  ezHashedString m_sBlackboardEntry;      // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezGetBlackboardBoolAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGetBlackboardBoolAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCheckBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  ezAnimGraphBoolOutputPin m_BoolPin; // [ property ]
  ezHashedString m_sBlackboardEntry;  // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezOnBlackboardValueChangedAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOnBlackboardValueChangedAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezOnBlackboardValuechangedAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  ezAnimGraphTriggerOutputPin m_ActivePin; // [ property ]
  ezHashedString m_sBlackboardEntry;       // [ property ]

  struct InstanceData
  {
    ezUInt32 m_uiChangeCounter = ezInvalidIndex;
  };
};
