#include "stdafx.h"
#include "pch_script.h"

#include "ai/stalker/ai_stalker.h"

//#include "stalker_animation_manager.h"
//#include "stalker_movement_manager_smart_cover.h"

//#include "CharacterPhysicsSupport.h"

void CAI_Stalker::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    m_pPhysics_support->in_NetSave(P);
}

bool CAI_Stalker::net_SaveRelevant() { return (inherited::net_SaveRelevant() || (PPhysicsShell() != NULL)); }
void CAI_Stalker::net_Export(NET_Packet& P)
{
    R_ASSERT(Local());

     
    // Position
    P.w_vec3(Position());
     

    // health
    P.w_float(GetfHealth());

    // agnles

    P.w_angle8(angle_normalize(movement().m_body.current.pitch));
    // P.w_angle8(movement().m_body.current.roll);
    P.w_angle8(angle_normalize(movement().m_body.current.yaw));
    P.w_angle8(angle_normalize(movement().m_head.current.pitch));
    P.w_angle8(angle_normalize(movement().m_head.current.yaw));

    // inventory
    CWeapon* weapon = smart_cast<CWeapon*>(inventory().ActiveItem());

    if (weapon && !weapon->strapped_mode())
        P.w_u16(inventory().ActiveItem()->CurrSlot());
    else
        P.w_u16(NO_ACTIVE_SLOT);

    // animation
    P.w_u16(m_animation_manager->torso().animation().idx);
    P.w_u8(m_animation_manager->torso().animation().slot);
    P.w_u16(m_animation_manager->legs().animation().idx);
    P.w_u8(m_animation_manager->legs().animation().slot);
    P.w_u16(m_animation_manager->head().animation().idx);
    P.w_u8(m_animation_manager->head().animation().slot);

    P.w_u16(m_animation_manager->script().animation().idx);
    P.w_u8(m_animation_manager->script().animation().slot);
}

void CAI_Stalker::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote()); 

    SRotation fv_direction;
    SRotation fv_head_orientation;
    Fvector fv_position;

    float f_health;

    u16 u_active_weapon_slot;

    u16 u_torso_motion_idx;
    u16 u_legs_motion_idx;
    u16 u_head_motion_idx;
    u16 u_script_motion_idx;

    u8 u_torso_motion_slot;
    u8 u_legs_motion_slot;
    u8 u_head_motion_slot;
    u8 u_script_motion_slot;

    P.r_vec3(fv_position);

    P.r_float(f_health);

    P.r_angle8(fv_direction.pitch);
    // P.r_angle8(fv_direction.roll);
    P.r_angle8(fv_direction.yaw);
    P.r_angle8(fv_head_orientation.pitch);
    P.r_angle8(fv_head_orientation.yaw);

    P.r_u16(u_active_weapon_slot);

    P.r_u16(u_torso_motion_idx);
    P.r_u8(u_torso_motion_slot);
    P.r_u16(u_legs_motion_idx);
    P.r_u8(u_legs_motion_slot);
    P.r_u16(u_head_motion_idx);
    P.r_u8(u_head_motion_slot);

    P.r_u16(u_script_motion_idx);
    P.r_u8(u_script_motion_slot);

    SetfHealth(f_health);

    inventory().SetActiveSlot(u_active_weapon_slot);

    movement().m_body.current.pitch = fv_direction.pitch;
    movement().m_body.current.yaw = fv_direction.yaw;
    movement().m_head.current.pitch = fv_head_orientation.pitch;
    movement().m_head.current.yaw = fv_head_orientation.yaw;

    Position().set(fv_position);
 
    SPHNetState State;
    State.position = fv_position;
    State.previous_position = fv_position;


    CPHSynchronize* pSyncObj = NULL;

    pSyncObj = PHGetSyncItem(0);

    if (pSyncObj)
    {
        pSyncObj->set_State(State);
    }
         
    

    // Pavel: create structure for animation?
    ApplyAnimation(u_torso_motion_idx, u_torso_motion_slot, 
                    u_legs_motion_idx, u_legs_motion_slot,
                    u_head_motion_idx, u_head_motion_slot, 
                    u_script_motion_idx, u_script_motion_slot);

    setVisible(TRUE);
    setEnabled(TRUE);
}

void CAI_Stalker::ApplyAnimation(u16 u_torso_motion_idx, u8 u_torso_motion_slot, u16 u_legs_motion_idx,
    u8 u_legs_motion_slot, u16 u_head_motion_idx, u8 u_head_motion_slot, u16 u_script_motion_idx,
    u8 u_script_motion_slot)
{
    MotionID motion;
    IKinematicsAnimated* ik_anim_obj = smart_cast<IKinematicsAnimated*>(Visual());

    if (u_last_torso_motion_idx != u_torso_motion_idx)
    {
        u_last_torso_motion_idx = u_torso_motion_idx;
        motion.idx = u_torso_motion_idx;
        motion.slot = u_torso_motion_slot;
        if (motion.valid())
        {
            ik_anim_obj->LL_PlayCycle(ik_anim_obj->LL_PartID("torso"), motion, TRUE,
                ik_anim_obj->LL_GetMotionDef(motion)->Accrue(), ik_anim_obj->LL_GetMotionDef(motion)->Falloff(),
                ik_anim_obj->LL_GetMotionDef(motion)->Speed(), FALSE, 0, 0, 0);
        }
    }
    if (u_last_legs_motion_idx != u_legs_motion_idx)
    {
        u_last_legs_motion_idx = u_legs_motion_idx;
        motion.idx = u_legs_motion_idx;
        motion.slot = u_legs_motion_slot;
        if (motion.valid())
        {
            CStepManager::on_animation_start(motion,
                ik_anim_obj->LL_PlayCycle(ik_anim_obj->LL_PartID("legs"), motion, TRUE,
                    ik_anim_obj->LL_GetMotionDef(motion)->Accrue(), ik_anim_obj->LL_GetMotionDef(motion)->Falloff(),
                    ik_anim_obj->LL_GetMotionDef(motion)->Speed(), FALSE, 0, 0, 0)

            );
        }
    }
    if (u_last_head_motion_idx != u_head_motion_idx)
    {
        u_last_head_motion_idx = u_head_motion_idx;
        motion.idx = u_head_motion_idx;
        motion.slot = u_head_motion_slot;
        if (motion.valid())
        {
            ik_anim_obj->LL_PlayCycle(ik_anim_obj->LL_PartID("head"), motion, TRUE,
                ik_anim_obj->LL_GetMotionDef(motion)->Accrue(), ik_anim_obj->LL_GetMotionDef(motion)->Falloff(),
                ik_anim_obj->LL_GetMotionDef(motion)->Speed(), FALSE, 0, 0, 0);
        }
    }

    if (u_last_script_motion_idx != u_script_motion_idx)
    {
        motion.idx = u_script_motion_idx;
        motion.slot = u_script_motion_slot;
        u_last_script_motion_idx = u_script_motion_idx;
        if (motion.valid())
        {
            ik_anim_obj->LL_PlayCycle(ik_anim_obj->LL_GetMotionDef(motion)->bone_or_part, motion, TRUE,
                ik_anim_obj->LL_GetMotionDef(motion)->Accrue(), ik_anim_obj->LL_GetMotionDef(motion)->Falloff(),
                ik_anim_obj->LL_GetMotionDef(motion)->Speed(), ik_anim_obj->LL_GetMotionDef(motion)->StopAtEnd(), 0, 0,
                0);
        }
    }
}
