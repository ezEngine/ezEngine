import ez = require("../TypeScript/ez")

enum ActiveWeapon {
    Gun1,
    Gun2,
    Gun3
};

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
    gun: ez.GameObject = null;
    flashlight: ez.SpotLightComponent = null;
    activeWeapon: ActiveWeapon = ActiveWeapon.Gun1;
    weapon1: ez.GameObject = null;
    weapon2: ez.GameObject = null;
    weapon3: ez.GameObject = null;
    interact: ez.PxRaycastInteractComponent = null;

    OnSimulationStarted(): void {
        let owner = this.GetOwner();
        this.characterControler = owner.TryGetComponentOfBaseType(ez.CharacterControllerComponent);
        this.camera = owner.FindChildByName("Camera", true);
        this.input = owner.TryGetComponentOfBaseType(ez.InputComponent);
        this.headBone = this.camera.TryGetComponentOfBaseType(ez.HeadBoneComponent);
        this.gun = owner.FindChildByName("Gun", true);
        this.flashlight = this.gun.TryGetComponentOfBaseType(ez.SpotLightComponent);
        this.weapon1 = this.gun.FindChildByName("Weapon1", true);
        this.weapon2 = this.gun.FindChildByName("Weapon2", true);
        this.weapon3 = this.gun.FindChildByName("Weapon3", true);
        this.interact = this.camera.TryGetComponentOfBaseType(ez.PxRaycastInteractComponent);
    }

    Tick(): number {

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

        return ez.Time.Milliseconds(0);
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgInputActionTriggered, "OnMsgInputActionTriggered");
    }

    OnMsgInputActionTriggered(msg: ez.MsgInputActionTriggered): void {

        if (msg.TriggerState == ez.TriggerState.Activated) {

            if (msg.InputActionHash == ez.Utils.StringToHash("Flashlight")) {
                this.flashlight.SetActive(!this.flashlight.IsActive());
            }

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon1")) {
                this.activeWeapon = ActiveWeapon.Gun1;
            }

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon2")) {
                this.activeWeapon = ActiveWeapon.Gun2;
            }

            if (msg.InputActionHash == ez.Utils.StringToHash("SwitchWeapon3")) {
                this.activeWeapon = ActiveWeapon.Gun3;
            }

            if (msg.InputActionHash == ez.Utils.StringToHash("Use")) {
                this.interact.ExecuteInteraction();
            }
        }

        if (msg.InputActionHash == ez.Utils.StringToHash("Shoot")) {

            switch (this.activeWeapon) {
                case ActiveWeapon.Gun1:
                    this.weapon1.TryGetComponentOfBaseType(ez.SpawnComponent).TriggerManualSpawn();
                    break;

                case ActiveWeapon.Gun2:
                    this.weapon2.TryGetComponentOfBaseType(ez.SpawnComponent).TriggerManualSpawn();
                    break;

                case ActiveWeapon.Gun3:
                    this.weapon3.TryGetComponentOfBaseType(ez.SpawnComponent).TriggerManualSpawn();
                    break;
            }

        }

    }
}

