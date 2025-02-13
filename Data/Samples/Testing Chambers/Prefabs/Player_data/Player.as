#include "../../Scripting/GameDecls.as"
#include "../../Prefabs/Guns/Weapon.as"

class Player : ezAngelScriptClass
{
    bool GiveAllWeapons = false;
    bool Invincible = false;

    private ezGameObjectHandle hCameraObj;
    private ezComponentHandle hCharacterComp;
    private ezComponentHandle hInputComp;
    private ezComponentHandle hGrabComp;
    private ezGameObjectHandle hFlashlightObj;
    private ezGameObjectHandle hDamageIndicatorObj;

    private WeaponInfo weaponInfoNone;
    private WeaponInfo weaponInfoPistol;
    private WeaponInfo weaponInfoShotgun;
    private WeaponInfo weaponInfoMachineGun;
    private WeaponInfo weaponInfoPlasmaRifle;
    private WeaponInfo weaponInfoRocketLauncher;

    private int iPlayerHealth = 100;
    private float fDamageIndicatorValue = 0;
    private bool bRequireNoShoot = false;
    private WeaponType eActiveWeapon = WeaponType::None;
    private WeaponType eHolsteredWeapon = WeaponType::None;
    private AmmoPouch ammoPouch;

    WeaponInfo& GetWeaponInfo(WeaponType type)
    {
        switch(type)
        {
        case WeaponType::None:
            return weaponInfoNone;
        case WeaponType::Pistol:
            return weaponInfoPistol;
        case WeaponType::Shotgun:
            return weaponInfoShotgun;
        case WeaponType::MachineGun:
            return weaponInfoMachineGun;
        case WeaponType::PlasmaRifle:
            return weaponInfoPlasmaRifle;
        case WeaponType::RocketLauncher:
            return weaponInfoRocketLauncher;
        }

        return weaponInfoNone;
    }

    void OnSimulationStarted()
    {
        auto owner = GetOwner();

        hCameraObj = owner.FindChildByName("Camera", true).GetHandle();
        hFlashlightObj = owner.FindChildByName("Flashlight", true).GetHandle();
        hDamageIndicatorObj = owner.FindChildByName("DamageIndicator").GetHandle();
        ezGameObject@ grabObj = owner.FindChildByName("GrabObject", true);

        weaponInfoPistol.hObject = owner.FindChildByName("Pistol", true).GetHandle();
        weaponInfoShotgun.hObject = owner.FindChildByName("Shotgun", true).GetHandle();
        weaponInfoMachineGun.hObject = owner.FindChildByName("MachineGun", true).GetHandle();
        weaponInfoPlasmaRifle.hObject = owner.FindChildByName("PlasmaRifle", true).GetHandle();
        weaponInfoRocketLauncher.hObject = owner.FindChildByName("RocketLauncher", true).GetHandle();

        weaponInfoPistol.eAmmoType = ConsumableType::Ammo_None;
        weaponInfoShotgun.eAmmoType = ConsumableType::Ammo_Shotgun;
        weaponInfoMachineGun.eAmmoType = ConsumableType::Ammo_MachineGun;
        weaponInfoPlasmaRifle.eAmmoType = ConsumableType::Ammo_Plasma;
        weaponInfoRocketLauncher.eAmmoType = ConsumableType::Ammo_Rocket;

        weaponInfoPistol.iClipSize = 8;
        weaponInfoShotgun.iClipSize = 8;
        weaponInfoMachineGun.iClipSize = 30;
        weaponInfoPlasmaRifle.iClipSize = 30;
        weaponInfoRocketLauncher.iClipSize = 3;

        if (GiveAllWeapons)
        {
            weaponInfoPistol.bUnlocked = true;
            weaponInfoShotgun.bUnlocked = true;
            weaponInfoMachineGun.bUnlocked = true;
            weaponInfoPlasmaRifle.bUnlocked = true;
            weaponInfoRocketLauncher.bUnlocked = true;

            ammoPouch.AmmoMachineGun = 9999;
            ammoPouch.AmmoPistol = 9999;
            ammoPouch.AmmoPlasmaRifle = 9999;
            ammoPouch.AmmoRocketLauncher = 9999;
            ammoPouch.AmmoShotgun = 9999;
        }

        ezJoltDefaultCharacterComponent@ characterComp;
        if (owner.TryGetComponentOfBaseType(@characterComp))
        {
            hCharacterComp = characterComp.GetHandle();
        }

        ezInputComponent@ inputComp;
        if (owner.TryGetComponentOfBaseType(@inputComp))
        {
            hInputComp = inputComp.GetHandle();
        }

        if (@grabObj != null)
        {
            ezJoltGrabObjectComponent@ grabComp;
            if (grabObj.TryGetComponentOfBaseType(@grabComp))
            {
                hGrabComp = grabComp.GetHandle();
            }
        }
    }

