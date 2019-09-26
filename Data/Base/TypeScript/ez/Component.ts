import __GameObject = require("./GameObject")

export import GameObject = __GameObject.GameObject;
export import GameObjectHandle = __GameObject.GameObjectHandle;

declare function __CPP_Component_GetOwner(component : Component) : GameObject;

export abstract class Component
{
    abstract Update() : void;

    GetOwner() : GameObject 
    {
        return __CPP_Component_GetOwner(this);
    }
}
