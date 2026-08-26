#include "DiscordData.hpp"

#include <cctype>
#include <cstdlib>

namespace DiscordBridge
{
    namespace
    {
        std::size_t FindKey(const std::string& json, const std::string& key)
        {
            const std::string needle = "\"" + key + "\"";
            std::size_t pos = json.find(needle);
            while (pos != std::string::npos)
            {
                std::size_t colon = pos + needle.size();
                while (colon < json.size() && std::isspace(static_cast<unsigned char>(json[colon]))) ++colon;
                if (colon < json.size() && json[colon] == ':') return colon + 1;
                pos = json.find(needle, pos + needle.size());
            }
            return std::string::npos;
        }

        void SkipWs(const std::string& json, std::size_t& pos)
        {
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
        }

        bool ParseString(const std::string& json, std::size_t& pos, std::string& out)
        {
            SkipWs(json, pos);
            if (pos >= json.size() || json[pos] != '"') return false;
            ++pos;
            out.clear();
            bool escaped = false;
            while (pos < json.size())
            {
                const char c = json[pos++];
                if (escaped)
                {
                    switch (c)
                    {
                        case 'n': out.push_back('\n'); break;
                        case 'r': out.push_back('\r'); break;
                        case 't': out.push_back('\t'); break;
                        case 'b': out.push_back('\b'); break;
                        case 'f': out.push_back('\f'); break;
                        default: out.push_back(c); break;
                    }
                    escaped = false;
                }
                else if (c == '\\') escaped = true;
                else if (c == '"') return true;
                else out.push_back(c);
            }
            return false;
        }

        bool ExtractBalanced(const std::string& json, std::size_t pos, char open, char close, std::string& out)
        {
            SkipWs(json, pos);
            if (pos >= json.size() || json[pos] != open) return false;
            const std::size_t start = pos;
            int depth = 0;
            bool inString = false;
            bool escaped = false;
            for (; pos < json.size(); ++pos)
            {
                const char c = json[pos];
                if (inString)
                {
                    if (escaped) escaped = false;
                    else if (c == '\\') escaped = true;
                    else if (c == '"') inString = false;
                    continue;
                }
                if (c == '"') { inString = true; continue; }
                if (c == open) ++depth;
                else if (c == close && --depth == 0)
                {
                    out.assign(json, start, pos - start + 1);
                    return true;
                }
            }
            return false;
        }
    }

    std::string DiscordDataStore::makePairKey(const std::string& first, const std::string& second) { return first + ':' + second; }

