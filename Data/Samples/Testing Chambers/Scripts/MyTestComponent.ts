import ez = require("./../TypeScript/ez")

export class MyTestComponent extends ez.TypescriptComponent {
    constructor() {
        super()
        ez.Log.Info("Construct MyTestComponent")
    }

    OnMsgSetColor(msg: ez.MsgSetColor): void {
        ez.Log.Info("MsgSetColor: " + msg.Color.r + ", " + msg.Color.g + ", " + msg.Color.b + ", " + msg.Color.a);
    }

    OnMsgSetFloatParameter(msg: ez.MsgSetFloatParameter): void {
        ez.Log.Info("MsgSetFloatParameter: " + msg.Value + " / " + msg.TypeNameHash);
    }

    static RegisterMessageHandlers() {
        ez.Log.Info("RegisterMessageHandlers")

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgSetColor, "OnMsgSetColor");
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgSetFloatParameter, "OnMsgSetFloatParameter");
    }

    Update(): void {
        //ez.Log.Info("MyTestComponent::Update")

        let owner = this.GetOwner();

        if (owner != null) {
            //ez.Log.Dev("Setting Local Position")

            let newPos = ez.Vec3.CreateRandomPointInSphere();
            newPos.MulNumber(0.1)
            newPos.AddVec3(owner.GetLocalPosition())
            owner.SetLocalPosition(newPos);

            let newDir = ez.Vec3.CreateRandomDirection();
            let newRot = new ez.Quat();
            newRot.SetShortestRotation(new ez.Vec3(1, 0, 0), newDir);

            //owner.SetLocalRotation(newRot);
        }

        if (false) {
            let child = owner.FindChildByName("Light");
            if (child != null) {
                child.SetActive(false);//Math.random() > 0.7);
            }
        }

        let comp = owner.TryGetComponentOfBaseType(ez.TransformComponent);
        if (comp != null) {
            //comp.SetActive(Math.random() > 0.5);
            if (Math.random() > 0.9) {
                if (comp.IsDirectionForwards())
                    comp.SetDirectionForwards(false);
                else
                    comp.SetDirectionForwards(true);
                //comp.ToggleDirection();
            }
            //comp.SetDirectionForwards(Math.random() > 0.9);
        }

        if (false && Math.random() > 0.9) {
            let setMat = new ez.MsgSetMeshMaterial();

            if (Math.random() > 0.5) {
                setMat.Material = "{ eed69dde-aab0-4ccd-8f98-a9822a389ea0 }";
            }
            else {
                setMat.Material = "{ 49324140-a093-4a75-9c6c-efde65a39fc4 }";
            }
            owner.SendMessage(setMat);
        }

        if (true && Math.random() > 0.9) {
            let setMat = new ez.MsgSetColor();

            if (Math.random() > 0.5) {
                setMat.Color.SetHotPink();
                ez.Log.Info("Color: " + setMat.Color.r + ", " + setMat.Color.g + ", " + setMat.Color.b + ", " + setMat.Color.a)
            }
            else {
                setMat.Color.SetBlueViolet();
                ez.Log.Info("Color: " + setMat.Color.r + ", " + setMat.Color.g + ", " + setMat.Color.b + ", " + setMat.Color.a)
            }
            owner.SendMessage(setMat);

            let fMsg = new ez.MsgSetFloatParameter();
            fMsg.Value = 23;
            owner.SendMessage(fMsg);

            --this.deleteCounter;

            if (false && this.deleteCounter == 0) {
                if (this.child == null) {
                    ez.Log.Info("Creating Object");

                    //ez.World.DeleteObjectDelayed(owner);
                    let desc = new ez.GameObjectDesc();
                    desc.Name = "From Script";
                    desc.Parent = owner

                    this.child = ez.World.CreateObject(desc);

                    let light = ez.World.CreateComponent(this.child, ez.PointLightComponent);
                    light.Intensity = 500
                    light.Range = 5
                    light.LightColor = ez.Color.RoyalBlue()

                    let spawnComp1 = owner.TryGetComponentOfBaseType(ez.SpawnComponent);

                    if (spawnComp1 != null) {
                        spawnComp1.TriggerManualSpawn();
                    }

                    let spawnObj = ez.World.TryGetObjectWithGlobalKey("Spawn");
                    if (spawnObj != null) {
                        let spawnComp = spawnObj.TryGetComponentOfBaseType(ez.SpawnComponent);
                        if (spawnComp != null) {
                            spawnComp.TriggerManualSpawn();
                        }
                    }
                }
                else {
                    ez.World.DeleteObjectDelayed(this.child)
                    this.child = null
                }

                this.deleteCounter = 3
            }
        }
    }

    private deleteCounter = 3;
    private child: ez.GameObject
}
