module ez.core.world.component;

public import ez.foundation.reflection.reflectedclass;
public import ez.core.world.gameobject;

extern(C++, class) class ezComponent : ezReflectedClass
{
  // CODEGEN-BEGIN: Class("ezComponent")
  alias SUPER = ezReflectedClass;
  static const(ezRTTI)* GetStaticRTTI();
  override const(ezRTTI)* GetDynamicRTTI() const;
  final void SetActiveFlag(bool bEnabled);
  final bool GetActiveFlag() const;
  final bool IsActive() const;
  final bool IsActiveAndInitialized() const;
  final bool IsActiveAndSimulating() const;
  // method 'GetOwningManager' - unsupported return type
  // method 'GetOwningManager' - unsupported return type
  final ezGameObject GetOwner();
  final const(ezGameObject) GetOwner() const;
  final ezWorld* GetWorld();
  final const(ezWorld)* GetWorld() const;
  // method 'GetHandle' - unsupported return type
  final uint GetUniqueID() const;
  final void SetUniqueID(uint uiUniqueID);
  // method 'SerializeComponent' - unsupported argument 'stream'
  // method 'DeserializeComponent' - unsupported argument 'stream'
  final void EnsureInitialized();
  final void EnsureSimulationStarted();
  // method 'SendMessage' - unsupported argument 'msg'
  // method 'SendMessage' - unsupported argument 'msg'
  // method 'PostMessage' - unsupported argument 'msg'
  final void SetUserFlag(ubyte flagIndex, bool set);
  final bool GetUserFlag(ubyte flagIndex) const;
  // Operator: =
  // CODEGEN-END
}
