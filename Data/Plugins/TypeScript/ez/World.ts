import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("./Component")
export import Component = __Component.Component;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Color = require("./Color")
export import Color = __Color.Color;

import __Transform = require("./Transform")
export import Transform = __Transform.Transform;

/**
 * Describes the properties of a to-be-created GameObject.
 */
export class GameObjectDesc {

  ActiveFlag: boolean = true;       /** Whether the GO should start in an active state. */
  Dynamic: boolean = false;         /** Whether the GO should be considered 'dynamic', ie. can be moved around. */
  TeamID: number = 0;               /** The team index to give to this GO. */
  Name: string;                     /** An optional name for the GO. */
  Parent: GameObject = null;        /** An optional parent object to attach the new GO to. */

  LocalPosition: Vec3 = null;       /** The local position offset to the parent. Default is (0, 0, 0). */
  LocalRotation: Quat = null;       /** The local rotation offset to the parent. Default is identity. */
  LocalScaling: Vec3 = null;        /** The local scaling offset to the parent. Default is (1, 1, 1). */
  LocalUniformScaling: number = 1;  /** The local uniform scaling offset to the parent. Default is 1. */
};

declare function __CPP_World_DeleteObjectDelayed(object: GameObject): void;
declare function __CPP_World_CreateObject(desc: GameObjectDesc): GameObject;
declare function __CPP_World_CreateComponent(owner: GameObject, componentTypeNameHash: number): any;
declare function __CPP_World_DeleteComponent(component: Component): void;
declare function __CPP_World_TryGetObjectWithGlobalKey(globalKey: string): GameObject;

declare function __CPP_World_FindObjectsInSphere(type: string, center: Vec3, radius: number, callback: (go: GameObject) => boolean): void;
declare function __CPP_World_FindObjectsInBox(type: string, min: Vec3, max: Vec3, callback: (go: GameObject) => boolean): void;

/**
 * Functions to modify or interact with the world / scenegraph.
 */
export namespace World {

  /**
   * Creates a new GameObject on the C++ side and returns a TypeScript GameObject that links to that.
   */
  export function CreateObject(desc: GameObjectDesc): GameObject { // [tested]
    return __CPP_World_CreateObject(desc);
  }

  /**
   * Queues the given GO for deletion at the end of the frame. 
   * This is the typical way to delete objects, they stay alive until the end of the frame, to guarantee that GOs never disappear in the middle of a frame.
   * 
   * @param object The object to be deleted.
   */
  export function DeleteObjectDelayed(object: GameObject): void { // [tested]
    __CPP_World_DeleteObjectDelayed(object);
  }

  /**
   * Creates a new component of the given type and attaches it to the given GameObject.
   * 
   * Example:
   *   let slider = ez.World.CreateComponent(someGameObject, ez.SliderComponent);
   * 
   * @param owner The GameObject to attach the component to.
   * @param typeClass The component class type to instantiate.
   */
  export function CreateComponent<TYPE extends Component>(owner: GameObject, typeClass: new () => TYPE): TYPE { // [tested]
    return __CPP_World_CreateComponent(owner, typeClass.GetTypeNameHash());
  }

  /**
   * Instructs the C++ side to delete the given component.
   */
  export function DeleteComponent(component: Component): void { // [tested]
    __CPP_World_DeleteComponent(component);
  }

  /**
   * Searches the world for a game object with the given 'global key'.
   * Global keys must be unique within a world, thus this lookup is fast. However, working with global keys
   * can be messy and ensuring a global key is never used twice can be difficult, therefore it is advised to
   * use this concept with care.
   */
  export function TryGetObjectWithGlobalKey(globalKey: string): GameObject { // [tested]
    return __CPP_World_TryGetObjectWithGlobalKey(globalKey);
  }

  /** 
   * Searches for objects with a specified type category that overlap with the given sphere.
   * 
   * @param type The object type category to search for. See ezMarkerComponent for a way to attach a type category to an object.
   * @param center World-space center of the sphere.
   * @param radius Radius of the sphere.
   * @param callback A function that is used to report every overlapping GameObject.
   *                 To pass in a member function that has access to your 'this' object, declare your callback like this: 
   *                 FoundObjectCallback = (go: ez.GameObject): boolean => { ... }
   *                 As long as the callack returns 'true', further results will be delivered. Return 'false' to cancel.
   */
  export function FindObjectsInSphere(type: string, center: Vec3, radius: number, callback: (go: GameObject) => boolean): void { // [tested]
    __CPP_World_FindObjectsInSphere(type, center, radius, callback);
  }

  /** 
   * Searches for objects with a specified type category that overlap with the given box.
   * 
   * @param type The object type category to search for. See ezMarkerComponent for a way to attach a type category to an object.
   * @param min The minimum vertex of the AABB.
   * @param max The maximum vertex of the AABB.
   * @param callback A function that is used to report every overlapping GameObject.
   *                 To pass in a member function that has access to your 'this' object, declare your callback like this: 
   *                 FoundObjectCallback = (go: ez.GameObject): boolean => { ... }
   *                 As long as the callack returns 'true', further results will be delivered. Return 'false' to cancel.
   */
  export function FindObjectsInBox(type: string, min: Vec3, max: Vec3, callback: (go: GameObject) => boolean): void { // [tested]
    __CPP_World_FindObjectsInBox(type, min, max, callback);
  }
};