    void Update()
    {
        ezGameObject@ cameraObj;
        if (!GetWorld().TryGetObject(hCameraObj, @cameraObj))
            return;

        ezJoltDefaultCharacterComponent@ characterComp;
        if (!GetWorld().TryGetComponent(hCharacterComp, @characterComp))
            return;

        ezInputComponent@ inputComp;
        if (!GetWorld().TryGetComponent(hInputComp, @inputComp))
            return;

        if (!weaponInfoNone.bUnlocked)
        {
            weaponInfoNone.bUnlocked = true;
            weaponInfoPistol.bUnlocked = true;

            // weapon to start with
            SwitchToWeapon(WeaponType::Pistol);
        }

        
        if (iPlayerHealth > 0)
        {
            ezStringBuilder text;
            text.SetFormat("Health: {}", ezMath::Ceil(iPlayerHealth));
            ezDebug::DrawInfoText(GetWorld(), text, ezDebugTextPlacement::TopLeft, "Player", ezColor::White);
            
            if (eActiveWeapon != WeaponType::None)
            {
                WeaponInfo@ weaponInfo = GetWeaponInfo(eActiveWeapon);

                if (weaponInfo.eAmmoType == ConsumableType::Ammo_None)
                {
                    text.SetFormat("Ammo: {}", weaponInfo.iAmmoInClip);
                    ezDebug::DrawInfoText(GetWorld(), text, ezDebugTextPlacement::TopLeft, "Player", ezColor::White);
                }
                else
                {
                    const int ammoOfType = ammoPouch.getAmmoType(weaponInfo.eAmmoType);
                    text.SetFormat("Ammo: {} / {}", weaponInfo.iAmmoInClip, ammoOfType);
                    ezDebug::DrawInfoText(GetWorld(), text, ezDebugTextPlacement::TopLeft, "Player", ezColor::White);
                }
    
                ezGameObject@ weaponObj;
                if (GetWorld().TryGetObject(weaponInfo.hObject, @weaponObj))
                {
                    MsgWeaponInteraction msgInteract;
                    @msgInteract.ammoPouch = @ammoPouch;
                    @msgInteract.weaponInfo = @weaponInfo;
                    msgInteract.interaction = WeaponInteraction::Update;

                    weaponObj.SendMessageRecursive(msgInteract);
                }

                // TODO: gunComp[activeWeapon].RenderCrosshair();
            }            

            // character controller update
            {
                ezMsgMoveCharacterController msgMove;
                msgMove.Jump = inputComp.GetCurrentInputState("Jump", true) > 0.5;
                msgMove.MoveForwards = inputComp.GetCurrentInputState("MoveForwards", false);
                msgMove.MoveBackwards = inputComp.GetCurrentInputState("MoveBackwards", false);
                msgMove.StrafeLeft = inputComp.GetCurrentInputState("StrafeLeft", false);
                msgMove.StrafeRight = inputComp.GetCurrentInputState("StrafeRight", false);
                msgMove.RotateLeft = inputComp.GetCurrentInputState("RotateLeft", false);
                msgMove.RotateRight = inputComp.GetCurrentInputState("RotateRight", false);
                msgMove.Run = inputComp.GetCurrentInputState("Run", false) > 0.5;
                msgMove.Crouch = inputComp.GetCurrentInputState("Crouch", false) > 0.5;

                GetOwner().SendMessageRecursive(msgMove);
                
                // look up / down
                ezHeadBoneComponent@ headBoneComp;
                if (cameraObj.TryGetComponentOfBaseType(@headBoneComp))
                {
                    float up = inputComp.GetCurrentInputState("LookUp", false);
                    float down = inputComp.GetCurrentInputState("LookDown", false);
                    
                    headBoneComp.ChangeVerticalRotation(down - up);
                }

                ezBlackboardComponent@ blackboardComp;
                if (GetOwner().TryGetComponentOfBaseType(@blackboardComp))
                {
                    // this is used to control the animation playback on the 'shadow proxy' mesh
                    // currently we only sync basic movement
                    // also note that the character mesh currently doesn't have crouch animations
                    // so we can't have a proper shadow there

                    blackboardComp.SetEntryValue("MoveForwards", msgMove.MoveForwards);
                    blackboardComp.SetEntryValue("MoveBackwards", msgMove.MoveBackwards);
                    blackboardComp.SetEntryValue("StrafeLeft", msgMove.StrafeLeft);
                    blackboardComp.SetEntryValue("StrafeRight", msgMove.StrafeRight);
                    blackboardComp.SetEntryValue("TouchingGround", characterComp.IsStandingOnGround());
                }
            }

            // reduce damage indicator value over time
            fDamageIndicatorValue = ezMath::Max(fDamageIndicatorValue - GetWorld().GetClock().GetTimeDiff().AsFloatInSeconds(), 0);
        }
        else
        {
            fDamageIndicatorValue = 3;
        }

        if (!hDamageIndicatorObj.IsInvalidated())
        {
            ezMsgSetColor msg;
            msg.Color = ezColor(1, 1, 1, fDamageIndicatorValue);

            GetWorld().SendMessage(hDamageIndicatorObj, msg);
        }
    }

