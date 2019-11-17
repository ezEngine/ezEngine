import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Transform = require("./Transform")
export import Transform = __Transform.Transform;

import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

declare function __CPP_Physics_CastRay(start: Vec3, dir: Vec3, distance: number, collisionLayer: number, shapeTypes: Physics.ShapeType, ignoreShapeId: number): Physics.HitResult;
declare function __CPP_Physics_SweepTestSphere(sphereRadius: number, start: Vec3, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number): Physics.HitResult;
declare function __CPP_Physics_SweepTestBox(boxExtends: Vec3, start: Transform, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number): Physics.HitResult;
declare function __CPP_Physics_SweepTestCapsule(capsuleRadius: number, capsuleHeight: number, start: Transform, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number): Physics.HitResult;
declare function __CPP_Physics_OverlapTestSphere(sphereRadius: number, position: Vec3, collisionLayer: number, ignoreShapeId: number): boolean;
declare function __CPP_Physics_OverlapTestCapsule(capsuleRadius: number, capsuleHeight: number, transform: Transform, collisionLayer: number, ignoreShapeId: number): boolean;
declare function __CPP_Physics_GetGravity(): Vec3;

export namespace Physics {

    export enum ShapeType {
        Static = 1 << 0,
        Dynamic = 1 << 1,
    }

    export class HitResult {
        position: Vec3;
        normal: Vec3;
        distance: number;
        shapeObject: GameObject;
        actorObject: GameObject;
        shapeId: number;
    };

    export function CastRay(start: Vec3, dir: Vec3, distance: number, collisionLayer: number, shapeTypes: ShapeType = ShapeType.Static | ShapeType.Dynamic, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_CastRay(start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId);
    }

    export function SweepTestSphere(sphereRadius: number, start: Vec3, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_SweepTestSphere(sphereRadius, start, dir, distance, collisionLayer, ignoreShapeId);
    }

    export function SweepTestBox(boxExtends: Vec3, start: Transform, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_SweepTestBox(boxExtends, start, dir, distance, collisionLayer, ignoreShapeId);
    }

    export function SweepTestCapsule(capsuleRadius: number, capsuleHeight: number, start: Transform, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_SweepTestCapsule(capsuleRadius, capsuleHeight, start, dir, distance, collisionLayer, ignoreShapeId);
    }

    export function OverlapTestSphere(sphereRadius: number, position: Vec3, collisionLayer: number, ignoreShapeId: number = -1): boolean {
        return __CPP_Physics_OverlapTestSphere(sphereRadius, position, collisionLayer, ignoreShapeId);
    }

    export function OverlapTestCapsule(capsuleRadius: number, capsuleHeight: number, transform: Transform, collisionLayer: number, ignoreShapeId: number = -1): boolean {
        return __CPP_Physics_OverlapTestCapsule(capsuleRadius, capsuleHeight, transform, collisionLayer, ignoreShapeId);
    }

    //export function QueryDynamicShapesInSphere(sphereRadius: number, position: Vec3, collisionLayer: number, ezPhysicsOverlapResult & out_Results, ignoreShapeId: number = -1): void { }

    export function GetGravity(): Vec3 {
        return __CPP_Physics_GetGravity();
    }
}
