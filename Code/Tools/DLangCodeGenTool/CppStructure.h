#pragma once

#include "RapidXML/rapidxml.hpp"

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

using namespace rapidxml;

class ezStreamReader;

enum class CE_Visibility : ezUInt8
{
  None,
  Public,
  Protected,
  Private,
};

enum class CodeElementType : ezUInt8
{
  None,
  Namespace,
  Struct,
  Typedef,
  Class,
  Function,
  Variable,
  OperatorFunction,
  Unimplemented,
  Union,
  CvQualifiedType,
  Enumeration,
  FundamentalType,
  PointerType,
  Constructor,
  OperatorMethod,
  Destructor,
  Field,
  ElaboratedType,
  ReferenceType,
  Method,
  ArrayType,
  Converter,
  FunctionType,
  Comment,
  File,
};

struct CodeElement
{
  EZ_DECLARE_POD_TYPE();

  CodeElementType m_Type = CodeElementType::None;
  ezUInt32 m_uiIndex = 0;
};

struct CE_File
{
  ezString m_sPath;
};

struct CE_Comment
{
  EZ_DECLARE_POD_TYPE();
};

struct CE_Typedef
{
  ezString m_sName;
  ezString m_sTypeID;
  ezString m_sContextID;
};

struct CE_Structure
{
  enum class Type
  {
    None,
    Struct,
    Class,
    Union
  };

  Type m_Type = Type::None;
  ezString m_sName;
  ezString m_sContextID;
  // bool m_bAstract = false; TODO ?
  // TODO: bases
  ezHybridArray<ezString, 16> m_Members;
};

struct CE_Argument
{
  ezString m_sName;
  ezString m_sTypeID;
  ezString m_sDefault;
};

struct CE_Function
{
  ezString m_sName;
  ezString m_sReturnTypeID;
  ezString m_sContextID;

  ezHybridArray<CE_Argument, 4> m_Arguments;
};

struct CE_Variable
{
  ezString m_sName;
  ezString m_sTypeID;
  ezString m_sContextID;
  CE_Visibility m_Visibility;
  bool m_bStatic = false;
  // bool m_bExtern ?
  // TODO: init value ?
};

struct CE_ArrayType
{
  ezString m_sTypeID;
  ezUInt32 m_uiSize = ezInvalidIndex;
};

struct CE_FundamentalType
{
  ezString m_sName;
};

struct CE_PointerOrReferenceType
{
  ezString m_sTypeID;
};

struct CE_CvQualifiedType
{
  ezString m_sTypeID;
  bool m_bConst = false;
  bool m_bVolatile = false;
};

struct CE_Field
{
  ezString m_sName;
  ezString m_sTypeID;
  CE_Visibility m_Visibility;
  ezString m_sContextID;
};

struct CE_ElaboratedType
{
  ezString m_sTypeID;
};

struct CE_Method
{
  ezString m_sName;
  ezString m_sReturnTypeID;
  bool m_bConst = false;
  bool m_bStatic = false;
  bool m_bVirtual = false;
  bool m_bOverride = false;
  bool m_bArtificial = false;
  CE_Visibility m_Visibility;
  ezString m_sContextID;

  ezHybridArray<CE_Argument, 4> m_Arguments;
};

struct CE_Constructor
{
  bool m_bArtifical = false;
  CE_Visibility m_Visibility;
  ezString m_sContextID;

  ezHybridArray<CE_Argument, 4> m_Arguments;
};

struct CE_Destructor
{
  ezString m_sName;
  ezString m_sContextID;
  bool m_bArtifical = false;
  bool m_bVirtual = false; // TODO
  CE_Visibility m_Visibility;
};

struct CE_Namespace
{
  ezString m_sName;
  ezString m_sContextID;
  // TODO: members
};

struct CE_EnumValue
{
  ezString m_sName;
  ezString m_sValue;
};

struct CE_Enumeration
{
  ezString m_sName;
  ezString m_sTypeID;
  ezString m_sContextID;
  CE_Visibility m_Visibility;

  ezHybridArray<CE_EnumValue, 4> m_Values;
};

class CppStructure
{
public:
  ezResult ParseXML(ezStreamReader& stream);

  ezResult SynthesizeTemplate(const char* szTargetType, const char* szTemplateType1, const char* szTemplateType2);

