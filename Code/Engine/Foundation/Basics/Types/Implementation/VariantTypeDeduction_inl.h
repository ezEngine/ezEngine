
template <>
struct ezVariant::TypeDeduction<bool>
{
  enum
  {
    value = Type::Bool,
    forceSharing = false
  };

  typedef bool StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezInt32>
{
  enum
  {
    value = Type::Int32,
    forceSharing = false
  };

  typedef ezInt32 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezUInt32>
{
  enum
  {
    value = Type::UInt32,
    forceSharing = false
  };

  typedef ezUInt32 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezInt64>
{
  enum
  {
    value = Type::Int64,
    forceSharing = false
  };

  typedef ezInt64 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezUInt64>
{
  enum
  {
    value = Type::UInt64,
    forceSharing = false
  };

  typedef ezUInt64 StorageType;
};

template <>
struct ezVariant::TypeDeduction<float>
{
  enum
  {
    value = Type::Float,
    forceSharing = false
  };

  typedef float StorageType;
};

template <>
struct ezVariant::TypeDeduction<double>
{
  enum
  {
    value = Type::Double,
    forceSharing = false
  };

  typedef double StorageType;
};

//template <>
//struct ezVariant::TypeDeduction<ezColor>
//{
//  enum
//  {
//    value = Type::Color,
//    forceSharing = false
//  };
//
//  typedef ezColor StorageType;
//};

template <>
struct ezVariant::TypeDeduction<ezVec2>
{
  enum
  {
    value = Type::Vector2,
    forceSharing = false
  };

  typedef ezVec2 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezVec3>
{
  enum
  {
    value = Type::Vector3,
    forceSharing = false
  };

  typedef ezVec3 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezVec4>
{
  enum
  {
    value = Type::Vector4,
    forceSharing = false
  };

  typedef ezVec4 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezQuat>
{
  enum
  {
    value = Type::Quaternion,
    forceSharing = false
  };

  typedef ezQuat StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezMat3>
{
  enum
  {
    value = Type::Matrix3,
    forceSharing = false
  };

  typedef ezMat3 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezMat4>
{
  enum
  {
    value = Type::Matrix4,
    forceSharing = false
  };

  typedef ezMat4 StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezString>
{
  enum
  {
    value = Type::String,
    forceSharing = true
  };

  typedef ezString StorageType;
};

template <>
struct ezVariant::TypeDeduction<char*>
{
  enum
  {
    value = Type::String,
    forceSharing = true
  };
  
  typedef ezString StorageType;
};

template <>
struct ezVariant::TypeDeduction<const char*>
{
  enum
  {
    value = Type::String,
    forceSharing = true
  };

  typedef ezString StorageType;
};

template <size_t N>
struct ezVariant::TypeDeduction<char[N]>
{
  enum
  {
    value = Type::String,
    forceSharing = true
  };
  
  typedef ezString StorageType;
};

template <size_t N>
struct ezVariant::TypeDeduction<const char[N]>
{
  enum
  {
    value = Type::String,
    forceSharing = true
  };

  typedef ezString StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezTime>
{
  enum
  {
    value = Type::Time,
    forceSharing = false
  };

  typedef ezTime StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezVariantArray>
{
  enum
  {
    value = Type::VariantArray,
    forceSharing = true
  };

  typedef ezVariantArray StorageType;
};

template <>
struct ezVariant::TypeDeduction<ezArrayPtr<ezVariant> >
{
  enum
  {
    value = Type::VariantArray,
    forceSharing = true
  };

  typedef ezVariantArray StorageType;
};


template <>
struct ezVariant::TypeDeduction<ezVariantDictionary>
{
  enum
  {
    value = Type::VariantDictionary,
    forceSharing = true
  };

  typedef ezVariantDictionary StorageType;
};

//template <>
//struct ezVariant::TypeDeduction<ezObject*>
//{
//};

template <>
struct ezVariant::TypeDeduction<void*>
{
  enum
  {
    value = Type::VoidPointer,
    forceSharing = false
  };

  typedef void* StorageType;
};
