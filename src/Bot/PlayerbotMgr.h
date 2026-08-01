/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_PLAYERBOTMGR_H
#define PLAYERBOTS_PLAYERBOTMGR_H

#include <mutex>
#include <shared_mutex>

#include "ObjectGuid.h"
#include "Player.h"
#include "PlayerbotAIBase.h"

class ChatHandler;
class PlayerbotAI;
class PlayerbotLoginQueryHolder;
class WorldPacket;

typedef std::map<ObjectGuid, Player*> PlayerBotMap;
typedef std::map<std::string, std::set<std::string> > PlayerBotErrorMap;

class PlayerbotHolder : public PlayerbotAIBase
{
public:
    PlayerbotHolder();
    virtual ~PlayerbotHolder(){};

    void AddPlayerBot(ObjectGuid guid, uint32 masterAccountId);
    bool IsAccountLinked(uint32 accountId, uint32 masterAccountId);
    void HandlePlayerBotLoginCallback(PlayerbotLoginQueryHolder const& holder);

    void LogoutPlayerBot(ObjectGuid guid);
    void DisablePlayerBot(ObjectGuid guid);
    void RemoveFromPlayerbotsMap(ObjectGuid guid);
    Player* GetPlayerBot(ObjectGuid guid) const;
    Player* GetPlayerBot(ObjectGuid::LowType lowGuid) const;

    /// Returns a copy of the bot map taken under the lock.
    ///
    /// This replaces the begin/end iterator pair that used to be exposed here. Handing
    /// out raw iterators cannot be made thread-safe: the caller walks the container long
    /// after the accessor returned, so a bot logging out on another map thread erases the
    /// node the caller is standing on. Every caller iterated them unguarded, which is why
    /// locking the accessors alone would not have closed the race.
    ///
    /// Copying is affordable because the map holds pointers, and iteration happens on
    /// command paths and logout, not per tick.
    PlayerBotMap GetPlayerBotsSnapshot() const;

    void UpdateAIInternal([[maybe_unused]] uint32 elapsed, [[maybe_unused]] bool minimal = false) override{};
    void UpdateSessions();
    void HandleBotPackets(WorldSession* session);

    void LogoutAllBots();
    void OnBotLogin(Player* const bot);

    std::vector<std::string> HandlePlayerbotCommand(char const* args, Player* master = nullptr);
    std::string const ProcessBotCommand(std::string const cmd, ObjectGuid guid, ObjectGuid masterguid, bool admin,
                                        uint32 masterAccountId, uint32 masterGuildId);
    uint32 GetAccountId(std::string const name);
    uint32 GetAccountId(ObjectGuid guid);
    std::string const ListBots(Player* master);
    std::string const LookupBots(Player* master);
    uint32 GetPlayerbotsCount() const;
    uint32 GetPlayerbotsCountByClass(uint32 cls);

protected:
    virtual void OnBotLoginInternal(Player* const bot) = 0;

    /// Guards playerBots. Reads dominate, so shared_mutex rather than mutex.
    mutable std::shared_mutex m_botsMutex;
    PlayerBotMap playerBots;

    /// botLoading is static -- shared by every holder -- so it needs a lock with the same
    /// storage duration rather than the per-instance one above.
    static std::mutex s_botLoadingMutex;
    static std::unordered_map<ObjectGuid, uint32> botLoading;
};

class PlayerbotMgr : public PlayerbotHolder
{
public:
    PlayerbotMgr(Player* const master);
    virtual ~PlayerbotMgr();

    static bool HandlePlayerbotMgrCommand(ChatHandler* handler, char const* args);
    void HandleMasterIncomingPacket(WorldPacket const& packet);
    void HandleMasterOutgoingPacket(WorldPacket const& packet);
    void HandleCommand(uint32 type, std::string const text);
    void OnPlayerLogin(Player* player);
    void CancelLogout();

    void UpdateAIInternal(uint32 elapsed, bool minimal = false) override;
    void TellError(std::string const botName, std::string const text);

    Player* GetMaster() const { return master; };

    void SaveToDB();

    void HandleSetSecurityKeyCommand(Player* player, const std::string& key);
    void HandleLinkAccountCommand(Player* player, const std::string& accountName, const std::string& key);
    void HandleViewLinkedAccountsCommand(Player* player);
    void HandleUnlinkAccountCommand(Player* player, const std::string& accountName);

protected:
    void OnBotLoginInternal(Player* const bot) override;
    void CheckTellErrors(uint32 elapsed);

private:
    Player* const master;
    PlayerBotErrorMap errors;
    time_t lastErrorTell;
};

class PlayerbotsMgr
{
public:
    static PlayerbotsMgr& instance()
    {
        static PlayerbotsMgr instance;
        return instance;
    }

    void AddPlayerbotData(Player* player, bool isBotAI);
    void RemovePlayerBotData(ObjectGuid const& guid, bool is_AI);

    PlayerbotAI* GetPlayerbotAI(Player* player);
    PlayerbotMgr* GetPlayerbotMgr(Player* player);

private:
    PlayerbotsMgr() = default;
    ~PlayerbotsMgr() = default;

    PlayerbotsMgr(const PlayerbotsMgr&) = delete;
    PlayerbotsMgr& operator=(const PlayerbotsMgr&) = delete;

    PlayerbotsMgr(PlayerbotsMgr&&) = delete;
    PlayerbotsMgr& operator=(PlayerbotsMgr&&) = delete;

    /// Guards both maps below.
    ///
    /// These are looked up once per player per tick from OnPlayerAfterUpdate, which runs
    /// on a map thread. With MapUpdate.Threads > 1 several map threads read concurrently
    /// while logins and logouts insert and erase, so an unsynchronised std::unordered_map
    /// is a data race: a rehash on insert invalidates the buckets a reader is walking.
    ///
    /// shared_mutex rather than mutex because the read path is the hot one — reads happen
    /// every tick per player, writes only on login and logout — and readers must not
    /// serialise against each other or the lock would undo the parallelism it protects.
    mutable std::shared_mutex _registryMutex;
    std::unordered_map<ObjectGuid, PlayerbotAIBase*> _playerbotsAIMap;
    std::unordered_map<ObjectGuid, PlayerbotAIBase*> _playerbotsMgrMap;
};

#define sPlayerbotsMgr PlayerbotsMgr::instance()

#endif
