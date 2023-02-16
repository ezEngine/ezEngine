import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("./Component")
export import Component = __Component.Component;
export import TypescriptComponent = __Component.TypescriptComponent;

declare function __CPP_Utils_StringToHash(text: string): number;
declare function __CPP_Utils_FindPrefabRootNode(prefabNode: GameObject): GameObject;
declare function __CPP_Utils_FindPrefabRootScript(prefabNode: GameObject, scriptComponentTypeName: string): any;
/**
 * Common utility functions.
 */
export namespace Utils {

    /**
     * Returns whether f1 and f2 are equal within a given epsilon.
     */
    export function IsNumberEqual(f1: number, f2: number, epsilon: number): boolean { // [tested]
        return f1 >= f2 - epsilon && f1 <= f2 + epsilon;
    }

    /**
     * Checks whether f1 is within epsilon close to zero.
     */
    export function IsNumberZero(f1: number, epsilon: number): boolean { // [tested]
        return f1 >= -epsilon && f1 <= epsilon;
    }

    /**
     * Computes the hash value for a given string.
     */
    export function StringToHash(text: string): number { // [tested]
        return __CPP_Utils_StringToHash(text);
    }

    /**
     * Returns value clamped to the range [low; high]
     */
    export function Clamp(value: number, low: number, high: number) { // [tested]
        return Math.min(Math.max(value, low), high);
    }

    /**
     * Returns value clamped to the range [0; 1]
     */
    export function Saturate(value: number) { // [tested]
        return Math.min(Math.max(value, 0.0), 1.0);
    }

    /**
     * Returns the linear interpolation between f0 and f1. 
     * @param lerpFactor Factor between 0 and 1 that specifies how much to interpolate.
     */
    export function LerpNumbers(f0: number, f1: number, lerpFactor: number): number {
        return f0 + (f1 - f0) * lerpFactor;
    }

    /**
     * Returns the root-node inside a prefab hierarchy, which is typically the node one wants to send messages to to interact with the prefab.
     * 
     * When a prefab is put into the world, there is a node that represents the prefab, which may have a name or global key to find it by,
     * say it is called 'prefab-proxy'. This node typically holds a single ezPrefabReferenceComponent. After prefab instantiation is complete,
     * the content of the prefab asset is attached as children below 'prefab-proxy'. Many prefabs have a single top-level node, but they are allowed
     * to have multiple. To interact with a prefab, it is common for the prefab to have a script at that top-level node. So you either send messages
     * to that, or you query for a specific script component type there.
     * This utility function makes it easier to get to that specific node. If the given 'prefabNode' has exactly one child node (the most common case),
     * it returns that. If it has multiple child nodes, it returns the one with the name 'root'. If 'prefabNode' is null, has no children, or no child with
     * the name 'root' it returns null.
     * 
     * @param prefabNode The object to search for the prefab root node. May be null.
     */
    export function FindPrefabRootNode(prefabNode: GameObject): GameObject { // [tested]

        if (prefabNode == null)
            return null;

        return __CPP_Utils_FindPrefabRootNode(prefabNode);
    }

    /**
     * Similar to ez.Utils.FindPrefabRootNode() but additionally searches for a specific script component.
     * 
     * @param prefabNode The GameObject representing the prefab proxy node under which the prefab was instantiated.
     * @param scriptComponentTypeName The name of the script component class to search for.
     * 
     * @returns If either 'prefabNode' is null, or the script class name is unknown, or the prefab has no top-level node with
     * the requested script component attached, null is returned. Otherwise, the script component object.
     */
    export function FindPrefabRootScript<TYPE extends TypescriptComponent>(prefabNode: GameObject, scriptComponentTypeName: string): TYPE { // [tested]

        if (prefabNode == null)
            return null;

        return __CPP_Utils_FindPrefabRootScript(prefabNode, scriptComponentTypeName);
    }
}
