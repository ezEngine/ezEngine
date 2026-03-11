#pragma once

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct ezVisualScriptVariableType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Invalid = ezVisualScriptDataType::Invalid,
    Bool = ezVisualScriptDataType::Bool,
    Byte,
    Int,
    Int64,
    Float,
    Double,
    Color,
    Vector3,
    Quaternion,
    Transform,
    Time,
    Angle,
    String,
    HashedString,
    GameObject,
    Component,
    TypedPointer,
    Variant,

    Resource = ezVisualScriptDataType::Resource,

    Default = Int,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptVariableType);

//////////////////////////////////////////////////////////////////////////////

struct ezVisualScriptVariableCategory
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Member,
    Array,
    Map,
    Default = Member,
  };

  static ezPropertyCategory::Enum GetPropertyCategory(Enum category);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptVariableCategory);

//////////////////////////////////////////////////////////////////////////////

struct ezVisualScriptVariableTypeDeclaration
{
  EZ_DECLARE_POD_TYPE();

  ezEnum<ezVisualScriptVariableType> m_Type;
  ezEnum<ezVisualScriptVariableCategory> m_Category;
  bool m_bPublic = false;
  ezUInt8 m_uiPadding = 0;

  bool operator==(const ezVisualScriptVariableTypeDeclaration& other) const
  {
    return m_Type == other.m_Type && m_Category == other.m_Category && m_bPublic == other.m_bPublic;
  }

  ezVisualScriptDataType::Enum GetDataType() const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptVariableTypeDeclaration);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezVisualScriptVariableTypeDeclaration);

class ezStreamWriter;
class ezStreamReader;

void operator<<(ezStreamWriter& inout_stream, const ezVisualScriptVariableTypeDeclaration& value);
void operator>>(ezStreamReader& inout_stream, ezVisualScriptVariableTypeDeclaration& value);

template <>
struct ezHashHelper<ezVisualScriptVariableTypeDeclaration>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezVisualScriptVariableTypeDeclaration& value) { return ezHashHelper<ezUInt32>::Hash(*(const ezUInt32*)&value); }

  EZ_ALWAYS_INLINE static bool Equal(const ezVisualScriptVariableTypeDeclaration& a, const ezVisualScriptVariableTypeDeclaration& b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

class ezVisualScriptVariableAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptVariableAttribute, ezTypeWidgetAttribute);
};

//////////////////////////////////////////////////////////////////////////

class ezQtVisualScriptVariableWidget : public ezQtVariantPropertyWidget
{
  Q_OBJECT;

public:
  ezQtVisualScriptVariableWidget();
  virtual ~ezQtVisualScriptVariableWidget();

  virtual void InternalSetValue(const ezVariant& value) override;

  virtual ezResult GetVariantTypeDisplayName(ezVariantType::Enum type, ezStringBuilder& out_sName) const override;
};

//////////////////////////////////////////////////////////////////////////

class ezQtVisualScriptVariableTypeDeclarationWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  ezQtVisualScriptVariableTypeDeclarationWidget();
  virtual ~ezQtVisualScriptVariableTypeDeclarationWidget();

  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

private:
  void ChangeType();

  QHBoxLayout* m_pLayout = nullptr;
  QComboBox* m_pTypeList = nullptr;
  QMenu* m_pCategoryList = nullptr;
  QPushButton* m_pCategoryButton = nullptr;
  QPushButton* m_pVisibilityButton = nullptr;
};

//////////////////////////////////////////////////////////////////////////////

struct ezVisualScriptVariable
{
  ezHashedString m_sName;
  ezVisualScriptVariableTypeDeclaration m_TypeDecl;
  ezVariant m_DefaultValue;

  bool m_bClampRange = false;
  double m_fMinValue = 0.0;
  double m_fMaxValue = 1.0;

  static void ConvertDefaultValue(ezVariant& inout_defaultValue, ezVisualScriptVariableTypeDeclaration targetTypeDecl);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptVariable);

//////////////////////////////////////////////////////////////////////////

struct ezVisualScriptExpressionDataType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Int = static_cast<ezUInt8>(ezProcessingStream::DataType::Int),
    Float = static_cast<ezUInt8>(ezProcessingStream::DataType::Float),
    Vector3 = static_cast<ezUInt8>(ezProcessingStream::DataType::Float3),
    Color = static_cast<ezUInt8>(ezProcessingStream::DataType::Float4),

    Default = Float
  };

  static ezVisualScriptDataType::Enum GetVisualScriptDataType(Enum dataType);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptExpressionDataType);

struct ezVisualScriptExpressionVariable
{
  ezHashedString m_sName;
  ezEnum<ezVisualScriptExpressionDataType> m_Type;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORPLUGINVISUALSCRIPT_DLL, ezVisualScriptExpressionVariable);
