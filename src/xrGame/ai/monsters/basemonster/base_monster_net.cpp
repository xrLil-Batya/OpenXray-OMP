#include "StdAfx.h"
#include "base_monster.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrAICore/Navigation/game_graph.h"
#include "ai_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "CharacterPhysicsSupport.h"

void CBaseMonster::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    m_pPhysics_support->in_NetSave(P);
}

bool CBaseMonster::net_SaveRelevant() { return (inherited::net_SaveRelevant() || BOOL(PPhysicsShell() != NULL)); }
void CBaseMonster::net_Export(NET_Packet& P)
{
    R_ASSERT(Local());
    if (IsGameTypeSingle())
    {
        // export last known packet
        R_ASSERT(!NET.empty());
        net_update& N = NET.back();
        P.w_float(GetfHealth());
        P.w_u32(N.dwTimeStamp);
        P.w_u8(0);
        P.w_vec3(N.p_pos);
        P.w_float /*w_angle8*/ (N.o_model);
        P.w_float /*w_angle8*/ (N.o_torso.yaw);
        P.w_float /*w_angle8*/ (N.o_torso.pitch);
        P.w_float /*w_angle8*/ (N.o_torso.roll);
        P.w_u8(u8(g_Team()));
        P.w_u8(u8(g_Squad()));
        P.w_u8(u8(g_Group()));

        GameGraph::_GRAPH_ID l_game_vertex_id = ai_location().game_vertex_id();
        P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
        P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
        //	P.w						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
        //	P.w						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
        float f1 = 0;
        if (ai().game_graph().valid_vertex_id(l_game_vertex_id))
        {
            f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
            P.w(&f1, sizeof(f1));
            f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
            P.w(&f1, sizeof(f1));
        }
        else
        {
            P.w(&f1, sizeof(f1));
            P.w(&f1, sizeof(f1));
        }
    }
    else
    {
        P.w_vec3(Position());
        P.w_float(GetfHealth());

        P.w_angle8(movement().m_body.current.pitch);
        P.w_angle8(movement().m_body.current.roll);
        P.w_angle8(movement().m_body.current.yaw);

        IKinematicsAnimated* ik_anim_obj = smart_cast<IKinematicsAnimated*>(Visual());
        P.w_u16(ik_anim_obj->ID_Cycle_Safe(m_anim_base->cur_anim_info().name).idx);
        P.w_u8(ik_anim_obj->ID_Cycle_Safe(m_anim_base->cur_anim_info().name).slot);
    }
}

u16 u_last_motion_slot;
u8 u_last_motion_idx;

void CBaseMonster::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote());
    if (IsGameTypeSingle())
    {
        net_update N;

        u8 flags;

        float health;
        P.r_float(health);
        SetfHealth(health);

        P.r_u32(N.dwTimeStamp);
        P.r_u8(flags);
        P.r_vec3(N.p_pos);
        P.r_float /*r_angle8*/ (N.o_model);
        P.r_float /*r_angle8*/ (N.o_torso.yaw);
        P.r_float /*r_angle8*/ (N.o_torso.pitch);
        P.r_float /*r_angle8*/ (N.o_torso.roll);
        id_Team = P.r_u8();
        id_Squad = P.r_u8();
        id_Group = P.r_u8();

        GameGraph::_GRAPH_ID l_game_vertex_id = ai_location().game_vertex_id();
        P.r(&l_game_vertex_id, sizeof(l_game_vertex_id));
        P.r(&l_game_vertex_id, sizeof(l_game_vertex_id));

        if (NET.empty() || (NET.back().dwTimeStamp < N.dwTimeStamp))
        {
            NET.push_back(N);
            NET_WasInterpolating = TRUE;
        }

        //	P.r						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
        //	P.r						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
        float f1 = 0;
        if (ai().game_graph().valid_vertex_id(l_game_vertex_id))
        {
            f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
            P.r(&f1, sizeof(f1));
            f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
            P.r(&f1, sizeof(f1));
        }
        else
        {
            P.r(&f1, sizeof(f1));
            P.r(&f1, sizeof(f1));
        }

        setVisible(TRUE);
        setEnabled(TRUE);
    }
    else
    {
        SRotation fv_direction;
        Fvector fv_position;
        float f_health;
        u16 u_motion_idx;
        u8 u_motion_slot;

        P.r_vec3(fv_position);

        P.r_float(f_health);

        P.r_angle8(fv_direction.pitch);
        P.r_angle8(fv_direction.roll);
        P.r_angle8(fv_direction.yaw);

        P.r_u16(u_motion_idx);
        P.r_u8(u_motion_slot);

        SetfHealth(f_health);

        //Position().set(fv_position);
        //XFORM().rotateY(fv_direction.yaw);
        
        //movement().m_body.current.pitch = fv_direction.pitch;
        //movement().m_body.current.roll = fv_direction.roll;
        //movement().m_body.current.yaw = fv_direction.yaw;
        
        Fmatrix M;
 
        XFORM().mulB_43(M);
        XFORM().rotateY(fv_direction.yaw);

        XFORM().translate_over(fv_position);
        Position().set(fv_position); // we need it?         

        SPHNetState State;
        State.position = fv_position;
        State.previous_position = fv_position;

        CPHSynchronize* pSyncObj = NULL;

        pSyncObj = PHGetSyncItem(0);

        if (pSyncObj)
        {
            pSyncObj->set_State(State);
        }

        MotionID motion;
        IKinematicsAnimated* ik_anim_obj = smart_cast<IKinematicsAnimated*>(Visual());
        if (u_last_motion_idx != u_motion_idx || u_last_motion_slot != u_motion_slot)
        {
            u_last_motion_idx = u_motion_idx;
            u_last_motion_slot = u_motion_slot;
            motion.idx = u_motion_idx;
            motion.slot = u_motion_slot;
            if (motion.valid())
            {
                CStepManager::on_animation_start(motion,
                    ik_anim_obj->LL_PlayCycle(ik_anim_obj->LL_GetMotionDef(motion)->bone_or_part, motion, TRUE,
                        ik_anim_obj->LL_GetMotionDef(motion)->Accrue(), ik_anim_obj->LL_GetMotionDef(motion)->Falloff(),
                        ik_anim_obj->LL_GetMotionDef(motion)->Speed(), FALSE, 0, 0, 0));
            }
        }

        setVisible(TRUE);
        setEnabled(TRUE);
    }
}