    void OnMsgInputActionTriggered(ezMsgInputActionTriggered@ msg)
    {
        if (iPlayerHealth <= 0)
            return;

        if (msg.TriggerState == ezTriggerState::Activated)
        {
            ezJoltGrabObjectComponent@ grabComp;
            if (!GetWorld().TryGetComponent(hGrabComp, @grabComp))
                return;

            if (!grabComp.HasObjectGrabbed()) 
            {
                if (msg.InputAction == "SwitchWeapon0")
                    SwitchToWeapon(WeaponType::None);

                if (msg.InputAction == "SwitchWeapon1")
                    SwitchToWeapon(WeaponType::Pistol);

                if (msg.InputAction == "SwitchWeapon2")
                    SwitchToWeapon(WeaponType::Shotgun);

                if (msg.InputAction == "SwitchWeapon3")
                    SwitchToWeapon(WeaponType::MachineGun);

                if (msg.InputAction == "SwitchWeapon4")
                    SwitchToWeapon(WeaponType::PlasmaRifle);

                if (msg.InputAction == "SwitchWeapon5")
                    SwitchToWeapon(WeaponType::RocketLauncher);
            }

            if (msg.InputAction == "Flashlight")
            {
                ezGameObject@ flashlightObj;
                if (GetWorld().TryGetObject(hFlashlightObj, @flashlightObj))
                {
                    ezSpotLightComponent@ flashLightComp;
                    if (flashlightObj.TryGetComponentOfBaseType(@flashLightComp))
                    {
                        flashLightComp.Active = !flashLightComp.Active;
                    }
                }
            }

            if (msg.InputAction == "Use")
            {
                ezGameObject@ cameraObj;
                if (!GetWorld().TryGetObject(hCameraObj, @cameraObj))
                    return;
        
                if (grabComp.HasObjectGrabbed())
                {
                    grabComp.DropGrabbedObject();
                    SwitchToWeapon(eHolsteredWeapon);
                }
                else if (grabComp.GrabNearbyObject())
                {
                    eHolsteredWeapon = eActiveWeapon;
                    SwitchToWeapon(WeaponType::None);
                }
                else
                {
                    ezVec3 vHitPosition;
                    ezVec3 vHitNormal;
                    ezGameObjectHandle hHitObject;

                    if (ezPhysics::Raycast(vHitPosition, vHitNormal, hHitObject, GetWorld(), cameraObj.GetGlobalPosition(), cameraObj.GetGlobalDirForwards(), 2.0, ezPhysics::GetCollisionLayerByName(GetWorld(), "Interaction Raycast")))
                    {
                        ezMsgGenericEvent msgUse;
                        msgUse.Message = "Use";

                        // TODO: add ezWorld::SendEventMessage
                        ezGameObject@ hitObj;
                        if (GetWorld().TryGetObject(hHitObject, @hitObj))
                        {
                            hitObj.SendEventMessage(msgUse, GetOwnerComponent());
                        }
                    }
                }
            }

            if (eActiveWeapon != WeaponType::None && msg.InputAction == "Reload")
            {
                WeaponInfo@ weaponInfo = GetWeaponInfo(eActiveWeapon);

                ezGameObject@ weaponObj;
                if (GetWorld().TryGetObject(weaponInfo.hObject, @weaponObj))
                {
                    MsgWeaponInteraction msgInteract;
                    msgInteract.keyState = msg.TriggerState;
                    @msgInteract.ammoPouch = @ammoPouch;
                    @msgInteract.weaponInfo = @weaponInfo;
                    msgInteract.interaction = WeaponInteraction::Reload;

                    weaponObj.SendMessageRecursive(msgInteract);
                }
            }

            if (msg.InputAction == "Teleport")
            {
                ezJoltDefaultCharacterComponent@ characterComp;
                if (GetWorld().TryGetComponent(hCharacterComp, @characterComp))
                {
                    ezVec3 pos = GetOwner().GetGlobalPosition();
                    ezVec3 dir = GetOwner().GetGlobalDirForwards();
                    dir.z = 0;
                    pos += dir.GetNormalized() * 5.0f;

                    characterComp.TeleportCharacter(pos);
                }
            }
        }

        if (msg.InputAction == "Shoot")
        {
            if (bRequireNoShoot)
            {
                if (msg.TriggerState == ezTriggerState::Activated)
                {
                    bRequireNoShoot = false;
                }
            }

            if (!bRequireNoShoot)
            {
                ezJoltGrabObjectComponent@ grabComp;
                if (!GetWorld().TryGetComponent(hGrabComp, @grabComp))
                    return;

                if (grabComp.HasObjectGrabbed())
                {
                    ezVec3 dir(0.75, 0, 0);
                    dir *= 30;

                    grabComp.ThrowGrabbedObject(dir);

                    SwitchToWeapon(eHolsteredWeapon);
                }
                else
                {
                    WeaponInfo@ weaponInfo = GetWeaponInfo(eActiveWeapon);

                    ezGameObject@ weaponObj;
                    if (GetWorld().TryGetObject(weaponInfo.hObject, @weaponObj))
                    {
                        MsgWeaponInteraction msgInteract;
                        msgInteract.keyState = msg.TriggerState;
                        @msgInteract.ammoPouch = @ammoPouch;
                        @msgInteract.weaponInfo = @weaponInfo;
                        msgInteract.interaction = WeaponInteraction::Fire;

                        weaponObj.SendMessageRecursive(msgInteract);
                    }
                }
            }
        }
    }

