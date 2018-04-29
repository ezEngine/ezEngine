#pragma once

#include <Foundation/Utilities/Node.h>
#include <ProceduralPlacementPlugin/VM/ExpressionAST.h>

class ezProceduralPlacementNodeBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementNodeBase, ezReflectedClass);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) = 0;
};

//////////////////////////////////////////////////////////////////////////

class ezProceduralPlacementLayerOutput : public ezProceduralPlacementNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementLayerOutput, ezProceduralPlacementNodeBase);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  void Save(ezStreamWriter& stream);

  ezString m_sName;
  ezHybridArray<ezString, 4> m_ObjectsToPlace;

  float m_fFootprint;

  ezVec3 m_vMinOffset;
  ezVec3 m_vMaxOffset;

  float m_fAlignToNormal;

  ezVec3 m_vMinScale;
  ezVec3 m_vMaxScale;

  float m_fCullDistance;

  ezUInt32 m_uiByteCodeIndex;

  ezInputNodePin m_DensityPin;
  ezInputNodePin m_ScalePin;
  ezInputNodePin m_ColorIndexPin;
  ezInputNodePin m_ObjectIndexPin;
};

//////////////////////////////////////////////////////////////////////////

class ezProceduralPlacementRandom : public ezProceduralPlacementNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementRandom, ezProceduralPlacementNodeBase);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  ezInt32 m_iSeed = -1;

  ezOutputNodePin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class ezProceduralPlacementBlend : public ezProceduralPlacementNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementBlend, ezProceduralPlacementNodeBase);

public:
  virtual ezExpressionAST::Node* GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast) override;

  float m_fInputValueA = 1.0f;
  float m_fInputValueB = 1.0f;

  ezInputNodePin m_InputValueAPin;
  ezInputNodePin m_InputValueBPin;
  ezOutputNodePin m_OutputValuePin;
};