    void DiscordDataStore::clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        guilds_.clear(); channels_.clear(); roles_.clear(); members_.clear(); users_.clear();
    }

    void DiscordDataStore::storeGuild(const std::string& id, const std::string& json) { std::lock_guard<std::mutex> lock(mutex_); guilds_[id] = json; }
    void DiscordDataStore::storeChannel(const std::string& id, const std::string& json) { std::lock_guard<std::mutex> lock(mutex_); channels_[id] = json; }
    void DiscordDataStore::storeRole(const std::string& g, const std::string& r, const std::string& json) { std::lock_guard<std::mutex> lock(mutex_); roles_[makePairKey(g,r)] = json; }
    void DiscordDataStore::storeMember(const std::string& g, const std::string& u, const std::string& json) { std::lock_guard<std::mutex> lock(mutex_); members_[makePairKey(g,u)] = json; }
    void DiscordDataStore::storeUser(const std::string& u, const std::string& json) { std::lock_guard<std::mutex> lock(mutex_); users_[u] = json; }

    bool DiscordDataStore::getString(const std::string& json, const std::string& key, std::string& value)
    {
        std::size_t pos = FindKey(json, key); if (pos == std::string::npos) return false; SkipWs(json,pos);
        if (json.compare(pos,4,"null") == 0) { value.clear(); return true; }
        return ParseString(json,pos,value);
    }

    bool DiscordDataStore::getInt(const std::string& json, const std::string& key, int& value)
    {
        std::size_t pos = FindKey(json,key); if (pos == std::string::npos) return false; SkipWs(json,pos);
        char* end = nullptr; const long parsed = std::strtol(json.c_str()+pos,&end,10); if (end == json.c_str()+pos) return false; value = static_cast<int>(parsed); return true;
    }

    bool DiscordDataStore::getBool(const std::string& json, const std::string& key, bool& value)
    {
        std::size_t pos = FindKey(json,key); if (pos == std::string::npos) return false; SkipWs(json,pos);
        if (json.compare(pos,4,"true") == 0) { value=true; return true; }
        if (json.compare(pos,5,"false") == 0) { value=false; return true; }
        return false;
    }

    bool DiscordDataStore::getObject(const std::string& json, const std::string& key, std::string& value)
    {
        std::size_t pos = FindKey(json,key); if (pos == std::string::npos) return false; return ExtractBalanced(json,pos,'{','}',value);
    }

    bool DiscordDataStore::arrayHasString(const std::string& json, const std::string& key, const std::string& value)
    {
        std::size_t pos = FindKey(json,key); if (pos == std::string::npos) return false; std::string arr; if (!ExtractBalanced(json,pos,'[',']',arr)) return false;
        return arr.find("\"" + value + "\"") != std::string::npos;
    }

    bool DiscordDataStore::getStringArray(const std::string& json, const std::string& key, std::vector<std::string>& values)
    {
        values.clear();
        std::size_t pos = FindKey(json, key);
        if (pos == std::string::npos) return false;
        std::string array;
        if (!ExtractBalanced(json, pos, '[', ']', array)) return false;
        pos = 1;
        while (pos < array.size())
        {
            SkipWs(array, pos);
            if (pos >= array.size() || array[pos] == ']') break;
            if (array[pos] == ',') { ++pos; continue; }
            std::string value;
            if (!ParseString(array, pos, value)) return false;
            values.push_back(std::move(value));
        }
        return true;
    }

    std::string DiscordDataStore::cdnExtension(const std::string& hash)
    {
        return hash.rfind("a_", 0) == 0 ? "gif" : "png";
    }

    bool DiscordDataStore::findObjectById(const std::string& array, const std::string& id, std::string& object)
    {
        std::size_t pos = 0;
        while ((pos = array.find('{', pos)) != std::string::npos)
        {
            std::string candidate;
            if (!ExtractBalanced(array,pos,'{','}',candidate)) return false;
            std::string candidateId;
            if (getString(candidate,"id",candidateId) && candidateId == id) { object = std::move(candidate); return true; }
            pos += candidate.size();
        }
        return false;
    }

    bool DiscordDataStore::extractEmbeddedUser(const std::string& memberJson, std::string& userJson) { return getObject(memberJson,"user",userJson); }