    void OnMsgMsgDamage(ezMsgDamage@ msg)
    {
        if (Invincible)
            return;

        if (iPlayerHealth <= 0)
            return;

        iPlayerHealth -= int(msg.Damage * 2);

        fDamageIndicatorValue = ezMath::Min(fDamageIndicatorValue + msg.Damage * 0.2f, 2.0f);
        
		if (iPlayerHealth <= 0)
        {
            ezLog::Warning("Player died.");

            ezJoltDefaultCharacterComponent@ characterComp;
            if (GetWorld().TryGetComponent(hCharacterComp, @characterComp))
            {
                // deactivate the character controller, so that it isn't in the way
                characterComp.Active = false;
            }

            auto owner = GetOwner();
            auto cameraObj = owner.FindChildByName("Camera");
            auto camPos = cameraObj.GetGlobalPosition();

            ezGameObjectDesc go;
            go.m_LocalPosition = cameraObj.GetGlobalPosition();
            go.m_bDynamic = true;

            ezGameObject@ rbCam;
            GetWorld().CreateObject(go, rbCam);
            rbCam.UpdateGlobalTransform();

            ezJoltDynamicActorComponent@ rbCamActor;
            rbCam.CreateComponent(@rbCamActor);

            ezJoltShapeSphereComponent@ rbCamSphere;
            rbCam.CreateComponent(@rbCamSphere);
            rbCamSphere.Radius = 0.3;

            ezPointLightComponent@ rbCamLight;
            rbCam.CreateComponent(@rbCamLight);
            rbCamLight.LightColor = ezColor::DarkRed;
            rbCamLight.Intensity = 200;

            rbCamActor.Mass = 30;
            rbCamActor.LinearDamping = 0.7;
            rbCamActor.AngularDamping = 0.9;
            rbCamActor.CollisionLayer = ezPhysics::GetCollisionLayerByName(GetWorld(), "Default");
            rbCamActor.AddAngularForce(10 * ezVec3::MakeRandomPointInSphere(GetWorld().GetRandomNumberGenerator()));

            cameraObj.SetParent(rbCam.GetHandle());
         }
    }

