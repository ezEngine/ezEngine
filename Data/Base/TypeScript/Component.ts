abstract class ezTsGameObjectHandle
{
}

namespace ezLog
{
    export declare function Error(text : string) : void;
    export declare function SeriousWarning(text : string) : void;
    export declare function Warning(text : string) : void;
    export declare function Success(text : string) : void;
    export declare function Info(text : string) : void;
    export declare function Dev(text : string) : void;
    export declare function Debug(text : string) : void;
};

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

        ezLog.Info("Construct MyComponent: " + name)
        this._name = name;
    }

    Update(): void
    {
        ezLog.Dev("MyComponent::Update: " + this._name)

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
    ezLog.Info("_Create_MyComponent: " + name)
    return new MyComponent(name);
}