#include "stdafx.h"
#include "game_cl_freemp.h"
#include "clsid_game.h"
#include "xr_level_controller.h"
#include "UIGameFMP.h"
#include "actor_mp_client.h"

game_cl_freemp::game_cl_freemp() { 
    
    m_game_ui = NULL;

}

game_cl_freemp::~game_cl_freemp()
{


}

CUIGameCustom* game_cl_freemp::createGameUI()
{
    if (GEnv.isDedicatedServer)
        return NULL;

    CLASS_ID clsid = CLSID_GAME_UI_FREEMP;
    m_game_ui = smart_cast<CUIGameFMP*>(NEW_INSTANCE(clsid));
    R_ASSERT(m_game_ui);
    m_game_ui->Load();
    m_game_ui->SetClGame(this);
    return m_game_ui;
}

void game_cl_freemp::SetGameUI(CUIGameCustom* uigame)
{
    inherited::SetGameUI(uigame);
    m_game_ui = smart_cast<CUIGameFMP*>(uigame);
    R_ASSERT(m_game_ui);
}

void game_cl_freemp::TranslateGameMessageScript(u32 msg, NET_Packet& P)
{
    switch (msg)
    {
        // Msg("working");
    case (MP_Surge):
    {
        Msg("M_SCRIPT MP_Surge");

        luabind::functor<void> funct;
        R_ASSERT(GEnv.ScriptEngine->functor("mp_game_cl.StartSurge", funct));
        funct(P, msg);
    }
    break;

    case (MP_SurgeStop):
    {
        Msg("M_SCRIPT MP_SurgeStop");

        luabind::functor<void> funct;
        R_ASSERT(GEnv.ScriptEngine->functor("mp_game_cl.StopSurge", funct));
        funct(P, msg);
    }
    break;

    case (MP_Door):
    {
        u32 id;

        P.r_u32(id);
        Log("[Event] DoorOpen = ", id);

        u8 sound;
        P.r_u8(sound);

        luabind::functor<void> funct;
        R_ASSERT(GEnv.ScriptEngine->functor("ph_door.change_door_state_on_client", funct));
        funct(id, true, sound ? true : false);
    }
    break;

    case (MP_DoorClose):
    {
        u32 id;

        P.r_u32(id);
        Log("[Event] DoorClose = ", id);

        u8 sound;
        P.r_u8(sound);

        luabind::functor<void> funct;
        R_ASSERT(GEnv.ScriptEngine->functor("ph_door.change_door_state_on_client", funct));
        funct(id, false, sound ? true : false);
    }
    break;

    case (MP_Install_Upgrade):
    {
        u16 id;
        shared_str idname;
        P.r_u16(id);
        P.r_stringZ(idname);

        IGameObject* obj = Level().Objects.net_Find(id);

        CInventoryItem* object = smart_cast<CInventoryItem*>(obj);
        Msg("Upgrade Add to table[%d][%s]", id, idname);
        object->add_upgrade(idname, false);
    }
    break;

    case (GEG_WEAPON_REPAIR):
    {
        u16 id;

        P.r_u16(id);

        IGameObject* obj = Level().Objects.net_Find(id);

        CInventoryItem* object = smart_cast<CInventoryItem*>(obj);

        Msg("Upgrade Add to table[%d]", id);
        object->SetCondition(1.0f);
    }
    break;

    case (MP_GIVEINFO):
    {
        shared_str info;
        P.r_stringZ(info);

        Actor()->TransferInfo(info, true);
        Msg("TransferInfo[%s]", info);
    }
    break;

    default: inherited::TranslateGameMessageScript(msg, P);
    }
}

void game_cl_freemp::net_import_state(NET_Packet& P) { inherited::net_import_state(P); }

void game_cl_freemp::net_import_update(NET_Packet& P) { inherited::net_import_update(P); }

void game_cl_freemp::shedule_Update(u32 dt)
{
    if (!local_player)
        return;

    // синхронизация имени и денег игроков для InventoryOwner
    for (auto cl : players)
    {
        game_PlayerState* ps = cl.second;
        if (!ps || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            continue;

        CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
        if (!pActor || !pActor->g_Alive())
            continue;

        // pActor->SetName(ps->getName());

        pActor->cName_set(ps->getName());

        if (local_player->GameID == ps->GameID)
        {
            pActor->set_money((u32)ps->money_for_round, false);
        }

        if (pActor->get_money() < 1000)
        {
        }
    }
}

bool game_cl_freemp::OnKeyboardPress(int key)
{
    if (kJUMP == key)
    {
        bool b_need_to_send_ready = false;

        IGameObject* curr = Level().CurrentControlEntity();
        if (!curr)
            return (false);

        bool is_actor = !!smart_cast<CActor*>(curr);
        bool is_spectator = !!smart_cast<CSpectator*>(curr);

        game_PlayerState* ps = local_player;

        if (is_spectator || (is_actor && ps && ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
        {
            b_need_to_send_ready = true;
        }

        if (b_need_to_send_ready)
        {
            CGameObject* GO = smart_cast<CGameObject*>(curr);
            NET_Packet P;
            GO->u_EventGen(P, GE_GAME_EVENT, GO->ID());
            P.w_u16(GAME_EVENT_PLAYER_READY);
            GO->u_EventSend(P);
            return true;
        }
        else
        {
            return false;
        }
    };

    return inherited::OnKeyboardPress(key);
}

LPCSTR game_cl_freemp::GetGameScore(string32& score_dest)
{
    s32 frags = local_player ? local_player->frags() : 0;
    xr_sprintf(score_dest, "[%d]", frags);
    return score_dest;
}

void game_cl_freemp::OnConnected()
{
    inherited::OnConnected();
    if (m_game_ui)
    {
        R_ASSERT(!GEnv.isDedicatedServer);
        m_game_ui = smart_cast<CUIGameFMP*>(CurrentGameUI());
        m_game_ui->SetClGame(this);
    }

    luabind::functor<void> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("mp_game_cl.on_connected", funct));
    funct();
    ImportUpdates();
}

void game_cl_freemp::ImportUpdates()
{
    Msg("ImportUpdates");
    u32 num = Level().Objects.o_count();
    for (u32 i = 0; i < num; i++)
    {
        NET_Packet packet;
        IGameObject* object = Level().Objects.o_get_by_iterator(i);
        CInventoryItem* item = smart_cast<CInventoryItem*>(object);
        if (item)
        {
            packet.w_begin(M_EVENT);
            packet.w_u32(M_SCRIPT);
            packet.w_u32(MP_Exports_Upgrade);
            packet.w_u32(item->object_id());
            u_EventSend(packet);
        }
    }
}
