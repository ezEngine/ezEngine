import ez = require("TypeScript/ez")

import _ge = require("Scripting/GameEnums")
import _gm = require("Scripting/GameMessages")
import _guns = require("Prefabs/Guns/Gun")

export class Player2 extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    characterControler: ez.CharacterControllerComponent = null;
    camera: ez.GameObject = null;
    input: ez.InputComponent = null;
    headBone: ez.HeadBoneComponent = null;
    gunRoot: ez.GameObject = null;
    flashlightObj: ez.GameObject = null;
    flashlight: ez.SpotLightComponent = null;
    activeWeapon: _ge.Weapon = _ge.Weapon.None;
    guns: ez.GameObject[] = [];
    gunComp: _guns.Gun[] = [];
    interact: ez.PxRaycastInteractComponent = null;
    ammoPouch: _guns.AmmoPouch = new _guns.AmmoPouch();
    weaponUnlocked: boolean[] = [];

    OnSimulationStarted(): void {
        let owner = this.GetOwner();
        this.characterControler = owner.TryGetComponentOfBaseType(ez.CharacterControllerComponent);
        this.camera = owner.FindChildByName("Camera", true);
        this.input = owner.TryGetComponentOfBaseType(ez.InputComponent);
        this.headBone = this.camera.TryGetComponentOfBaseType(ez.HeadBoneComponent);
        this.gunRoot = owner.FindChildByName("Gun", true);
        this.flashlightObj = owner.FindChildByName("Flashlight", true);
        this.flashlight = this.flashlightObj.TryGetComponentOfBaseType(ez.SpotLightComponent);
        this.guns[_ge.Weapon.Pistol] = ez.Utils.FindPrefabRootNode(this.gunRoot.FindChildByName("Pistol", true));
        this.guns[_ge.Weapon.Shotgun] = ez.Utils.FindPrefabRootNode(this.gunRoot.FindChildByName("Shotgun", true));
        this.guns[_ge.Weapon.MachineGun] = ez.Utils.FindPrefabRootNode(this.gunRoot.FindChildByName("MachineGun", true));
        this.guns[_ge.Weapon.PlasmaRifle] = ez.Utils.FindPrefabRootNode(this.gunRoot.FindChildByName("PlasmaRifle", true));
        this.guns[_ge.Weapon.RocketLauncher] = ez.Utils.FindPrefabRootNode(this.gunRoot.FindChildByName("RocketLauncher", true));

        this.interact = this.camera.TryGetComponentOfBaseType(ez.PxRaycastInteractComponent);
        this.SetTickInterval(ez.Time.Milliseconds(0));

        this.weaponUnlocked[_ge.Weapon.Pistol] = true;
        //this.weaponUnlocked[_ge.Weapon.PlasmaRifle] = true;
    }

    Tick(): void {

        if (this.gunComp[_ge.Weapon.Pistol] == null) {
            this.gunComp[_ge.Weapon.Pistol] = this.guns[_ge.Weapon.Pistol].TryGetScriptComponent("Pistol");
            this.gunComp[_ge.Weapon.Shotgun] = this.guns[_ge.Weapon.Shotgun].TryGetScriptComponent("Shotgun");
            this.gunComp[_ge.Weapon.MachineGun] = this.guns[_ge.Weapon.MachineGun].TryGetScriptComponent("MachineGun");
            this.gunComp[_ge.Weapon.PlasmaRifle] = this.guns[_ge.Weapon.PlasmaRifle].TryGetScriptComponent("PlasmaRifle");
            this.gunComp[_ge.Weapon.RocketLauncher] = this.guns[_ge.Weapon.RocketLauncher].TryGetScriptComponent("RocketLauncher");

            this.SwitchToWeapon(_ge.Weapon.Pistol);
            return;
        }

        if (this.health > 0) {

            // character controller update
            {
                let msg = new ez.MsgMoveCharacterController();

                msg.Jump = this.input.GetCurrentInputState("Jump", false) > 0.5;
                msg.MoveForwards = this.input.GetCurrentInputState("MoveForwards", false);
                msg.MoveBackwards = this.input.GetCurrentInputState("MoveBackwards", false);
                msg.StrafeLeft = this.input.GetCurrentInputState("StrafeLeft", false);
                msg.StrafeRight = this.input.GetCurrentInputState("StrafeRight", false);
                msg.RotateLeft = this.input.GetCurrentInputState("RotateLeft", false);
                msg.RotateRight = this.input.GetCurrentInputState("RotateRight", false);
                msg.Run = this.input.GetCurrentInputState("Run", false) > 0.5;
                msg.Crouch = this.input.GetCurrentInputState("Crouch", false) > 0.5;

                this.characterControler.SendMessage(msg);
            }

            // look up / down
            {
                let up = this.input.GetCurrentInputState("LookUp", false);
                let down = this.input.GetCurrentInputState("LookDown", false);

                this.headBone.ChangeVerticalRotation(down - up);
            }
        }

        ez.Debug.Draw2DText("Health: " + Math.ceil(this.health), new ez.Vec2(10, 10), ez.Color.White(), 32);

        const ammoInClip = this.gunComp[this.activeWeapon].GetAmmoInClip();

        if (this.gunComp[this.activeWeapon].GetAmmoType() == _ge.Consumable.Ammo_None) {
            ez.Debug.Draw2DText("Ammo: " + ammoInClip, new ez.Vec2(10, 50), ez.Color.White(), 32);
        } else {
            const ammoOfType = this.ammoPouch.ammo[this.gunComp[this.activeWeapon].GetAmmoType()];
            ez.Debug.Draw2DText("Ammo: " + ammoInClip + " / " + ammoOfType, new ez.Vec2(10, 50), ez.Color.White(), 32);
        }

        if (this.activeWeapon != _ge.Weapon.None) {

            this.gunComp[this.activeWeapon].RenderCrosshair();
        }
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgInputActionTriggered, "OnMsgInputActionTriggered");
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgMsgDamage");
        ez.TypescriptComponent.RegisterMessageHandler(_gm.MsgAddConsumable, "OnMsgAddConsumable");
        ez.TypescriptComponent.RegisterMessageHandler(_gm.MsgUnlockWeapon, "OnMsgUnlockWeapon");
    }

    OnMsgInputActionTriggered(msg: ez.MsgInputActionTriggered): void {

        if (this.health <= 0)
            return;

        if (msg.TriggerState == ez.TriggerState.Activated) {

            if (msg.InputActionHash == ez.Utils.StringToHash("Flashlight")) {
                this.flashlight.SetActiveFlag(!this.flashlight.GetActiveFlag());
            }

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon1"))
                this.SwitchToWeapon(_ge.Weapon.Pistol);

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon2"))
                this.SwitchToWeapon(_ge.Weapon.Shotgun);

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon3"))
                this.SwitchToWeapon(_ge.Weapon.MachineGun);

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon4"))
                this.SwitchToWeapon(_ge.Weapon.PlasmaRifle);

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon5"))
                this.SwitchToWeapon(_ge.Weapon.RocketLauncher);

            if (msg.InputActionHash == ez.Utils.StringToHash("Use")) {
                this.interact.ExecuteInteraction();
            }
        }

        if (msg.InputActionHash == ez.Utils.StringToHash("Shoot")) {

            let msgInteract = new _guns.MsgGunInteraction();
            msgInteract.keyState = msg.TriggerState;
            msgInteract.ammoPouch = this.ammoPouch;
            msgInteract.interaction = _guns.GunInteraction.Fire;

            this.guns[this.activeWeapon].SendMessage(msgInteract);
        }

        if (msg.InputActionHash == ez.Utils.StringToHash("Reload")) {

            let msgInteract = new _guns.MsgGunInteraction();
            msgInteract.keyState = msg.TriggerState;
            msgInteract.ammoPouch = this.ammoPouch;
            msgInteract.interaction = _guns.GunInteraction.Reload;

            this.guns[this.activeWeapon].SendMessage(msgInteract);
        }
    }

    health: number = 100;

    OnMsgMsgDamage(msg: ez.MsgDamage): void {

        if (this.health <= 0)
            return;

        this.health -= msg.Damage * 2;

        if (this.health <= 0) {

            ez.Log.Info("Player died.")

            let owner = this.GetOwner();

            let camera = owner.FindChildByName("Camera");

            let camPos = camera.GetGlobalPosition();

            let go = new ez.GameObjectDesc();
            go.LocalPosition = camera.GetGlobalPosition();
            go.Dynamic = true;

            let rbCam = ez.World.CreateObject(go);

            let rbCamActor = ez.World.CreateComponent(rbCam, ez.PxDynamicActorComponent);
            let rbCamSphere = ez.World.CreateComponent(rbCam, ez.PxShapeSphereComponent);
            rbCamSphere.Radius = 0.3;
            rbCamSphere.CollisionLayer = 2; // debris
            let rbCamLight = ez.World.CreateComponent(rbCam, ez.PointLightComponent);
            rbCamLight.LightColor = ez.Color.DarkRed();
            rbCamLight.Intensity = 200;

            rbCamActor.Mass = 30;
            rbCamActor.LinearDamping = 0.5;
            rbCamActor.AngularDamping = 0.99;
            rbCamActor.AddAngularForce(ez.Vec3.CreateRandomPointInSphere());

            camera.SetParent(rbCam);
        }
    }

    OnMsgAddConsumable(msg: _gm.MsgAddConsumable): void {

        const maxAmount = _ge.MaxConsumableAmount[msg.consumableType];

        if (msg.consumableType == _ge.Consumable.Health) {

            if (this.health <= 0 || this.health >= maxAmount) {
                msg.return_consumed = false;
                return;
            }

            msg.return_consumed = true;

            this.health = ez.Utils.Clamp(this.health + msg.amount, 1, 100);

            return;
        }

        if (msg.consumableType > _ge.Consumable.AmmoTypes_Start && msg.consumableType < _ge.Consumable.AmmoTypes_End) {
            const amount = this.ammoPouch.ammo[msg.consumableType] + msg.amount;

            this.ammoPouch.ammo[msg.consumableType] = ez.Utils.Clamp(amount, 0, maxAmount);
        }
    }

    SwitchToWeapon(weapon: _ge.Weapon) {

        if (this.weaponUnlocked[weapon] == undefined || this.weaponUnlocked[weapon] == false)
            return;

        if (this.activeWeapon == weapon)
            return;

        if (this.gunComp[this.activeWeapon])
            this.gunComp[this.activeWeapon].DeselectGun();

        this.activeWeapon = weapon;

        if (this.gunComp[this.activeWeapon])
            this.gunComp[this.activeWeapon].SelectGun();
    }

    OnMsgUnlockWeapon(msg: _gm.MsgUnlockWeapon): void {

        msg.return_consumed = true;

        if (this.weaponUnlocked[msg.WeaponType] == undefined || this.weaponUnlocked[msg.WeaponType] == false) {

            this.weaponUnlocked[msg.WeaponType] = true;
            this.SwitchToWeapon(msg.WeaponType);
        }
    }
}

