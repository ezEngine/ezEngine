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

export class GameObjectDesc {

  Active: boolean = false;
  Dynamic: boolean = false;
  TeamID: number = 0;
  Name: string;
  Parent: GameObject = null;

  LocalPosition: Vec3 = null; // default is (0, 0, 0)
  LocalRotation: Quat = null; // default is identity
  LocalScaling: Vec3 = null; // default is (1, 1, 1)
  LocalUniformScaling: number = 1;
};

declare function __CPP_World_DeleteObjectDelayed(object: GameObject): void;
declare function __CPP_World_CreateObject(desc: GameObjectDesc): GameObject;
declare function __CPP_World_CreateComponent(owner: GameObject, componentTypeNameHash: number): any;
declare function __CPP_World_DeleteComponent(component: Component): void;
declare function __CPP_World_TryGetObjectWithGlobalKey(globalKey: string): GameObject;

declare function __CPP_World_Clock_SetSpeed(speed: number): void;
declare function __CPP_World_Clock_GetSpeed(): number;
declare function __CPP_World_Clock_GetTimeDiff(): number;
declare function __CPP_World_Clock_GetAccumulatedTime(): number;

declare function __CPP_World_RNG_Bool(): boolean;
declare function __CPP_World_RNG_DoubleZeroToOneExclusive(): number;
declare function __CPP_World_RNG_DoubleZeroToOneInclusive(): number;

declare function __CPP_World_RNG_UIntInRange(range: number): number;
declare function __CPP_World_RNG_DoubleVarianceAroundZero(variance: number): number;

declare function __CPP_World_RNG_IntInRange(minValue: number, range: number): number;
declare function __CPP_World_RNG_IntMinMax(minValue: number, maxValue: number): number;
declare function __CPP_World_RNG_DoubleInRange(minValue: number, range: number): number;
declare function __CPP_World_RNG_DoubleMinMax(minValue: number, maxValue: number): number;
declare function __CPP_World_RNG_DoubleVariance(value: number, variance: number): number;

declare function __CPP_World_FindObjectsInSphere(center: Vec3, radius: number, callback: (go: GameObject) => boolean): void;
declare function __CPP_World_FindObjectsInBox(min: Vec3, max: Vec3, callback: (go: GameObject) => boolean): void;

declare function __CPP_World_Debug_DrawCross(pos: Vec3, size: number, color: Color);
declare function __CPP_World_Debug_DrawLines(lines: World.Debug.Line[], color: Color);
declare function __CPP_World_Debug_DrawLineBox(min: Vec3, max: Vec3, color: Color, transform: Transform);
declare function __CPP_World_Debug_DrawSolidBox(min: Vec3, max: Vec3, color: Color, transform: Transform)
declare function __CPP_World_Debug_DrawLineSphere(center: Vec3, radius: number, color: Color, transform: Transform);
declare function __CPP_World_Debug_Draw3DText(text: string, pos: Vec3, color: Color, sizeInPixel: number);

export namespace World {

  export function CreateObject(desc: GameObjectDesc): GameObject {
    return __CPP_World_CreateObject(desc);
  }

  export function DeleteObjectDelayed(object: GameObject): void {
    __CPP_World_DeleteObjectDelayed(object);
  }

  export function CreateComponent<TYPE extends Component>(owner: GameObject, typeClass: new () => TYPE): TYPE {
    return __CPP_World_CreateComponent(owner, typeClass.GetTypeNameHash());
  }

  export function DeleteComponent(component: Component): void {
    __CPP_World_DeleteComponent(component);
  }

  export function TryGetObjectWithGlobalKey(globalKey: string): GameObject {
    return __CPP_World_TryGetObjectWithGlobalKey(globalKey);
  }

  export function FindObjectsInSphere(center: Vec3, radius: number, callback: (go: GameObject) => boolean): void {
    __CPP_World_FindObjectsInSphere(center, radius, callback);
  }

  export function FindObjectsInBox(min: Vec3, max: Vec3, callback: (go: GameObject) => boolean): void {
    __CPP_World_FindObjectsInBox(min, max, callback);
  }

  export namespace Clock {

    export function SetClockSpeed(speed: number): void {
      __CPP_World_Clock_SetSpeed(speed);
    }

    export function GetClockSpeed(): number {
      return __CPP_World_Clock_GetSpeed();
    }

    export function GetTimeDiff(): number {
      return __CPP_World_Clock_GetTimeDiff();
    }

    export function GetAccumulatedTime(): number {
      return __CPP_World_Clock_GetAccumulatedTime();
    }
  }

  export namespace Random {
    export function Bool(): boolean {
      return __CPP_World_RNG_Bool();
    }

    export function DoubleZeroToOneExclusive(): number {
      return __CPP_World_RNG_DoubleZeroToOneExclusive();
    }

    export function DoubleZeroToOneInclusive(): number {
      return __CPP_World_RNG_DoubleZeroToOneInclusive();
    }

    export function UIntInRange(range: number): number {
      return __CPP_World_RNG_UIntInRange(range);
    }

    export function DoubleVarianceAroundZero(variance: number): number {
      return __CPP_World_RNG_DoubleVarianceAroundZero(variance);
    }

    export function IntInRange(minValue: number, range: number): number {
      return __CPP_World_RNG_IntInRange(minValue, range);
    }

    export function IntMinMax(minValue: number, maxValue: number): number {
      return __CPP_World_RNG_IntMinMax(minValue, maxValue);
    }

    export function DoubleInRange(minValue: number, range: number): number {
      return __CPP_World_RNG_DoubleInRange(minValue, range);
    }

    export function DoubleMinMax(minValue: number, maxValue: number): number {
      return __CPP_World_RNG_DoubleMinMax(minValue, maxValue);
    }

    export function DoubleVariance(value: number, variance: number): number {
      return __CPP_World_RNG_DoubleVariance(value, variance);
    }
  }

  export namespace Debug {

    export function DrawCross(pos: Vec3, size: number, color: Color) {
      __CPP_World_Debug_DrawCross(pos, size, color);
    }

    export class Line {
      startX: number = 0;
      startY: number = 0;
      startZ: number = 0;
      endX: number = 0;
      endY: number = 0;
      endZ: number = 0;
    }

    export function DrawLines(lines: Line[], color: Color = null) {
      __CPP_World_Debug_DrawLines(lines, color);
    }

    export function DrawLineBox(min: Vec3, max: Vec3, color: Color = null, transform: Transform = null) {
      __CPP_World_Debug_DrawLineBox(min, max, color, transform);
    }

    export function DrawSolidBox(min: Vec3, max: Vec3, color: Color = null, transform: Transform = null) {
      __CPP_World_Debug_DrawSolidBox(min, max, color, transform);
    }

    export function DrawLineSphere(center: Vec3, radius: number, color: Color = null, transform: Transform = null) {
      __CPP_World_Debug_DrawLineSphere(center, radius, color, transform);
    }

    export function Draw3DText(text: string, pos: Vec3, color: Color = null, sizeInPixel: number = 16) {
      __CPP_World_Debug_Draw3DText(text, pos, color, sizeInPixel);
    }

    // DrawLineBoxCorners
    // DrawLineCapsuleZ
    // DrawLineFrustum
  }

  // GetCoordinateSystem
};
