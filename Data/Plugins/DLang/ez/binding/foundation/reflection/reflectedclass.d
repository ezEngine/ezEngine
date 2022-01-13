module ez.foundation.reflection.reflectedclass;

extern(C++, class) struct ezRTTI
{
  // CODEGEN-BEGIN: InterfaceStruct("ezRTTI")
  @disable this();
  // Typedef 'ezEnumerableBase' has unsupported type
  static ezRTTI* GetFirstInstance();
  ezRTTI* GetNextInstance();
  const(ezRTTI)* GetNextInstance() const;
  // constructor - unsupported argument 'flags'
  void VerifyCorrectness() const;
  static void VerifyCorrectnessForAllTypes();
  const(char)* GetTypeName() const;
  ulong GetTypeNameHash() const;
  const(ezRTTI)* GetParentType() const;
  // method 'GetVariantType' - unsupported return type
  bool IsDerivedFrom(const(ezRTTI)* pBaseType) const;
  bool IsDerivedFrom() const;
  // method 'GetAllocator' - unsupported return type
  // method 'GetProperties' - unsupported return type
  // method 'GetFunctions' - unsupported return type
  // method 'GetAttributes' - unsupported return type
  // method 'GetAllProperties' - unsupported argument 'out_Properties'
  uint GetTypeSize() const;
  uint GetTypeVersion() const;
  // method 'GetTypeFlags' - unsupported return type
  static ezRTTI* FindTypeByName(const(char)* szName);
  static ezRTTI* FindTypeByNameHash(ulong uiNameHash);
  static ezRTTI* FindTypeByNameHash32(uint uiNameHash);
  // method 'FindPropertyByName' - unsupported return type
  const(char)* GetPluginName() const;
  // method 'GetMessageHandlers' - unsupported return type
  // method 'DispatchMessage' - unsupported argument 'msg'
  // method 'DispatchMessage' - unsupported argument 'msg'
  bool CanHandleMessage(ushort id) const;
  // method 'GetMessageSender' - unsupported return type
  // method 'GetAllTypesDerivedFrom' - unsupported return type
  // Operator: =
  // CODEGEN-END
}

// CODEGEN: WhitelistType("ezReflectedClass", ReferenceType)
extern(C++, class) class ezReflectedClass
{
  const(ezRTTI)* GetDynamicRTTI() const;
  void vtbl_Destructor() {}
  final bool IsInstanceOf(const(ezRTTI)* pType) const;
}