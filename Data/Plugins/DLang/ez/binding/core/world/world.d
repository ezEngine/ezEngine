module ez.core.world.world;

public import ez.foundation.types.types;
public import ez.foundation.math.random;
public import ez.foundation.time.clock;
public import ez.core.world.gameobject;

extern(C++, class) struct ezWorld
{
  // CODEGEN-BEGIN: Struct("ezWorld")
  // constructor - unsupported argument 'desc'
  void Clear();
  const(char)* GetName() const;
  ubyte GetIndex() const;
  // method 'CreateObject' - unsupported argument 'desc'
  // method 'CreateObject' - unsupported argument 'desc'
  void DeleteObjectNow(ref const(ezGameObjectHandle) object, bool bAlsoDeleteEmptyParents = true);
  void DeleteObjectDelayed(ref const(ezGameObjectHandle) object, bool bAlsoDeleteEmptyParents = true);
  // method 'GetObjectDeletionEvent' - unsupported return type
  bool IsValidObject(ref const(ezGameObjectHandle) object) const;
  bool TryGetObject(ref const(ezGameObjectHandle) object, ref ezGameObject out_pObject);
  bool TryGetObject(ref const(ezGameObjectHandle) object, ref const(ezGameObject) out_pObject) const;
  // method 'TryGetObjectWithGlobalKey' - unsupported argument 'sGlobalKey'
  // method 'TryGetObjectWithGlobalKey' - unsupported argument 'sGlobalKey'
  uint GetObjectCount() const;
  // method 'GetObjects' - unsupported return type
  // method 'GetObjects' - unsupported return type
  // Typedef 'VisitorFunc' has unsupported type
  enum TraversalMethod : int
  {
    BreadthFirst = 0,
    DepthFirst = 1,
  }
  // method 'Traverse' - unsupported argument 'visitorFunc'
  // method 'GetOrCreateModule' - unsupported return type
  void DeleteModule(const(ezRTTI)* pRtti);
  // method 'GetModule' - unsupported return type
  // method 'GetModule' - unsupported return type
  // method 'GetOrCreateManagerForComponentType' - unsupported return type
  // method 'GetManagerForComponentType' - unsupported return type
  // method 'GetManagerForComponentType' - unsupported return type
  // method 'IsValidComponent' - unsupported argument 'component'
  // method 'TryGetComponent' - unsupported argument 'component'
  // method 'TryGetComponent' - unsupported argument 'component'
  // method 'CreateComponentInitBatch' - unsupported return type
  // method 'DeleteComponentInitBatch' - unsupported argument 'batch'
  // method 'BeginAddingComponentsToInitBatch' - unsupported argument 'batch'
  // method 'EndAddingComponentsToInitBatch' - unsupported argument 'batch'
  // method 'SubmitComponentInitBatch' - unsupported argument 'batch'
  // method 'IsComponentInitBatchCompleted' - unsupported argument 'batch'
  // method 'CancelComponentInitBatch' - unsupported argument 'batch'
  // method 'SendMessage' - unsupported argument 'msg'
  // method 'SendMessageRecursive' - unsupported argument 'msg'
  // method 'PostMessage' - unsupported argument 'msg'
  // method 'PostMessageRecursive' - unsupported argument 'msg'
  // method 'SendMessage' - unsupported argument 'receiverComponent'
  // method 'PostMessage' - unsupported argument 'receiverComponent'
  // method 'FindEventMsgHandlers' - unsupported argument 'msg'
  void SetWorldSimulationEnabled(bool bEnable);
  bool GetWorldSimulationEnabled() const;
  void Update();
  // method 'GetUpdateTask' - unsupported return type
  // method 'GetSpatialSystem' - unsupported return type
  // method 'GetSpatialSystem' - unsupported return type
  // method 'GetCoordinateSystem' - unsupported argument 'out_CoordinateSystem'
  // method 'SetCoordinateSystemProvider' - unsupported argument 'pProvider'
  // method 'GetCoordinateSystemProvider' - unsupported return type
  // method 'GetCoordinateSystemProvider' - unsupported return type
  ref ezClock GetClock();
  ref const(ezClock) GetClock() const;
  ref ezRandom GetRandomNumberGenerator();
  // method 'GetAllocator' - unsupported return type
  // method 'GetBlockAllocator' - unsupported return type
  // method 'GetStackAllocator' - unsupported return type
  // method 'GetReadMarker' - unsupported return type
  // method 'GetWriteMarker' - unsupported return type
  void SetUserData(void* pUserData);
  void* GetUserData() const;
  // Typedef 'ReferenceResolver' has unsupported type
  // method 'SetGameObjectReferenceResolver' - unsupported argument 'resolver'
  // method 'GetGameObjectReferenceResolver' - unsupported return type
  static ulong GetMaxNumGameObjects();
  static ulong GetMaxNumHierarchyLevels();
  static ulong GetMaxNumComponentsPerType();
  static ulong GetMaxNumWorldModules();
  static ulong GetMaxNumComponentTypes();
  static ulong GetMaxNumWorlds();
  static uint GetWorldCount();
  static ezWorld* GetWorld(uint uiIndex);
  static ezWorld* GetWorld(ref const(ezGameObjectHandle) object);
  // method 'GetWorld' - unsupported argument 'component'
  // Field 'm_pUpdateTask' has unsupported type
  // Field 'm_Data' has unsupported type
  // Typedef 'QueuedMsgMetaData' has unsupported type
private:
  uint m_uiIndex;
  // Variable 's_Worlds' has unsupported type
  // CODEGEN-END
}