    void SwitchToWeapon(WeaponType weapon)
    {
        if (eActiveWeapon == weapon)
            return;

        auto infoNew = GetWeaponInfo(weapon);

        if (!infoNew.bUnlocked)
            return;

        auto infoOld = GetWeaponInfo(eActiveWeapon);

        bRequireNoShoot = true;

        ezGameObject@ oldObj;
        if (GetWorld().TryGetObject(infoOld.hObject, oldObj))
        {
            MsgWeaponInteraction msg;
            msg.interaction = WeaponInteraction::HolsterWeapon;
            oldObj.SendMessage(msg);
        }

        ezGameObject@ newObj;
        if (GetWorld().TryGetObject(infoNew.hObject, newObj))
        {
            MsgWeaponInteraction msg;
            msg.interaction = WeaponInteraction::DrawWeapon;
            newObj.SendMessage(msg);
        }

         eActiveWeapon = weapon;
    }

    void OnMsgUnlockWeapon(MsgUnlockWeapon@ msg)
    {
        msg.return_consumed = true;

        WeaponInfo@ wi = GetWeaponInfo(msg.weaponType);

        if (wi.bUnlocked == false)
        {
            wi.bUnlocked = true;
            SwitchToWeapon(msg.weaponType);
        }
    }    

    void OnMsgPhysicsJointBroke(ezMsgPhysicsJointBroke@ msg)
    {
        // must be the 'object grabber' joint
        SwitchToWeapon(eHolsteredWeapon);
    }

    int GetMaxConsumableAmount(ConsumableType type) const
    {
        switch (type)
        {
        case ConsumableType::Health:
            return 100;
        case ConsumableType::Ammo_Pistol:
            return 50;
        case ConsumableType::Ammo_Shotgun:
            return 40;
        case ConsumableType::Ammo_MachineGun:
            return 150;
        case ConsumableType::Ammo_Plasma:
            return 100;
        case ConsumableType::Ammo_Rocket:
            return 20;
        }
    
        // TODO: assert
        return 0;
    }

    void OnMsgAddConsumable(MsgAddConsumable@ msg)
    {
        const int maxAmount = GetMaxConsumableAmount(msg.consumableType);

        if (msg.consumableType == ConsumableType::Health)
        {
            if (iPlayerHealth <= 0 || iPlayerHealth >= maxAmount)
                return;

            msg.return_consumed = true;

            iPlayerHealth = ezMath::Clamp(iPlayerHealth + msg.amount, 1, maxAmount);
            return;
        }

        if (msg.consumableType > ConsumableType::AmmoTypes_Start && msg.consumableType < ConsumableType::AmmoTypes_End)
        {
            const int curAmount = ammoPouch.getAmmoType(msg.consumableType);

            if (curAmount >= maxAmount)
                return;

            msg.return_consumed = true;

            const int newAmount = curAmount + msg.amount;

            ammoPouch.getAmmoType(msg.consumableType) = ezMath::Clamp(newAmount, 0, maxAmount);
        }
    }    
}