#define DB_GET_STRING(map,keyexpr,field) do { std::lock_guard<std::mutex> lock(mutex_); auto it=(map).find(keyexpr); if(it==(map).end()) return false; return getString(it->second,field,value); } while(0)
#define DB_GET_INT(map,keyexpr,field) do { std::lock_guard<std::mutex> lock(mutex_); auto it=(map).find(keyexpr); if(it==(map).end()) return false; return getInt(it->second,field,value); } while(0)

    bool DiscordDataStore::getGuildName(const std::string& id, std::string& value) const { DB_GET_STRING(guilds_,id,"name"); }
    bool DiscordDataStore::getGuildOwner(const std::string& id, std::string& value) const { DB_GET_STRING(guilds_,id,"owner_id"); }
    bool DiscordDataStore::getGuildMemberCount(const std::string& id, int& value) const { DB_GET_INT(guilds_,id,"approximate_member_count"); }
    bool DiscordDataStore::getGuildIcon(const std::string& id, std::string& value) const { DB_GET_STRING(guilds_,id,"icon"); }
    bool DiscordDataStore::getGuildBanner(const std::string& id, std::string& value) const { DB_GET_STRING(guilds_,id,"banner"); }
    bool DiscordDataStore::getGuildDescription(const std::string& id, std::string& value) const { DB_GET_STRING(guilds_,id,"description"); }
    bool DiscordDataStore::getGuildIconUrl(const std::string& id, std::string& value) const
    {
        std::string hash; if (!getGuildIcon(id, hash) || hash.empty()) return false;
        value = "https://cdn.discordapp.com/icons/" + id + "/" + hash + "." + cdnExtension(hash) + "?size=1024"; return true;
    }
    bool DiscordDataStore::getGuildBannerUrl(const std::string& id, std::string& value) const
    {
        std::string hash; if (!getGuildBanner(id, hash) || hash.empty()) return false;
        value = "https://cdn.discordapp.com/banners/" + id + "/" + hash + "." + cdnExtension(hash) + "?size=1024"; return true;
    }
    bool DiscordDataStore::getChannelName(const std::string& id, std::string& value) const { DB_GET_STRING(channels_,id,"name"); }
    bool DiscordDataStore::getChannelType(const std::string& id, int& value) const { DB_GET_INT(channels_,id,"type"); }
    bool DiscordDataStore::getChannelTopic(const std::string& id, std::string& value) const { DB_GET_STRING(channels_,id,"topic"); }
    bool DiscordDataStore::getChannelParent(const std::string& id, std::string& value) const { DB_GET_STRING(channels_,id,"parent_id"); }
    bool DiscordDataStore::getChannelGuild(const std::string& id, std::string& value) const { DB_GET_STRING(channels_,id,"guild_id"); }
    bool DiscordDataStore::getChannelPosition(const std::string& id, int& value) const { DB_GET_INT(channels_,id,"position"); }
    bool DiscordDataStore::getChannelNsfw(const std::string& id, bool& value) const { std::lock_guard<std::mutex> lock(mutex_); auto it=channels_.find(id); return it!=channels_.end() && getBool(it->second,"nsfw",value); }
    bool DiscordDataStore::getChannelSlowmode(const std::string& id, int& value) const { DB_GET_INT(channels_,id,"rate_limit_per_user"); }
    bool DiscordDataStore::getRoleName(const std::string& g,const std::string& r,std::string& value) const { DB_GET_STRING(roles_,makePairKey(g,r),"name"); }
    bool DiscordDataStore::getRoleColor(const std::string& g,const std::string& r,int& value) const { DB_GET_INT(roles_,makePairKey(g,r),"color"); }
    bool DiscordDataStore::getRolePosition(const std::string& g,const std::string& r,int& value) const { DB_GET_INT(roles_,makePairKey(g,r),"position"); }
    bool DiscordDataStore::getRoleHoist(const std::string& g,const std::string& r,bool& value) const { std::lock_guard<std::mutex> lock(mutex_); auto it=roles_.find(makePairKey(g,r)); return it!=roles_.end() && getBool(it->second,"hoist",value); }
    bool DiscordDataStore::getRoleMentionable(const std::string& g,const std::string& r,bool& value) const { std::lock_guard<std::mutex> lock(mutex_); auto it=roles_.find(makePairKey(g,r)); return it!=roles_.end() && getBool(it->second,"mentionable",value); }
    bool DiscordDataStore::getRolePermissions(const std::string& g,const std::string& r,std::string& value) const { DB_GET_STRING(roles_,makePairKey(g,r),"permissions"); }
    bool DiscordDataStore::getRoleManaged(const std::string& g,const std::string& r,bool& value) const { std::lock_guard<std::mutex> lock(mutex_); auto it=roles_.find(makePairKey(g,r)); return it!=roles_.end() && getBool(it->second,"managed",value); }

    bool DiscordDataStore::getMemberNick(const std::string& g,const std::string& u,std::string& value) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); if(it==members_.end()) return false;
        if (getString(it->second,"nick",value) && !value.empty()) return true;
        std::string user; if (!getObject(it->second,"user",user)) return false;
        if (getString(user,"global_name",value) && !value.empty()) return true;
        return getString(user,"username",value);
    }

    bool DiscordDataStore::getMemberName(const std::string& g,const std::string& u,std::string& value) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); if(it==members_.end()) return false;
        std::string user; return getObject(it->second,"user",user) && getString(user,"username",value);
    }


    bool DiscordDataStore::getMemberGlobalName(const std::string& g,const std::string& u,std::string& value) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); if(it==members_.end()) return false;
        std::string user; return getObject(it->second,"user",user) && getString(user,"global_name",value);
    }

    bool DiscordDataStore::getMemberAvatar(const std::string& g,const std::string& u,std::string& value) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); if(it==members_.end()) return false;
        if (getString(it->second,"avatar",value) && !value.empty()) return true;
        std::string user; return getObject(it->second,"user",user) && getString(user,"avatar",value);
    }

    bool DiscordDataStore::memberHasRole(const std::string& g,const std::string& u,const std::string& r) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); return it!=members_.end() && arrayHasString(it->second,"roles",r);
    }


    bool DiscordDataStore::getMemberJoinedAt(const std::string& g,const std::string& u,std::string& value) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); return it!=members_.end() && getString(it->second,"joined_at",value);
    }

    bool DiscordDataStore::getMemberRoleCount(const std::string& g,const std::string& u,int& value) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); if(it==members_.end()) return false;
        std::vector<std::string> roles; if(!getStringArray(it->second,"roles",roles)) return false; value=static_cast<int>(roles.size()); return true;
    }

    bool DiscordDataStore::getMemberRole(const std::string& g,const std::string& u,int index,std::string& value) const
    {
        if(index < 0) return false;
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); if(it==members_.end()) return false;
        std::vector<std::string> roles; if(!getStringArray(it->second,"roles",roles) || static_cast<std::size_t>(index)>=roles.size()) return false; value=roles[static_cast<std::size_t>(index)]; return true;
    }

    bool DiscordDataStore::getMemberAvatarUrl(const std::string& g,const std::string& u,std::string& value) const
    {
        std::lock_guard<std::mutex> lock(mutex_); auto it=members_.find(makePairKey(g,u)); if(it==members_.end()) return false;
        std::string hash;
        if (getString(it->second,"avatar",hash) && !hash.empty())
        { value="https://cdn.discordapp.com/guilds/"+g+"/users/"+u+"/avatars/"+hash+"."+cdnExtension(hash)+"?size=1024"; return true; }
        std::string user; if(!getObject(it->second,"user",user) || !getString(user,"avatar",hash) || hash.empty()) return false;
        value="https://cdn.discordapp.com/avatars/"+u+"/"+hash+"."+cdnExtension(hash)+"?size=1024"; return true;
    }

    bool DiscordDataStore::getUserName(const std::string& u,std::string& value) const { DB_GET_STRING(users_,u,"username"); }
    bool DiscordDataStore::getUserGlobalName(const std::string& u,std::string& value) const { DB_GET_STRING(users_,u,"global_name"); }
    bool DiscordDataStore::getUserAvatar(const std::string& u,std::string& value) const { DB_GET_STRING(users_,u,"avatar"); }
    bool DiscordDataStore::isUserBot(const std::string& u,bool& value) const { std::lock_guard<std::mutex> lock(mutex_); auto it=users_.find(u); return it!=users_.end() && getBool(it->second,"bot",value); }
    bool DiscordDataStore::getUserAvatarUrl(const std::string& u,std::string& value) const
    {
        std::string hash; if(!getUserAvatar(u,hash) || hash.empty()) return false;
        value="https://cdn.discordapp.com/avatars/"+u+"/"+hash+"."+cdnExtension(hash)+"?size=1024"; return true;
    }

#undef DB_GET_STRING
#undef DB_GET_INT
}