  ezMap<ezString, CodeElement> m_IdToElement;

  ezDeque<CE_File> m_CesFile;
  ezDeque<CE_Comment> m_CesComment;
  ezDeque<CE_Typedef> m_CesTypedef;
  ezDeque<CE_Structure> m_CesStructure;
  ezDeque<CE_Function> m_CesFunction;
  ezDeque<CE_Variable> m_CesVariable;
  ezDeque<CE_FundamentalType> m_CesFundamentalType;
  ezDeque<CE_ArrayType> m_CesArrayType;
  ezDeque<CE_PointerOrReferenceType> m_CesPointerType;
  ezDeque<CE_PointerOrReferenceType> m_CesReferenceType;
  ezDeque<CE_CvQualifiedType> m_CesCvQualifiedType;
  ezDeque<CE_Field> m_CesField;
  ezDeque<CE_ElaboratedType> m_CesElaboratedType;
  ezDeque<CE_Method> m_CesMethod;
  ezDeque<CE_Constructor> m_CesConstructor;
  ezDeque<CE_Destructor> m_CesDestructor;
  ezDeque<CE_Function> m_CesOperatorFunction;
  ezDeque<CE_Namespace> m_CesNamespace;
  ezDeque<CE_Enumeration> m_CesEnumeration;
  ezDeque<CE_Method> m_CesOperatorMethod;

private:
  ezUInt32 ParseNamespace(xml_node<>* node);                          // [done]
  ezUInt32 ParseTypedef(xml_node<>* node);                            // [done]
  ezUInt32 ParseStructure(xml_node<>* node, CE_Structure::Type type); // [done]
  ezUInt32 ParseFunction(xml_node<>* node);                           // [done]
  ezUInt32 ParseVariable(xml_node<>* node);                           // [done]
  ezUInt32 ParseOperatorFunction(xml_node<>* node);                   // [done]
  ezUInt32 ParseUnimplemented(xml_node<>* node);                      // [done]
  ezUInt32 ParseCvQualifiedType(xml_node<>* node);                    // [done]
  ezUInt32 ParseEnumeration(xml_node<>* node);                        // [done]
  ezUInt32 ParseFundamentalType(xml_node<>* node);                    // [done]
  ezUInt32 ParsePointerType(xml_node<>* node);                        // [done]
  ezUInt32 ParseConstructor(xml_node<>* node);                        // [done]
  ezUInt32 ParseOperatorMethod(xml_node<>* node);                     // [done]
  ezUInt32 ParseDestructor(xml_node<>* node);                         // [done]
  ezUInt32 ParseField(xml_node<>* node);                              // [done]
  ezUInt32 ParseElaboratedType(xml_node<>* node);                     // [done]
  ezUInt32 ParseReferenceType(xml_node<>* node);                      // [done]
  ezUInt32 ParseMethod(xml_node<>* node);                             // [done]
  ezUInt32 ParseArrayType(xml_node<>* node);                          // [done]
  ezUInt32 ParseConverter(xml_node<>* node);                          //
  ezUInt32 ParseFunctionType(xml_node<>* node);                       //
  ezUInt32 ParseComment(xml_node<>* node);                            // [done]
  ezUInt32 ParseFile(xml_node<>* node);                               // [done]
  void ParseArguments(xml_node<>* first, ezDynamicArray<CE_Argument>& arguments);
  void ParseEnumValues(xml_node<>* first, ezDynamicArray<CE_EnumValue>& values);
  void ParseVisibility(xml_node<>* node, CE_Visibility& out);
  void ParseContext(xml_node<>* node, ezString& out);

  ezResult SynthesizeMember(CE_Structure& out, const ezString& m1, const ezString& m2);
  ezResult SynthesizeMethod(CE_Method& out, const CE_Method& m1, const CE_Method& m2);
  ezResult SynthesizeType(ezString& type, const ezString& m1, const ezString& m2);
  ezString RegisterSynthElement(CodeElementType type, ezUInt32 index);
  void PreprocessStructureForSynthesis(CE_Structure& structure) const;

  ezUInt32 m_uiTtCounter = 0;

  ezStringBuilder m_sXmlContent;
  xml_document<> m_XmlDoc;
};
