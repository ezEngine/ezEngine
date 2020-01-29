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
declare function __CPP_Physics_QueryDynamicShapesInSphere(radius: number, position: Vec3, collisionLayer: number, ignoreShapeId: number, callback: Physics.QueryShapeCallback): void;

/**
 * Functions in this module are typically implemented by a physics engine and operate on the physics representation of the world,
 * which may be more simplified than the graphical representation.
 */
export namespace Physics {

    /**
     * Callback declaration for retrieving information about one object that was found in a shape query.
     */
    export type QueryShapeCallback = (actor: GameObject, shape: GameObject, shapeId: number) => boolean;

    /**
     * Static physics shapes never move. In exchange, they cost much less performance.
     * Dynamic physics shapes are simulated according to rigid body dynamics and thus fall down, collide and interact with other dynamic shapes.
     */
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

    /**
     * Casts a ray in the physics world. Returns a HitResult for the closest object that was hit, or null if no object was hit.
     * 
     * @param start The start position of the ray in global space.
     * @param dir The normalized direction into which to cast the ray.
     * @param distance The length of the ray. Objects farther away than this cannot be hit.
     * @param collisionLayer The index of the collision layer to use, thus describing which objects can be hit by the raycast at all.
     * @param shapeTypes Wether to raycast against static or dynamic shapes, or both.
     * @param ignoreShapeId A single shape ID can be given to be ignored. This can be used, for instance, to filter out the own character controller capsule.
     * @returns A HitResult object or null.
     */
    export function CastRay(start: Vec3, dir: Vec3, distance: number, collisionLayer: number, shapeTypes: ShapeType = ShapeType.Static | ShapeType.Dynamic, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_CastRay(start, dir, distance, collisionLayer, shapeTypes, ignoreShapeId);
    }
    
    /**
     * Sweeps a sphere from a position along a direction for a maximum distance and reports the first shape that was hit along the way.
     * @returns A HitResult object or null.
     */
    export function SweepTestSphere(sphereRadius: number, start: Vec3, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_SweepTestSphere(sphereRadius, start, dir, distance, collisionLayer, ignoreShapeId);
    }

    /**
     * Sweeps a box from a position along a direction for a maximum distance and reports the first shape that was hit along the way.
     * @returns A HitResult object or null.
     */
    export function SweepTestBox(boxExtends: Vec3, start: Transform, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_SweepTestBox(boxExtends, start, dir, distance, collisionLayer, ignoreShapeId);
    }

    /**
     * Sweeps a capsule from a position along a direction for a maximum distance and reports the first shape that was hit along the way.
     * @returns A HitResult object or null.
     */
    export function SweepTestCapsule(capsuleRadius: number, capsuleHeight: number, start: Transform, dir: Vec3, distance: number, collisionLayer: number, ignoreShapeId: number = -1): HitResult {
        return __CPP_Physics_SweepTestCapsule(capsuleRadius, capsuleHeight, start, dir, distance, collisionLayer, ignoreShapeId);
    }

    /**
     * Checks whether any shape overlaps with the given sphere.
     */
    export function OverlapTestSphere(sphereRadius: number, position: Vec3, collisionLayer: number, ignoreShapeId: number = -1): boolean {
        return __CPP_Physics_OverlapTestSphere(sphereRadius, position, collisionLayer, ignoreShapeId);
    }

    /**
     * Checks whether any shape overlaps with the given capsule.
     */
    export function OverlapTestCapsule(capsuleRadius: number, capsuleHeight: number, transform: Transform, collisionLayer: number, ignoreShapeId: number = -1): boolean {
        return __CPP_Physics_OverlapTestCapsule(capsuleRadius, capsuleHeight, transform, collisionLayer, ignoreShapeId);
    }

    /**
     * Returns the currently set gravity.
     */
    export function GetGravity(): Vec3 {
        return __CPP_Physics_GetGravity();
    }

    /**
     * Reports all dynamic shapes found within the given sphere, using a callback function.
     */
    export function QueryDynamicShapesInSphere(radius: number, position: Vec3, collisionLayer: number, callback: QueryShapeCallback, ignoreShapeId: number = -1): void {
        __CPP_Physics_QueryDynamicShapesInSphere(radius, position, collisionLayer, ignoreShapeId, callback);
    }
}
