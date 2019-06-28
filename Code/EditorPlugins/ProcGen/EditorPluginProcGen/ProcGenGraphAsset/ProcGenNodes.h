#pragma once

#include <Foundation/Utilities/Node.h>
#include <ProcGenPlugin/VM/ExpressionAST.h>

class ezProcGenNodeBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenNodeBase, ezReflectedClass);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) = 0;
};

//////////////////////////////////////////////////////////////////////////

class ezProcGenOutput : public ezProcGenNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenOutput, ezProcGenNodeBase);

public:
  bool m_bActive = true;

  ezString m_sName;

  ezUInt32 m_uiByteCodeIndex = ezInvalidIndex;
};

//////////////////////////////////////////////////////////////////////////

class ezProcGenPlacementOutput : public ezProcGenOutput
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenPlacementOutput, ezProcGenOutput);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  void Save(ezStreamWriter& stream);

  ezHybridArray<ezString, 4> m_ObjectsToPlace;

  float m_fFootprint = 1.0f;

  ezVec3 m_vMinOffset = ezVec3(0);
  ezVec3 m_vMaxOffset = ezVec3(0);

  float m_fAlignToNormal = 1.0f;

  ezVec3 m_vMinScale = ezVec3(1);
  ezVec3 m_vMaxScale = ezVec3(1);

  float m_fCullDistance = 30.0f;

  ezUInt32 m_uiCollisionLayer = 0;

  ezString m_sSurface;

  ezString m_sColorGradient;

  ezInputNodePin m_DensityPin;
  ezInputNodePin m_ScalePin;
  ezInputNodePin m_ColorIndexPin;
  ezInputNodePin m_ObjectIndexPin;
};

//////////////////////////////////////////////////////////////////////////

class ezProcGenVertexColorOutput : public ezProcGenOutput
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenVertexColorOutput, ezProcGenOutput);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  void Save(ezStreamWriter& stream);

  ezInputNodePin m_RPin;
  ezInputNodePin m_GPin;
  ezInputNodePin m_BPin;
  ezInputNodePin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class ezProcGenRandom : public ezProcGenNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenRandom, ezProcGenNodeBase);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  ezInt32 m_iSeed = -1;

  ezOutputNodePin m_OutputValuePin;

private:
  void OnObjectCreated(const ezAbstractObjectNode& node);

  ezUInt32 m_uiAutoSeed;
};

//////////////////////////////////////////////////////////////////////////

class ezProcGenBlend : public ezProcGenNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenBlend, ezProcGenNodeBase);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  float m_fInputValueA = 1.0f;
  float m_fInputValueB = 1.0f;

  ezInputNodePin m_InputValueAPin;
  ezInputNodePin m_InputValueBPin;
  ezOutputNodePin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class ezProcGenHeight : public ezProcGenNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenHeight, ezProcGenNodeBase);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  float m_fMinHeight = 0.0f;
  float m_fMaxHeight = 1000.0f;
  float m_fLowerFade = 0.2f;
  float m_fUpperFade = 0.2f;

  ezOutputNodePin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class ezProcGenSlope : public ezProcGenNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenSlope, ezProcGenNodeBase);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  ezAngle m_MinSlope = ezAngle::Degree(0.0f);
  ezAngle m_MaxSlope = ezAngle::Degree(30.0f);
  float m_fLowerFade = 0.0f;
  float m_fUpperFade = 0.2f;

  ezOutputNodePin m_OutputValuePin;
};


