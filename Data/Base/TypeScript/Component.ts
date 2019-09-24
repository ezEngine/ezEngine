import ez = require("./ez")

declare function ezInternal_ezTsComponent_GetOwner(component : ezTsComponent) : ezTsGameObject;
declare function ezInternal_ezTsGameObject_SetLocalPosition(go : ezTsGameObject, x : number, y : number, z : number) : void;

class ezTsGameObject
{
    // SetLocalPosition, GetLocalPosition
    // rotation, scaling

    //GetHandle() : ezTsGameObjectHandle { return new ezTsGameObjectHandle(); }

    SetLocalPosition(x : number, y : number, z : number) : void
    {
        ezInternal_ezTsGameObject_SetLocalPosition(this, x, y, z);
    }    
}

function _ezTS_CreateGameObject() : ezTsGameObject
{
    return new ezTsGameObject;
}

abstract class ezTsComponent
{
    abstract Update() : void;

    GetOwner() : ezTsGameObject 
    {
        return ezInternal_ezTsComponent_GetOwner(this);
    }
}

class MyComponent extends ezTsComponent 
{
    constructor(name: string) 
    {
        super()
        ez.Log.Info("Construct MyComponent: " + name)
        this._name = name;
    }

    Update(): void
    {
        ez.Log.Dev("MyComponent::Update: " + this._name)

        var go = this.GetOwner();

        if (go != null)
        {
            go.SetLocalPosition(0, 0, Math.random());
        }
    }

    private _name: string;
}

// called by the runtime
function _Create_MyComponent(name: string): MyComponent
{
    ez.Log.Info("_Create_MyComponent: " + name)
    return new MyComponent(name);
}