#pragma once

#include "game_sv_mp.h"
#include "xrEngine/pure_relcase.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "alife_time_manager.h"

class IClient;

class game_sv_freemp : public game_sv_mp, private pure_relcase
{
    friend class pure_relcase;
    typedef game_sv_mp inherited;

    protected:
    CALifeSimulator* m_alife_simulator;

public:
    game_sv_freemp();
    virtual ~game_sv_freemp();

    virtual void Create(shared_str& options);

    virtual bool UseSKin() const { return false; }

    virtual LPCSTR type_name() const { return "freemp"; };
    void __stdcall net_Relcase(IGameObject* O){};

    virtual void OnPlayerReady(ClientID id_who);
    virtual void OnPlayerConnect(ClientID id_who);
    virtual void OnPlayerConnectFinished(ClientID id_who);
    virtual void OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID);

    virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType,
        SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);

    virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);

    virtual void Update();

    virtual BOOL OnTouch(u16 eid_who, u16 eid_what, BOOL bForced = FALSE);

    virtual void OnTradeWindowActivate(ClientID id, u32 money);

    virtual void OnSurgeStart();
    virtual void OnSurgeStop();

    virtual void TryDoor(u32 id);

    shared_str level_name(const shared_str& server_options) const;
    shared_str parse_level_name(const shared_str& server_options);

    IC xrServer& server() const
    {
        VERIFY(m_server);
        return (*m_server);
    }

    IC CALifeSimulator& alife() const
    {
        VERIFY(m_alife_simulator);
        return (*m_alife_simulator);
    }

};
