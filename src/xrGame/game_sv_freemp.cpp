#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"

game_sv_freemp::game_sv_freemp() : pure_relcase(&game_sv_freemp::net_Relcase)
{
    m_type = eGameIDFreemp;
    m_alife_simulator = NULL;
}

game_sv_freemp::~game_sv_freemp() { delete_data(m_alife_simulator); }

void game_sv_freemp::Create(shared_str& options)
{
    inherited::Create(options);
    string_path file_name;

    if (FS.exist(file_name, "$game_spawn$", *level_name(options), ".spawn"))
    {
        m_alife_simulator = xr_new<CALifeSimulator>(&server(), &options);
    }
    else
    {
        Msg("Multiplayer>> alife.spawn not found! No A-life");
    }

//    R_ASSERT2(rpoints[1].size(), "rpoints for players not found");

    switch_Phase(GAME_PHASE_PENDING);

    ::Random.seed(GetTickCount());
    m_CorpseList.clear();
}

// player connect #1
void game_sv_freemp::OnPlayerConnect(ClientID id_who)
{
    inherited::OnPlayerConnect(id_who);

    xrClientData* xrCData = m_server->ID_to_client(id_who);
    game_PlayerState* ps_who = get_id(id_who);

    if (!xrCData->flags.bReconnect)
    {
        ps_who->clear();
        ps_who->team = 1;
        ps_who->skin = -1;
    };
    ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);

    ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

    if (GEnv.isDedicatedServer && (xrCData == m_server->GetServerClient()))
    {
        ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
        return;
    }
}

// player connect #2
void game_sv_freemp::OnPlayerConnectFinished(ClientID id_who)
{
    xrClientData* xrCData = m_server->ID_to_client(id_who);
    SpawnPlayer(id_who, "spectator");

    if (xrCData)
    {
        R_ASSERT2(xrCData->ps, "Player state not created yet");
        NET_Packet P;
        GenerateGameMessage(P);
        P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
        P.w_clientID(id_who);
        xrCData->ps->team = 1;
        xrCData->ps->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
        xrCData->ps->setFlag(GAME_PLAYER_FLAG_READY);
        xrCData->ps->net_Export(P, TRUE);
        xrCData->ps->money_for_round = 100000;
        u_EventSend(P);
        xrCData->net_Ready = TRUE;
    };
}

void game_sv_freemp::OnPlayerReady(ClientID id_who)
{
    switch (Phase())
    {
    case GAME_PHASE_INPROGRESS:
    {
        xrClientData* xrCData = (xrClientData*)m_server->ID_to_client(id_who);
        game_PlayerState* ps = get_id(id_who);
        if (ps->IsSkip())
            break;

        if (!(ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
            break;

        xrClientData* xrSCData = (xrClientData*)m_server->GetServerClient();

        CSE_Abstract* pOwner = xrCData->owner;

        RespawnPlayer(id_who, false);
        pOwner = xrCData->owner;
    }
    break;

    default: break;
    };
}

// player disconnect
void game_sv_freemp::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
    inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_freemp::OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType,
    SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA)
{
    if (ps_killed)
    {
        ps_killed->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
        ps_killed->DeathTime = Device.dwTimeGlobal;
    }
    signal_Syncronize();
}

void game_sv_freemp::OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender)
{
    switch (type)
    {
    case GAME_EVENT_PLAYER_KILL: // (g_kill)
    {
        u16 ID = P.r_u16();
        xrClientData* l_pC = (xrClientData*)get_client(ID);
        if (!l_pC)
            break;
        KillPlayer(l_pC->ID, l_pC->ps->GameID);
    }
    break;
    default: inherited::OnEvent(P, type, time, sender);
    };
}

void game_sv_freemp::Update()
{
    if (Phase() != GAME_PHASE_INPROGRESS)
    {
        OnRoundStart();
    }
}

BOOL game_sv_freemp::OnTouch(u16 eid_who, u16 eid_what, BOOL bForced)
{
    CSE_ActorMP* e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));
    if (!e_who)
        return TRUE;

    CSE_Abstract* e_entity = m_server->ID_to_entity(eid_what);
    if (!e_entity)
        return FALSE;

    return TRUE;
}

void game_sv_freemp::OnTradeWindowActivate(ClientID id, u32 money)
{
    Log("Working Server OnTradeWindowActivate = ", money);
    xrClientData* xrCData = m_server->ID_to_client(id);

    xrCData->ps->money_for_round = money;
}

void game_sv_freemp::OnSurgeStart()
{
    luabind::functor<void> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("mp_game_cl.StartSurgeServer", funct));
    funct();

    NET_Packet packet;
     
    packet.w_begin(M_SCRIPT);
    packet.w_u32(MP_Surge);

    // u_EventGen(packet, M_SCRIPT, net_flags(TRUE, TRUE));
    ClientID id;
    id.set(0);

    m_server->SendBroadcast(id, packet, net_flags(TRUE, TRUE));
}

void game_sv_freemp::OnSurgeStop()
{
    luabind::functor<void> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("mp_game_cl.StopSurgeServer", funct));
    funct();

    NET_Packet packet;

    packet.w_begin(M_SCRIPT);
    packet.w_u32(MP_SurgeStop);

    // u_EventGen(packet, M_SCRIPT, net_flags(TRUE, TRUE));
    ClientID id;
    id.set(0);

    m_server->SendBroadcast(id, packet, net_flags(TRUE, TRUE));
}

void game_sv_freemp::TryDoor(u32 id)
{
    luabind::functor<void> funct;
    R_ASSERT(GEnv.ScriptEngine->functor("ph_door.use_callback_mp", funct));
    funct(id);
}

shared_str game_sv_freemp::level_name(const shared_str& server_options) const
{
    if (ai().get_alife())
        return (alife().level_name());
    else
        return inherited::level_name(server_options);
}

shared_str game_sv_freemp::parse_level_name(const shared_str& server_options)
{
    if (ai().get_alife())
        return (alife().level_name());
    else
        return inherited::level_name(server_options);
}
