declare function ezLog_Info(text: string): void;
declare function GameObjectMove(obj: ezGameObject): void;
declare function GetGameObject(): ezGameObject;

/*
class ezGameObject
{
    Update() : void
    {   
        
        ezLog_Info("ezGameObject::Update")
        GameObjectMove(this)
    }
}

let go : ezGameObject = null;

function _CreateGameObject()
{
    return new ezGameObject;
}

function Update() : void
{
    if (go === null)
    {
        go = GetGameObject()
        //go = new ezGameObject()
    }

    go.Update();
}
*/


interface ezTsComponent {
    Update(): void;
    // GetOwner() : ezTsGameObject
    // Activate / Deactivate / SetActive
    // GetWorld
    // GetHandle
    // Initialize / Deinitialize / OnActivate / OnDeactivate / OnSimulationStarted  
}

class MyComponent implements ezTsComponent {
    constructor(name: string) {
        ezLog_Info("Construct MyComponent: " + name)
        this._name = name;
    }

    Update(): void {
        ezLog_Info("MyComponent::Update: " + this._name)
    }

    private _name: string = "test"
}

// call by the runtime
function _Create_MyComponent(name: string): MyComponent {
    ezLog_Info("_Create_MyComponent: " + name)
    return new MyComponent(name);
}