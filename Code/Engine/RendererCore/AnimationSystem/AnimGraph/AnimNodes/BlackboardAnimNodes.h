#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezSetBlackboardNumberAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetBlackboardNumberAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSetBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  double m_fNumber = 0.0f;                      // [ property ]

private:
  ezAnimGraphTriggerInputPin m_InActivate;      // [ property ]
  ezAnimGraphNumberInputPin m_InNumber;         // [ property ]
  ezHashedString m_sBlackboardEntry;            // [ property ]
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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezGetBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  ezHashedString m_sBlackboardEntry;            // [ property ]
  ezAnimGraphNumberOutputPin m_OutNumber;       // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezCompareBlackboardNumberAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCompareBlackboardNumberAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezCompareBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  double m_fReferenceValue = 0.0;               // [ property ]
  ezEnum<ezComparisonOperator> m_Comparison;    // [ property ]

private:
  ezHashedString m_sBlackboardEntry;            // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnTrue;      // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFalse;     // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsTrue;         // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsFalse;        // [ property ]

  struct InstanceData
  {
    ezInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezCheckBlackboardBoolAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCheckBlackboardBoolAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezCheckBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  ezHashedString m_sBlackboardEntry;            // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnTrue;      // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFalse;     // [ property ]
  ezAnimGraphBoolOutputPin m_OutBool;           // [ property ]

  struct InstanceData
  {
    ezInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezSetBlackboardBoolAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetBlackboardBoolAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSetBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  bool m_bBool = false;                         // [ property ]

private:
  ezHashedString m_sBlackboardEntry;            // [ property ]
  ezAnimGraphTriggerInputPin m_InActivate;      // [ property ]
  ezAnimGraphBoolInputPin m_InBool;             // [ property ]
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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezGetBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  ezHashedString m_sBlackboardEntry;            // [ property ]
  ezAnimGraphBoolOutputPin m_OutBool;           // [ property ]
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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezOnBlackboardValuechangedAnimNode

public:
  void SetBlackboardEntry(const char* szEntry);    // [ property ]
  const char* GetBlackboardEntry() const;          // [ property ]

private:
  ezHashedString m_sBlackboardEntry;               // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnValueChanged; // [ property ]

  struct InstanceData
  {
    ezUInt32 m_uiChangeCounter = ezInvalidIndex;
  };
};
