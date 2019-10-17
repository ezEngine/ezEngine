import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("./Component")
export import Component = __Component.Component;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

export class GameObjectDesc {

  Active: boolean = false;
  Dynamic: boolean = false;
  TeamID: number = 0;
  Name: string;
  Parent: GameObject = null;

  // TODO: make it work with null objects
  LocalPosition: Vec3 = new Vec3();
  LocalRotation: Quat = new Quat();
  LocalScaling: Vec3 = new Vec3(1, 1, 1);
  LocalUniformScaling: number = 1;

  constructor() {
    this.LocalRotation.SetIdentity();
  }
};

declare function __CPP_World_DeleteObjectDelayed(object: GameObject): void;
declare function __CPP_World_CreateObject(desc: GameObjectDesc): GameObject;
declare function __CPP_World_CreateComponent(parent: GameObject, componentTypeNameHash: number);

export namespace World {

  export function DeleteObjectDelayed(object: GameObject): void {
    __CPP_World_DeleteObjectDelayed(object);
  }

  export function CreateObject(desc: GameObjectDesc): GameObject {
    return __CPP_World_CreateObject(desc);
  }

  export function CreateComponent<TYPE extends Component>(parent: GameObject, typeClass: new () => TYPE): TYPE {
    return __CPP_World_CreateComponent(parent, typeClass.GetTypeNameHash());
  }

  // TryGetObject
  // TryGetObjectWithGlobalKey
  // IsValidComponent
  // TryGetComponent
  // SendMessage
  // SendMessageRecursive
  // PostMessage
  // PostMessageRecursive
  // GetCoordinateSystem
  // GetClock
  // GetRandomNumberGenerator
  // SpawnPrefab ?

};
