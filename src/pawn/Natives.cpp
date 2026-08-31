#include "Natives.hpp"

#include "../Version.hpp"
#include "../core/BridgeCore.hpp"
#include "../discord/components/legacy/Button.hpp"
#include "../discord/embeds/Embed.hpp"
#include "../discord/components/legacy/SelectMenu.hpp"
#include "../discord/components/legacy/Modal.hpp"
#include "../discord/commands/Command.hpp"
#include "../discord/components/v2/Component.hpp"
#include "../core/Metrics.hpp"
#include "../plugin/Plugin.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

using namespace std;

namespace DiscordBridge
{
    namespace
    {
        bool GetPawnString(AMX *amx, cell parameter, string &output, bool allowEmpty = true)
        {
            if (!amx)
                return false;

            cell *address = nullptr;
            if (amx_GetAddr(amx, parameter, &address) != AMX_ERR_NONE || !address)
                return false;

            int length = 0;
            if (amx_StrLen(address, &length) != AMX_ERR_NONE)
                return false;

            if (length <= 0)
            {
                output.clear();
                return allowEmpty;
            }

            vector<char> buffer(static_cast<size_t>(length) + 1);
            if (amx_GetString(buffer.data(), address, 0, buffer.size()) != AMX_ERR_NONE)
                return false;

            output.assign(buffer.data(), static_cast<size_t>(length));
            return true;
        }

        bool SetPawnString(AMX *amx, cell parameter, cell maxLength, const string &value)
        {
            if (!amx || maxLength <= 0)
                return false;
            cell *address = nullptr;
            if (amx_GetAddr(amx, parameter, &address) != AMX_ERR_NONE || !address)
                return false;
            return amx_SetString(address, value.c_str(), 0, 0, static_cast<size_t>(maxLength)) == AMX_ERR_NONE;
        }

        bool HasParams(const cell *params, size_t count)
        {
            return params && params[0] >= static_cast<cell>(count * sizeof(cell));
        }

        BridgeCore *GetCore()
        {
            BridgeCore *core = GetBridgeCore();
            return core && core->isInitialized() ? core : nullptr;
        }

        DiscordClient *GetDiscord()
        {
            BridgeCore *core = GetCore();
            return core ? &core->getDiscordClient() : nullptr;
        }

        Embed *GetEmbed(cell handle)
        {
            BridgeCore *core = GetCore();
            if (!core || handle <= 0)
                return nullptr;

            return core->getEmbedManager().get(static_cast<EmbedHandle>(handle));
        }

        Button *GetButton(cell handle)
        {
            BridgeCore *core = GetCore();
            if (!core || handle <= 0)
                return nullptr;

            return core->getButtonManager().get(static_cast<ButtonHandle>(handle));
        }

        ActionRow *GetActionRow(cell handle)
        {
            BridgeCore *core = GetCore();
            if (!core || handle <= 0)
                return nullptr;

            return core->getActionRowManager().get(static_cast<ActionRowHandle>(handle));
        }

        cell AMX_NATIVE_CALL Native_Connect(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            string token;
            if (!GetPawnString(amx, params[1], token, false))
                return 0;

            DiscordClient *discord = GetDiscord();
            if (!discord)
                return 0;

            if (discord->isInitialized())
                return 1;

            return discord->initialize(token) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_Disconnect(AMX *, cell *)
        {
            DiscordClient *discord = GetDiscord();
            if (!discord || !discord->isInitialized())
                return 0;

            discord->shutdown();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_SetStatus(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            DiscordClient *discord = GetDiscord();

            return discord && discord->setStatus(static_cast<int>(params[1])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_SetActivity(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;

            string name;
            string state;
            string url;

            if (!GetPawnString(amx, params[2], name, false))
                return 0;
            if (!GetPawnString(amx, params[3], state))
                return 0;
            if (!GetPawnString(amx, params[4], url))
                return 0;

            DiscordClient *discord = GetDiscord();

            return discord && discord->setActivity(static_cast<int>(params[1]), name, state, url) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ClearActivity(AMX *, cell *)
        {
            DiscordClient *discord = GetDiscord();

            return discord && discord->clearActivity() ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_SetPresence(AMX *amx, cell *params)
        {
            if (!HasParams(params, 6))
                return 0;

            string name;
            string state;
            string url;

            if (!GetPawnString(amx, params[3], name))
                return 0;
            if (!GetPawnString(amx, params[4], state))
                return 0;
            if (!GetPawnString(amx, params[5], url))
                return 0;

            DiscordClient *discord = GetDiscord();

            return discord && discord->setPresence(static_cast<int>(params[1]), static_cast<int>(params[2]), name, state, url, params[6] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_SendMessage(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            string channelId;
            string message;

            if (!GetPawnString(amx, params[1], channelId, false))
                return 0;
            if (!GetPawnString(amx, params[2], message, false))
                return 0;

            DiscordClient *discord = GetDiscord();

            return discord && discord->sendMessage(channelId, message) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_EditMessage(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;

            string channelId;
            string messageId;
            string content;

            if (!GetPawnString(amx, params[1], channelId, false))
                return 0;
            if (!GetPawnString(amx, params[2], messageId, false))
                return 0;
            if (!GetPawnString(amx, params[3], content))
                return 0;

            DiscordClient *discord = GetDiscord();

            return discord && discord->editMessage(channelId, messageId, content) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_DeleteMessage(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            string channelId;
            string messageId;

            if (!GetPawnString(amx, params[1], channelId, false))
                return 0;
            if (!GetPawnString(amx, params[2], messageId, false))
                return 0;

            DiscordClient *discord = GetDiscord();

            return discord && discord->deleteMessage(channelId, messageId) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CreateEmbed(AMX *, cell *)
        {
            BridgeCore *core = GetCore();

            return core ? static_cast<cell>(core->getEmbedManager().create()) : 0;
        }

        cell AMX_NATIVE_CALL Native_DestroyEmbed(AMX *, cell *params)
        {
            if (!HasParams(params, 1) || params[1] <= 0)
                return 0;

            BridgeCore *core = GetCore();

            return core && core->getEmbedManager().destroy(static_cast<EmbedHandle>(params[1])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetTitle(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string title;

            if (!GetPawnString(amx, params[2], title))
                return 0;
            if (title.size() > 256)
                return 0;

            embed->setTitle(title);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetDescription(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string description;

            if (!GetPawnString(amx, params[2], description))
                return 0;
            if (description.size() > 4096)
                return 0;

            embed->setDescription(description);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetUrl(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string url;

            if (!GetPawnString(amx, params[2], url))
                return 0;

            embed->setUrl(url);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetColor(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            const uint32_t color = static_cast<uint32_t>(params[2]);
            if (color > 0xFFFFFF)
                return 0;

            embed->setColor(color);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetAuthor(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string name;
            string url;
            string iconUrl;

            if (!GetPawnString(amx, params[2], name, false))
                return 0;
            if (!GetPawnString(amx, params[3], url))
                return 0;
            if (!GetPawnString(amx, params[4], iconUrl))
                return 0;
            if (name.size() > 256)
                return 0;

            embed->setAuthor(name, url, iconUrl);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedClearAuthor(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            embed->clearAuthor();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetThumbnail(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string url;

            if (!GetPawnString(amx, params[2], url))
                return 0;

            embed->setThumbnail(url);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedClearThumbnail(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            embed->clearThumbnail();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetImage(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string url;

            if (!GetPawnString(amx, params[2], url))
                return 0;

            embed->setImage(url);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedClearImage(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            embed->clearImage();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedSetFooter(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string text;
            string iconUrl;

            if (!GetPawnString(amx, params[2], text, false))
                return 0;
            if (!GetPawnString(amx, params[3], iconUrl))
                return 0;
            if (text.size() > 2048)
                return 0;

            embed->setFooter(text, iconUrl);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedClearFooter(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            embed->clearFooter();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedAddField(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            string name;
            string value;

            if (!GetPawnString(amx, params[2], name, false))
                return 0;
            if (!GetPawnString(amx, params[3], value, false))
                return 0;
            if (name.size() > 256 || value.size() > 1024)
                return 0;

            return embed->addField(name, value, params[4] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_EmbedClearFields(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            embed->clearFields();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_EmbedClear(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            Embed *embed = GetEmbed(params[1]);
            if (!embed)
                return 0;

            embed->clear();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_SendEmbed(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            string channelId;

            if (!GetPawnString(amx, params[1], channelId, false))
                return 0;

            Embed *embed = GetEmbed(params[2]);
            if (!embed || embed->empty())
                return 0;

            DiscordClient *discord = GetDiscord();

            return discord && discord->sendEmbed(channelId, embed->toJson()) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CreateButton(AMX *, cell *)
        {
            BridgeCore *core = GetCore();

            return core ? static_cast<cell>(core->getButtonManager().create()) : 0;
        }

        cell AMX_NATIVE_CALL Native_DestroyButton(AMX *, cell *params)
        {
            if (!HasParams(params, 1) || params[1] <= 0)
                return 0;

            BridgeCore *core = GetCore();

            return core && core->getButtonManager().destroy(static_cast<ButtonHandle>(params[1])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetLabel(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Button *button = GetButton(params[1]);
            if (!button)
                return 0;

            string label;

            if (!GetPawnString(amx, params[2], label, false))
                return 0;
            if (label.size() > 80)
                return 0;

            button->setLabel(label);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetCustomId(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Button *button = GetButton(params[1]);
            if (!button)
                return 0;

            string customId;

            if (!GetPawnString(amx, params[2], customId, false))
                return 0;
            if (customId.size() > 100)
                return 0;

            button->setCustomId(customId);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetUrl(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Button *button = GetButton(params[1]);
            if (!button)
                return 0;

            string url;

            if (!GetPawnString(amx, params[2], url, false))
                return 0;

            button->setUrl(url);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetEmoji(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Button *button = GetButton(params[1]);
            if (!button)
                return 0;

            string emoji;

            if (!GetPawnString(amx, params[2], emoji))
                return 0;

            button->setEmoji(emoji);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetEmojiEx(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            Button *button = GetButton(params[1]);
            if (!button)
                return 0;
            string name, id;
            if (!GetPawnString(amx, params[2], name, false) || !GetPawnString(amx, params[3], id, false))
                return 0;
            if (id.size() > 20)
                return 0;
            for (const char c : id)
                if (c < '0' || c > '9')
                    return 0;
            button->setEmojiEx(name, id, params[4] != 0);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetSkuId(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            Button *button = GetButton(params[1]);
            if (!button)
                return 0;
            string skuId;
            if (!GetPawnString(amx, params[2], skuId, false) || skuId.size() > 20)
                return 0;
            for (const char c : skuId)
                if (c < '0' || c > '9')
                    return 0;
            button->setSkuId(skuId);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetComponentId(AMX *, cell *params)
        {
            if (!HasParams(params, 2) || params[2] < 0)
                return 0;
            Button *button = GetButton(params[1]);
            if (!button)
                return 0;
            button->setComponentId(static_cast<std::uint32_t>(params[2]));
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetStyle(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            const int style = static_cast<int>(params[2]);
            if (style < 1 || style > 6)
                return 0;

            Button *button = GetButton(params[1]);
            if (!button)
                return 0;

            button->setStyle(static_cast<ButtonStyle>(style));
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonSetDisabled(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            Button *button = GetButton(params[1]);
            if (!button)
                return 0;

            button->setDisabled(params[2] != 0);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_ButtonClear(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            Button *button = GetButton(params[1]);
            if (!button)
                return 0;

            button->clear();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CreateActionRow(AMX *, cell *)
        {
            BridgeCore *core = GetCore();

            return core ? static_cast<cell>(core->getActionRowManager().create()) : 0;
        }

        cell AMX_NATIVE_CALL Native_DestroyActionRow(AMX *, cell *params)
        {
            if (!HasParams(params, 1) || params[1] <= 0)
                return 0;

            BridgeCore *core = GetCore();

            return core && core->getActionRowManager().destroy(static_cast<ActionRowHandle>(params[1])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ActionRowAddButton(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            ActionRow *row = GetActionRow(params[1]);
            Button *button = GetButton(params[2]);

            if (!row || !button)
                return 0;

            return row->addButton(static_cast<ButtonHandle>(params[2])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ActionRowRemoveButton(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            ActionRow *row = GetActionRow(params[1]);
            if (!row)
                return 0;

            return row->removeButton(static_cast<ButtonHandle>(params[2])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ActionRowClear(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;

            ActionRow *row = GetActionRow(params[1]);
            if (!row)
                return 0;

            row->clear();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_SendComponents(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;

            string channelId;
            if (!GetPawnString(amx, params[1], channelId, false))
                return 0;

            BridgeCore *core = GetCore();
            if (!core)
                return 0;

            ActionRow *row = GetActionRow(params[2]);
            if (!row || row->empty())
                return 0;

            const string componentsJson = row->toJson(core->getButtonManager(), &core->getSelectMenuManager());
            if (componentsJson.empty())
                return 0;

            return core->getDiscordClient().sendComponents(channelId, componentsJson) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CreateSelectMenu(AMX *, cell *)
        {
            auto *c = GetCore();
            return c ? static_cast<cell>(c->getSelectMenuManager().create()) : 0;
        }
        cell AMX_NATIVE_CALL Native_CreateSelectMenuType(AMX *, cell *p)
        {
            if (!HasParams(p, 1))
                return 0;
            auto *c = GetCore();
            if (!c)
                return 0;
            const auto handle = c->getSelectMenuManager().create();
            auto *menu = c->getSelectMenuManager().get(handle);
            if (!menu)
                return 0;
            menu->setType(p[1]);
            return static_cast<cell>(handle);
        }
        cell AMX_NATIVE_CALL Native_DestroySelectMenu(AMX *, cell *p)
        {
            auto *c = GetCore();
            return HasParams(p, 1) && c && c->getSelectMenuManager().destroy(static_cast<SelectMenuHandle>(p[1]));
        }
        cell AMX_NATIVE_CALL Native_SelectMenuSetCustomId(AMX *a, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getSelectMenuManager().get(p[1]) : nullptr;
            string v;
            if (!m || !GetPawnString(a, p[2], v, false) || v.size() > 100)
                return 0;
            m->setCustomId(v);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_SelectSetPlaceholder(AMX *a, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getSelectMenuManager().get(p[1]) : nullptr;
            string v;
            if (!m || !GetPawnString(a, p[2], v) || v.size() > 150)
                return 0;
            m->setPlaceholder(v);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_SelectMenuSetRange(AMX *, cell *p)
        {
            if (!HasParams(p, 3))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getSelectMenuManager().get(p[1]) : nullptr;
            if (!m)
                return 0;
            m->setMinValues(p[2]);
            m->setMaxValues(p[3]);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_SelectMenuSetDisabled(AMX *, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getSelectMenuManager().get(p[1]) : nullptr;
            if (!m)
                return 0;
            m->setDisabled(p[2] != 0);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_SelectMenuAddOption(AMX *a, cell *p)
        {
            if (!HasParams(p, 6))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getSelectMenuManager().get(p[1]) : nullptr;
            if (!m)
                return 0;
            SelectOption o;
            if (!GetPawnString(a, p[2], o.label, false) || !GetPawnString(a, p[3], o.value, false) || !GetPawnString(a, p[4], o.description) || !GetPawnString(a, p[5], o.emoji))
                return 0;
            o.isDefault = p[6] != 0;
            return m->addOption(o);
        }
        cell AMX_NATIVE_CALL Native_SelectMenuSetType(AMX *, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getSelectMenuManager().get(p[1]) : nullptr;
            if (!m)
                return 0;
            m->setType(p[2]);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_SelectMenuAddChannelType(AMX *, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getSelectMenuManager().get(p[1]) : nullptr;
            return m && m->addChannelType(p[2]) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ActionRowSetSelectMenu(AMX *, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            if (!c || !c->getSelectMenuManager().get(p[2]))
                return 0;
            auto *r = c->getActionRowManager().get(p[1]);
            return r && r->setSelectMenu(p[2]);
        }

        cell AMX_NATIVE_CALL Native_CreateModal(AMX *, cell *)
        {
            auto *c = GetCore();
            return c ? static_cast<cell>(c->getModalManager().create()) : 0;
        }
        cell AMX_NATIVE_CALL Native_DestroyModal(AMX *, cell *p)
        {
            auto *c = GetCore();
            return HasParams(p, 1) && c && c->getModalManager().destroy(p[1]);
        }
        cell AMX_NATIVE_CALL Native_ModalSetCustomId(AMX *a, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getModalManager().get(p[1]) : nullptr;
            string v;
            if (!m || !GetPawnString(a, p[2], v, false))
                return 0;
            m->setCustomId(v);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_ModalSetTitle(AMX *a, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getModalManager().get(p[1]) : nullptr;
            string v;
            if (!m || !GetPawnString(a, p[2], v, false))
                return 0;
            m->setTitle(v);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_ModalAddTextInput(AMX *a, cell *p)
        {
            if (!HasParams(p, 9))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getModalManager().get(p[1]) : nullptr;
            if (!m)
                return 0;
            TextInput i;
            if (!GetPawnString(a, p[2], i.customId, false) || !GetPawnString(a, p[3], i.label, false) || !GetPawnString(a, p[5], i.placeholder) || !GetPawnString(a, p[6], i.value))
                return 0;
            i.style = p[4];
            i.required = p[7] != 0;
            i.minLength = p[8];
            i.maxLength = p[9];
            return m->addInput(i);
        }
        cell AMX_NATIVE_CALL Native_ModalAddSelectMenu(AMX *a, cell *p)
        {
            if (!HasParams(p, 4))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getModalManager().get(p[1]) : nullptr;
            auto *menu = c ? c->getSelectMenuManager().get(p[2]) : nullptr;
            if (!m || !menu)
                return 0;
            string label, description;
            if (!GetPawnString(a, p[3], label, false) || !GetPawnString(a, p[4], description))
                return 0;
            const string json = menu->toJson();
            return !json.empty() && m->addLabeledComponent(label, description, json) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ModalAddFileUpload(AMX *a, cell *p)
        {
            if (!HasParams(p, 7))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getModalManager().get(p[1]) : nullptr;
            if (!m)
                return 0;
            string customId, label, description;
            if (!GetPawnString(a, p[2], customId, false) || !GetPawnString(a, p[3], label, false) || !GetPawnString(a, p[4], description))
                return 0;
            return m->addFileUpload(customId, label, description, p[5], p[6], p[7] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ModalAddComponent(AMX *, cell *p)
        {
            auto *core = GetCore();
            if (!core || !HasParams(p, 2))
                return 0;
            auto *modal = core->getModalManager().get((ModalHandle)p[1]);
            auto *c = core->getComponentManager().get((ComponentHandle)p[2]);
            if (!modal || !c)
                return 0;
            const int t = c->type();
            if (t != 10 && t != 18)
                return 0;
            return modal->addRawComponent(c->toJson(core->getComponentManager())) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ModalAddTextDisplay(AMX *a, cell *p)
        {
            if (!HasParams(p, 2))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getModalManager().get(p[1]) : nullptr;
            string content;
            return m && GetPawnString(a, p[2], content, false) && m->addTextDisplay(content) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ShowModal(AMX *a, cell *p)
        {
            if (!HasParams(p, 3))
                return 0;
            string id, token;
            if (!GetPawnString(a, p[1], id, false) || !GetPawnString(a, p[2], token, false))
                return 0;
            auto *c = GetCore();
            auto *m = c ? c->getModalManager().get(p[3]) : nullptr;
            if (!m)
                return 0;
            return c->getDiscordClient().showModal(id, token, m->toJson());
        }
        cell AMX_NATIVE_CALL Native_ModalGetValue(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3) || params[3] <= 0)
                return 0;
            string customId;
            if (!GetPawnString(amx, params[1], customId, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            string value;
            if (!core->getPawnRuntime().getModalValue(customId, value))
                return 0;
            return SetPawnString(amx, params[2], params[3], value) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_RespondInteraction(AMX *a, cell *p)
        {
            if (!HasParams(p, 4))
                return 0;
            string id, token, content;
            if (!GetPawnString(a, p[1], id, false) || !GetPawnString(a, p[2], token, false) || !GetPawnString(a, p[3], content, false))
                return 0;
            auto *c = GetCore();
            return c && c->getDiscordClient().respondInteraction(id, token, content, p[4] != 0);
        }

        cell AMX_NATIVE_CALL Native_InteractionValueCount(AMX *, cell *)
        {
            BridgeCore *core = GetCore();
            return core ? static_cast<cell>(core->getPawnRuntime().getInteractionValueCount()) : 0;
        }

        cell AMX_NATIVE_CALL Native_InteractionGetValue(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3) || params[1] < 0 || params[3] <= 0)
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            string value;
            if (!core->getPawnRuntime().getInteractionValue(static_cast<size_t>(params[1]), value))
                return 0;
            return SetPawnString(amx, params[2], params[3], value) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_InteractionGetField(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3) || params[3] <= 0)
                return 0;
            string name;
            if (!GetPawnString(amx, params[1], name, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            string value;
            if (!core->getPawnRuntime().getInteractionField(name, value))
                return 0;
            return SetPawnString(amx, params[2], params[3], value) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CreateChannel(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            string guildId, name;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], name, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->createChannel(guildId, name, static_cast<int>(params[3])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_DeleteChannel(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            string channelId;
            if (!GetPawnString(amx, params[1], channelId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->deleteChannel(channelId) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CreateRole(AMX *amx, cell *params)
        {
            if (!HasParams(params, 5))
                return 0;
            string guildId, name;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], name, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->createRole(guildId, name, static_cast<int>(params[3]), params[4] != 0, params[5] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_DeleteRole(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string guildId, roleId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], roleId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->deleteRole(guildId, roleId) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_AddMemberRole(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            string guildId, userId, roleId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], userId, false) || !GetPawnString(amx, params[3], roleId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->addMemberRole(guildId, userId, roleId) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_RemoveMemberRole(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            string guildId, userId, roleId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], userId, false) || !GetPawnString(amx, params[3], roleId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->removeMemberRole(guildId, userId, roleId) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_KickMember(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string guildId, userId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], userId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->kickMember(guildId, userId) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_BanMember(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            string guildId, userId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], userId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->banMember(guildId, userId, static_cast<int>(params[3])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_UnbanMember(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string guildId, userId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], userId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->unbanMember(guildId, userId) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CreateCommand(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string name, description;
            if (!GetPawnString(amx, params[1], name, false) || !GetPawnString(amx, params[2], description, false))
                return 0;
            BridgeCore *core = GetCore();
            return core ? static_cast<cell>(core->getCommandManager().create(name, description)) : 0;
        }

        cell AMX_NATIVE_CALL Native_DestroyCommand(AMX *, cell *params)
        {
            if (!HasParams(params, 1) || params[1] <= 0)
                return 0;
            BridgeCore *core = GetCore();
            return core && core->getCommandManager().destroy(static_cast<CommandHandle>(params[1])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CommandAddOption(AMX *amx, cell *params)
        {
            if (!HasParams(params, 5) || params[1] <= 0)
                return 0;
            string name, description;
            if (!GetPawnString(amx, params[3], name, false) || !GetPawnString(amx, params[4], description, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            CommandOption option;
            option.type = static_cast<int>(params[2]);
            option.name = std::move(name);
            option.description = std::move(description);
            option.required = params[5] != 0;
            return static_cast<cell>(core->getCommandManager().addOption(static_cast<CommandHandle>(params[1]), std::move(option)));
        }

        cell AMX_NATIVE_CALL Native_CommandAddOptionEx(AMX *amx, cell *params)
        {
            if (!HasParams(params, 6) || params[1] <= 0)
                return 0;
            string name, description;
            if (!GetPawnString(amx, params[3], name, false) || !GetPawnString(amx, params[4], description, false))
                return 0;
            auto *core = GetCore();
            if (!core)
                return 0;
            CommandOption option;
            option.type = static_cast<int>(params[2]);
            option.name = std::move(name);
            option.description = std::move(description);
            option.required = params[5] != 0;
            return static_cast<cell>(core->getCommandManager().addOption(static_cast<CommandHandle>(params[1]), std::move(option), static_cast<CommandOptionHandle>(params[6])));
        }

        cell AMX_NATIVE_CALL Native_CommandAddChoice(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            string name, value;
            if (!GetPawnString(amx, params[3], name, false) || !GetPawnString(amx, params[4], value, false))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command)
                return 0;
            CommandChoice choice;
            choice.name = std::move(name);
            choice.value = std::move(value);
            choice.numeric = false;
            if (!command->addChoice(params[2], std::move(choice)))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandAddNumericChoice(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            string name, value;
            if (!GetPawnString(amx, params[3], name, false) || !GetPawnString(amx, params[4], value, false))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command)
                return 0;
            CommandChoice choice;
            choice.name = std::move(name);
            choice.value = std::move(value);
            choice.numeric = true;
            if (!command->addChoice(params[2], std::move(choice)))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandSetAutocomplete(AMX *, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command || !command->setAutocomplete(params[2], params[3] != 0))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandSetNumberRange(AMX *, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command || !command->setNumberRange(params[2], static_cast<double>(params[3]), static_cast<double>(params[4])))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandSetStringLength(AMX *, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command || !command->setStringLength(params[2], params[3], params[4]))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandAddChannelType(AMX *, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command || !command->addChannelType(params[2], params[3]))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandSetPermissions(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string permissions;
            if (!GetPawnString(amx, params[2], permissions, false))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command)
                return 0;
            command->setDefaultMemberPermissions(std::move(permissions));
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandSetNsfw(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command)
                return 0;
            command->setNsfw(params[2] != 0);
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandAddContext(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command || !command->addContext(params[2]))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandAddIntegrationType(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            auto *core = GetCore();
            auto *command = core ? core->getCommandManager().get(params[1]) : nullptr;
            if (!command || !command->addIntegrationType(params[2]))
                return 0;
            core->getCommandManager().markDirty();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_SetCommandGuild(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string guildId;
            if (!GetPawnString(amx, params[1], guildId, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            core->getCommandManager().setGuild(guildId, params[2] != 0);
            return 1;
        }

        cell AMX_NATIVE_CALL Native_DeployCommands(AMX *amx, cell *params)
        {
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            string guildId = core->getCommandManager().guildId();
            if (HasParams(params, 1))
            {
                string supplied;
                if (GetPawnString(amx, params[1], supplied) && !supplied.empty())
                    guildId = std::move(supplied);
            }
            if (guildId.empty())
                return 0;
            const bool queued = core->getDiscordClient().deployCommands(guildId, core->getCommandManager().toJson());
            if (queued)
                core->getCommandManager().markDeployed();
            return queued ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ClearCommands(AMX *, cell *)
        {
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            core->getCommandManager().clear();
            return 1;
        }

        cell AMX_NATIVE_CALL Native_CommandGetValue(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3) || params[3] <= 0)
                return 0;
            string name;
            if (!GetPawnString(amx, params[1], name, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            string value;
            if (!core->getPawnRuntime().getCommandValue(name, value))
                return 0;
            return SetPawnString(amx, params[2], params[3], value) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_RespondAutocomplete(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            string id, token, choices;
            if (!GetPawnString(amx, params[1], id, false) || !GetPawnString(amx, params[2], token, false) || !GetPawnString(amx, params[3], choices, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->respondAutocomplete(id, token, choices) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_CreateV2(AMX *, cell *)
        {
            BridgeCore *core = GetCore();
            return core ? static_cast<cell>(core->getV2Manager().create()) : 0;
        }

        cell AMX_NATIVE_CALL Native_DestroyV2(AMX *, cell *params)
        {
            if (!HasParams(params, 1) || params[1] <= 0)
                return 0;
            BridgeCore *core = GetCore();
            return core && core->getV2Manager().destroy(static_cast<V2Handle>(params[1])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddText(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[1]));
            if (!v2)
                return 0;
            string text;
            if (!GetPawnString(amx, params[2], text, false))
                return 0;
            return v2->addText(text) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddSeparator(AMX *, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[1]));
            if (!v2)
                return 0;
            return v2->addSeparator(params[2] != 0, static_cast<int>(params[3])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddContainer(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[1]));
            if (!v2)
                return 0;
            string text;
            if (!GetPawnString(amx, params[2], text, false))
                return 0;
            return v2->addContainer(text, static_cast<int>(params[3]), params[4] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddSection(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[1]));
            if (!v2)
                return 0;
            string text, url;
            if (!GetPawnString(amx, params[2], text, false) || !GetPawnString(amx, params[3], url, false))
                return 0;
            return v2->addSection(text, url) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddSectionButton(AMX *amx, cell *params)
        {
            if (!HasParams(params, 6))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[1]));
            if (!v2)
                return 0;
            string text, label, customId;
            if (!GetPawnString(amx, params[2], text, false) || !GetPawnString(amx, params[3], label, false) || !GetPawnString(amx, params[4], customId, false))
                return 0;
            return v2->addSectionButton(text, label, customId, static_cast<int>(params[5]), params[6] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddMedia(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[1]));
            if (!v2)
                return 0;
            string url, description;
            if (!GetPawnString(amx, params[2], url, false) || !GetPawnString(amx, params[3], description))
                return 0;
            return v2->addMedia(url, description, params[4] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddActionRow(AMX *, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[1]));
            const ActionRow *row = core->getActionRowManager().get(static_cast<ActionRowHandle>(params[2]));
            if (!v2 || !row)
                return 0;
            const string json = row->toJson(core->getButtonManager(), &core->getSelectMenuManager());
            return v2->addActionRow(json) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2AddFile(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            string name;
            return v2 && GetPawnString(amx, params[2], name, false) && v2->addFile(name, params[3] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2CreateContainer(AMX *, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            return v2 ? static_cast<cell>(v2->createContainer(params[2], params[3] != 0)) : 0;
        }

        cell AMX_NATIVE_CALL Native_V2ContainerAddText(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            string text;
            return v2 && GetPawnString(amx, params[3], text, false) && v2->containerAddText(params[2], text) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2ContainerAddSeparator(AMX *, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            return v2 && v2->containerAddSeparator(params[2], params[3] != 0, params[4]) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2ContainerAddActionRow(AMX *, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            auto *core = GetCore();
            if (!core)
                return 0;
            auto *v2 = core->getV2Manager().get(params[1]);
            auto *row = core->getActionRowManager().get(params[3]);
            if (!v2 || !row)
                return 0;
            return v2->containerAddActionRow(params[2], row->toJson(core->getButtonManager(), &core->getSelectMenuManager())) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2ContainerAddMedia(AMX *amx, cell *params)
        {
            if (!HasParams(params, 5))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            string url, description;
            return v2 && GetPawnString(amx, params[3], url, false) && GetPawnString(amx, params[4], description) && v2->containerAddMedia(params[2], url, description, params[5] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2CreateSection(AMX *, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            return v2 ? static_cast<cell>(v2->createSection()) : 0;
        }

        cell AMX_NATIVE_CALL Native_V2SectionAddText(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            string text;
            return v2 && GetPawnString(amx, params[3], text, false) && v2->sectionAddText(params[2], text) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2SectionSetThumbnail(AMX *amx, cell *params)
        {
            if (!HasParams(params, 5))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            string url, description;
            return v2 && GetPawnString(amx, params[3], url, false) && GetPawnString(amx, params[4], description) && v2->sectionSetThumbnail(params[2], url, description, params[5] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_V2SectionSetButton(AMX *amx, cell *params)
        {
            if (!HasParams(params, 6))
                return 0;
            auto *core = GetCore();
            auto *v2 = core ? core->getV2Manager().get(params[1]) : nullptr;
            string label, customId;
            return v2 && GetPawnString(amx, params[3], label, false) && GetPawnString(amx, params[4], customId, false) && v2->sectionSetButton(params[2], label, customId, params[5], params[6] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_SendV2(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string channelId;
            if (!GetPawnString(amx, params[1], channelId, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            const ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[2]));
            if (!v2 || v2->empty())
                return 0;
            return core->getDiscordClient().sendV2(channelId, v2->messageJson()) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_EditV2(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            string channelId, messageId;
            if (!GetPawnString(amx, params[1], channelId, false) || !GetPawnString(amx, params[2], messageId, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            const ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[3]));
            if (!v2 || v2->empty())
                return 0;
            return core->getDiscordClient().editV2(channelId, messageId, v2->messageJson()) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_RespondV2(AMX *amx, cell *params)
        {
            if (!HasParams(params, 4))
                return 0;
            string interactionId, token;
            if (!GetPawnString(amx, params[1], interactionId, false) || !GetPawnString(amx, params[2], token, false))
                return 0;
            BridgeCore *core = GetCore();
            if (!core)
                return 0;
            const ComponentsV2 *v2 = core->getV2Manager().get(static_cast<V2Handle>(params[3]));
            if (!v2 || v2->empty())
                return 0;
            return core->getDiscordClient().respondV2(interactionId, token, v2->componentsJson(), params[4] != 0) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_FetchGuild(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            string id;
            if (!GetPawnString(amx, params[1], id, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->fetchGuild(id) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_FetchChannel(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            string id;
            if (!GetPawnString(amx, params[1], id, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->fetchChannel(id) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_FetchRole(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string guildId, roleId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], roleId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->fetchRole(guildId, roleId) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_FetchMember(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string guildId, userId;
            if (!GetPawnString(amx, params[1], guildId, false) || !GetPawnString(amx, params[2], userId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->fetchMember(guildId, userId) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_FetchUser(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            string userId;
            if (!GetPawnString(amx, params[1], userId, false))
                return 0;
            auto *discord = GetDiscord();
            return discord && discord->fetchUser(userId) ? 1 : 0;
        }

        bool SetDataString(AMX *amx, cell *params, int idCount, const std::function<bool(DiscordDataStore &, const std::vector<string> &, string &)> &getter)
        {
            if (!HasParams(params, idCount + 2) || params[idCount + 2] <= 0)
                return false;
            auto *d = GetDiscord();
            if (!d)
                return false;
            std::vector<string> ids;
            for (int i = 0; i < idCount; ++i)
            {
                string id;
                if (!GetPawnString(amx, params[i + 1], id, false))
                    return false;
                ids.push_back(std::move(id));
            }
            string value;
            if (!getter(d->getDataStore(), ids, value))
                return false;
            return SetPawnString(amx, params[idCount + 1], params[idCount + 2], value);
        }

        cell AMX_NATIVE_CALL Native_GetGuildName(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getGuildName(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetGuildOwner(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getGuildOwner(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetGuildIcon(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getGuildIcon(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetGuildIconUrl(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getGuildIconUrl(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetGuildBanner(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getGuildBanner(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetGuildBannerUrl(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getGuildBannerUrl(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetGuildDescription(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getGuildDescription(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetChannelName(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getChannelName(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetChannelTopic(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getChannelTopic(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetChannelParent(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getChannelParent(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetChannelGuild(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getChannelGuild(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetRoleName(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getRoleName(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetRolePermissions(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getRolePermissions(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetMemberNick(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getMemberNick(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetMemberName(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getMemberName(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetMemberGlobal(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getMemberGlobalName(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetMemberAvatar(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getMemberAvatar(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetMemberJoinedAt(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getMemberJoinedAt(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetMemberAvatarUrl(AMX *a, cell *p)
        {
            return SetDataString(a, p, 2, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getMemberAvatarUrl(i[0], i[1], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetUserName(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getUserName(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetUserGlobal(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getUserGlobalName(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetUserAvatar(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getUserAvatar(i[0], v); })
                       ? 1
                       : 0;
        }
        cell AMX_NATIVE_CALL Native_GetUserAvatarUrl(AMX *a, cell *p)
        {
            return SetDataString(a, p, 1, [](DiscordDataStore &d, const std::vector<string> &i, string &v)
                                 { return d.getUserAvatarUrl(i[0], v); })
                       ? 1
                       : 0;
        }

        cell AMX_NATIVE_CALL Native_GetGuildMembers(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            string g;
            if (!GetPawnString(amx, params[1], g, false))
                return 0;
            auto *d = GetDiscord();
            int v = 0;
            return d && d->getDataStore().getGuildMemberCount(g, v) ? static_cast<cell>(v) : 0;
        }
        cell AMX_NATIVE_CALL Native_GetChannelType(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return -1;
            string c;
            if (!GetPawnString(amx, params[1], c, false))
                return -1;
            auto *d = GetDiscord();
            int v = -1;
            return d && d->getDataStore().getChannelType(c, v) ? static_cast<cell>(v) : -1;
        }
        cell AMX_NATIVE_CALL Native_GetRoleColor(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return -1;
            string g, r;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], r, false))
                return -1;
            auto *d = GetDiscord();
            int v = -1;
            return d && d->getDataStore().getRoleColor(g, r, v) ? static_cast<cell>(v) : -1;
        }
        cell AMX_NATIVE_CALL Native_GetRolePosition(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return -1;
            string g, r;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], r, false))
                return -1;
            auto *d = GetDiscord();
            int v = -1;
            return d && d->getDataStore().getRolePosition(g, r, v) ? static_cast<cell>(v) : -1;
        }
        cell AMX_NATIVE_CALL Native_GetRoleHoist(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string g, r;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], r, false))
                return 0;
            auto *d = GetDiscord();
            bool v = false;
            return d && d->getDataStore().getRoleHoist(g, r, v) && v ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_GetRoleMentionable(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string g, r;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], r, false))
                return 0;
            auto *d = GetDiscord();
            bool v = false;
            return d && d->getDataStore().getRoleMentionable(g, r, v) && v ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_MemberHasRole(AMX *amx, cell *params)
        {
            if (!HasParams(params, 3))
                return 0;
            string g, u, r;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], u, false) || !GetPawnString(amx, params[3], r, false))
                return 0;
            auto *d = GetDiscord();
            return d && d->getDataStore().memberHasRole(g, u, r) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_IsUserBot(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            string u;
            if (!GetPawnString(amx, params[1], u, false))
                return 0;
            auto *d = GetDiscord();
            bool v = false;
            return d && d->getDataStore().isUserBot(u, v) && v ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_GetChannelPosition(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return -1;
            string c;
            if (!GetPawnString(amx, params[1], c, false))
                return -1;
            auto *d = GetDiscord();
            int v = -1;
            return d && d->getDataStore().getChannelPosition(c, v) ? static_cast<cell>(v) : -1;
        }
        cell AMX_NATIVE_CALL Native_GetChannelNsfw(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return 0;
            string c;
            if (!GetPawnString(amx, params[1], c, false))
                return 0;
            auto *d = GetDiscord();
            bool v = false;
            return d && d->getDataStore().getChannelNsfw(c, v) && v ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_GetChannelSlowmode(AMX *amx, cell *params)
        {
            if (!HasParams(params, 1))
                return -1;
            string c;
            if (!GetPawnString(amx, params[1], c, false))
                return -1;
            auto *d = GetDiscord();
            int v = -1;
            return d && d->getDataStore().getChannelSlowmode(c, v) ? static_cast<cell>(v) : -1;
        }
        cell AMX_NATIVE_CALL Native_GetRoleManaged(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return 0;
            string g, r;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], r, false))
                return 0;
            auto *d = GetDiscord();
            bool v = false;
            return d && d->getDataStore().getRoleManaged(g, r, v) && v ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_GetMemberRoleCount(AMX *amx, cell *params)
        {
            if (!HasParams(params, 2))
                return -1;
            string g, u;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], u, false))
                return -1;
            auto *d = GetDiscord();
            int v = -1;
            return d && d->getDataStore().getMemberRoleCount(g, u, v) ? static_cast<cell>(v) : -1;
        }
        cell AMX_NATIVE_CALL Native_GetMemberRole(AMX *amx, cell *params)
        {
            if (!HasParams(params, 5) || params[5] <= 0)
                return 0;
            string g, u;
            if (!GetPawnString(amx, params[1], g, false) || !GetPawnString(amx, params[2], u, false))
                return 0;
            auto *d = GetDiscord();
            string v;
            return d && d->getDataStore().getMemberRole(g, u, static_cast<int>(params[3]), v) && SetPawnString(amx, params[4], params[5], v) ? 1 : 0;
        }

        Component *GetComponent(cell handle)
        {
            auto *core = GetCore();
            return core && handle > 0 ? core->getComponentManager().get(static_cast<ComponentHandle>(handle)) : nullptr;
        }

        cell AMX_NATIVE_CALL Native_CreateComponent(AMX *, cell *params)
        {
            auto *core = GetCore();
            return core && HasParams(params, 1) ? static_cast<cell>(core->getComponentManager().create(static_cast<int>(params[1]))) : 0;
        }

        cell AMX_NATIVE_CALL Native_DestroyComponent(AMX *, cell *params)
        {
            auto *core = GetCore();
            return core && HasParams(params, 1) && core->getComponentManager().destroy(static_cast<ComponentHandle>(params[1])) ? 1 : 0;
        }

        cell AMX_NATIVE_CALL Native_ComponentSetId(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v, false) && c->setCustomId(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetLabel(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v, false) && c->setLabel(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetDesc(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v) && c->setDescription(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetContent(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v, false) && c->setContent(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetPlaceholder(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v) && c->setPlaceholder(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetValue(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v) && c->setValue(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetUrl(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v, false) && c->setUrl(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetEmoji(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v) && c->setEmoji(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetSku(AMX *amx, cell *params)
        {
            string v;
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && GetPawnString(amx, params[2], v, false) && c->setSkuId(v) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetStyle(AMX *, cell *params)
        {
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            return c && c->setStyle(static_cast<int>(params[2])) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetDisabled(AMX *, cell *params)
        {
            auto *c = HasParams(params, 2) ? GetComponent(params[1]) : nullptr;
            if (!c)
                return 0;
            c->setDisabled(params[2] != 0);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetRequired(AMX *, cell *p)
        {
            auto *c = HasParams(p, 2) ? GetComponent(p[1]) : nullptr;
            if (!c)
                return 0;
            c->setRequired(p[2] != 0);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetRange(AMX *, cell *p)
        {
            auto *c = HasParams(p, 3) ? GetComponent(p[1]) : nullptr;
            return c && c->setRange((int)p[2], (int)p[3]) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetLength(AMX *, cell *p)
        {
            auto *c = HasParams(p, 3) ? GetComponent(p[1]) : nullptr;
            return c && c->setLength((int)p[2], (int)p[3]) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetAccent(AMX *, cell *p)
        {
            auto *c = HasParams(p, 2) ? GetComponent(p[1]) : nullptr;
            return c && c->setAccentColor((int)p[2]) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetSpoiler(AMX *, cell *p)
        {
            auto *c = HasParams(p, 2) ? GetComponent(p[1]) : nullptr;
            if (!c)
                return 0;
            c->setSpoiler(p[2] != 0);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetDivider(AMX *, cell *p)
        {
            auto *c = HasParams(p, 2) ? GetComponent(p[1]) : nullptr;
            if (!c)
                return 0;
            c->setDivider(p[2] != 0);
            return 1;
        }
        cell AMX_NATIVE_CALL Native_ComponentSetSpacing(AMX *, cell *p)
        {
            auto *c = HasParams(p, 2) ? GetComponent(p[1]) : nullptr;
            return c && c->setSpacing((int)p[2]) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentAddOption(AMX *amx, cell *params)
        {
            auto *c = HasParams(params, 4) ? GetComponent(params[1]) : nullptr;
            string l, v, d;
            return c && GetPawnString(amx, params[2], l, false) && GetPawnString(amx, params[3], v, false) && GetPawnString(amx, params[4], d) && c->addOption(l, v, d) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentAddOptionEx(AMX *a, cell *p)
        {
            auto *c = HasParams(p, 6) ? GetComponent(p[1]) : nullptr;
            string l, v, d, e;
            return c && GetPawnString(a, p[2], l, false) && GetPawnString(a, p[3], v, false) && GetPawnString(a, p[4], d) && GetPawnString(a, p[5], e) && c->addOption(l, v, d, e, p[6] != 0) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentAddMedia(AMX *a, cell *p)
        {
            auto *c = HasParams(p, 4) ? GetComponent(p[1]) : nullptr;
            string u, d;
            return c && GetPawnString(a, p[2], u, false) && GetPawnString(a, p[3], d) && c->addMedia(u, d, p[4] != 0) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentAddChanType(AMX *, cell *p)
        {
            auto *c = HasParams(p, 2) ? GetComponent(p[1]) : nullptr;
            return c && c->addChannelType((int)p[2]) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentAdd(AMX *, cell *params)
        {
            auto *core = GetCore();
            auto *parent = core && HasParams(params, 2) ? core->getComponentManager().get(static_cast<ComponentHandle>(params[1])) : nullptr;
            auto *child = core ? core->getComponentManager().get(static_cast<ComponentHandle>(params[2])) : nullptr;
            return parent && child && parent->addChild(static_cast<ComponentHandle>(params[2]), core->getComponentManager()) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_ComponentAccessory(AMX *, cell *p)
        {
            auto *core = GetCore();
            auto *section = core && HasParams(p, 2) ? core->getComponentManager().get((ComponentHandle)p[1]) : nullptr;
            return section && section->setAccessory((ComponentHandle)p[2], core->getComponentManager()) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_SendComponent(AMX *amx, cell *params)
        {
            auto *core = GetCore();
            if (!core || !HasParams(params, 2))
                return 0;
            string channel;
            if (!GetPawnString(amx, params[1], channel, false))
                return 0;
            auto *c = core->getComponentManager().get(static_cast<ComponentHandle>(params[2]));
            if (!c)
                return 0;
            const string json = c->toJson(core->getComponentManager());
            if (c->type() == 17 || c->type() == 9 || c->type() == 10 || c->type() == 14)
                return core->getDiscordClient().sendV2(channel, "{\"flags\":32768,\"components\":[" + json + "]}") ? 1 : 0;
            return core->getDiscordClient().sendComponents(channel, json) ? 1 : 0;
        }
        cell AMX_NATIVE_CALL Native_GetDroppedEvents(AMX *, cell *) { return static_cast<cell>(GlobalMetrics().droppedGatewayEvents.load()); }
        cell AMX_NATIVE_CALL Native_GetDroppedRequests(AMX *, cell *) { return static_cast<cell>(GlobalMetrics().droppedRestRequests.load() + GlobalMetrics().droppedInteractionRequests.load()); }
        cell AMX_NATIVE_CALL Native_GetProcessedEvents(AMX *, cell *) { return static_cast<cell>(GlobalMetrics().processedEvents.load()); }

        cell AMX_NATIVE_CALL Native_GetVersionMajor(AMX *, cell *)
        {
            return VERSION_MAJOR;
        }

        cell AMX_NATIVE_CALL Native_GetVersionMinor(AMX *, cell *)
        {
            return VERSION_MINOR;
        }

        cell AMX_NATIVE_CALL Native_GetVersionPatch(AMX *, cell *)
        {
            return VERSION_PATCH;
        }

        AMX_NATIVE_INFO natives[] =
            {
                {"DBridge_Connect", Native_Connect},
                {"DBridge_Disconnect", Native_Disconnect},

                {"DBridge_SetStatus", Native_SetStatus},
                {"DBridge_SetActivity", Native_SetActivity},
                {"DBridge_ClearActivity", Native_ClearActivity},
                {"DBridge_SetPresence", Native_SetPresence},

                {"DBridge_SendMessage", Native_SendMessage},
                {"DBridge_EditMessage", Native_EditMessage},
                {"DBridge_DeleteMessage", Native_DeleteMessage},

                {"DBridge_CreateEmbed", Native_CreateEmbed},
                {"DBridge_DestroyEmbed", Native_DestroyEmbed},
                {"DBridge_EmbedSetTitle", Native_EmbedSetTitle},
                {"DBridge_EmbedSetDescription", Native_EmbedSetDescription},
                {"DBridge_EmbedSetUrl", Native_EmbedSetUrl},
                {"DBridge_EmbedSetColor", Native_EmbedSetColor},
                {"DBridge_EmbedSetAuthor", Native_EmbedSetAuthor},
                {"DBridge_EmbedClearAuthor", Native_EmbedClearAuthor},
                {"DBridge_EmbedSetThumbnail", Native_EmbedSetThumbnail},
                {"DBridge_EmbedClearThumbnail", Native_EmbedClearThumbnail},
                {"DBridge_EmbedSetImage", Native_EmbedSetImage},
                {"DBridge_EmbedClearImage", Native_EmbedClearImage},
                {"DBridge_EmbedSetFooter", Native_EmbedSetFooter},
                {"DBridge_EmbedClearFooter", Native_EmbedClearFooter},
                {"DBridge_EmbedAddField", Native_EmbedAddField},
                {"DBridge_EmbedClearFields", Native_EmbedClearFields},
                {"DBridge_EmbedClear", Native_EmbedClear},
                {"DBridge_SendEmbed", Native_SendEmbed},

                {"DBridge_CreateButton", Native_CreateButton},
                {"DBridge_DestroyButton", Native_DestroyButton},
                {"DBridge_ButtonSetLabel", Native_ButtonSetLabel},
                {"DBridge_ButtonSetCustomId", Native_ButtonSetCustomId},
                {"DBridge_ButtonSetUrl", Native_ButtonSetUrl},
                {"DBridge_ButtonSetEmoji", Native_ButtonSetEmoji},
                {"DBridge_ButtonSetEmojiEx", Native_ButtonSetEmojiEx},
                {"DBridge_ButtonSetSkuId", Native_ButtonSetSkuId},
                {"DBridge_ButtonSetComponentId", Native_ButtonSetComponentId},
                {"DBridge_ButtonSetStyle", Native_ButtonSetStyle},
                {"DBridge_ButtonSetDisabled", Native_ButtonSetDisabled},
                {"DBridge_ButtonClear", Native_ButtonClear},

                {"DBridge_CreateActionRow", Native_CreateActionRow},
                {"DBridge_DestroyActionRow", Native_DestroyActionRow},
                {"DBridge_ActionRowAddButton", Native_ActionRowAddButton},
                {"DBridge_ActionRowRemoveButton", Native_ActionRowRemoveButton},
                {"DBridge_ActionRowClear", Native_ActionRowClear},
                {"DBridge_SendComponents", Native_SendComponents},

                {"DBridge_CreateSelectMenu", Native_CreateSelectMenu},
                {"DBridge_CreateSelectMenuType", Native_CreateSelectMenuType},
                {"DBridge_DestroySelectMenu", Native_DestroySelectMenu},
                {"DBridge_SelectMenuSetCustomId", Native_SelectMenuSetCustomId},
                {"DBridge_SelectMenuSetType", Native_SelectMenuSetType},
                {"DBridge_SelectAddChannelType", Native_SelectMenuAddChannelType},
                {"DBridge_SelectSetPlaceholder", Native_SelectSetPlaceholder},
                {"DBridge_SelectMenuSetRange", Native_SelectMenuSetRange},
                {"DBridge_SelectMenuSetDisabled", Native_SelectMenuSetDisabled},
                {"DBridge_SelectMenuAddOption", Native_SelectMenuAddOption},
                {"DBridge_ActionRowSetSelectMenu", Native_ActionRowSetSelectMenu},
                {"DBridge_CreateModal", Native_CreateModal},
                {"DBridge_DestroyModal", Native_DestroyModal},
                {"DBridge_ModalSetCustomId", Native_ModalSetCustomId},
                {"DBridge_ModalSetTitle", Native_ModalSetTitle},
                {"DBridge_ModalAddTextInput", Native_ModalAddTextInput},
                {"DBridge_ModalAddSelectMenu", Native_ModalAddSelectMenu},
                {"DBridge_ModalAddFileUpload", Native_ModalAddFileUpload},
                {"DBridge_ModalAddTextDisplay", Native_ModalAddTextDisplay},
                {"DBridge_ModalAddComponent", Native_ModalAddComponent},
                {"DBridge_ShowModal", Native_ShowModal},
                {"DBridge_ModalGetValue", Native_ModalGetValue},
                {"DBridge_RespondInteraction", Native_RespondInteraction},
                {"DBridge_InteractionValueCount", Native_InteractionValueCount},
                {"DBridge_InteractionGetValue", Native_InteractionGetValue},
                {"DBridge_InteractionGetField", Native_InteractionGetField},

                {"DBridge_CreateChannel", Native_CreateChannel},
                {"DBridge_DeleteChannel", Native_DeleteChannel},
                {"DBridge_CreateRole", Native_CreateRole},
                {"DBridge_DeleteRole", Native_DeleteRole},
                {"DBridge_AddMemberRole", Native_AddMemberRole},
                {"DBridge_RemoveMemberRole", Native_RemoveMemberRole},
                {"DBridge_KickMember", Native_KickMember},
                {"DBridge_BanMember", Native_BanMember},
                {"DBridge_UnbanMember", Native_UnbanMember},

                {"DBridge_CreateCommand", Native_CreateCommand},
                {"DBridge_DestroyCommand", Native_DestroyCommand},
                {"DBridge_CommandAddOption", Native_CommandAddOption},
                {"DBridge_CommandAddOptionEx", Native_CommandAddOptionEx},
                {"DBridge_CommandAddChoice", Native_CommandAddChoice},
                {"DBridge_CommandAddNumericChoice", Native_CommandAddNumericChoice},
                {"DBridge_CommandSetAutocomplete", Native_CommandSetAutocomplete},
                {"DBridge_CommandSetNumberRange", Native_CommandSetNumberRange},
                {"DBridge_CommandSetStringLength", Native_CommandSetStringLength},
                {"DBridge_CommandAddChannelType", Native_CommandAddChannelType},
                {"DBridge_CommandSetPermissions", Native_CommandSetPermissions},
                {"DBridge_CommandSetNsfw", Native_CommandSetNsfw},
                {"DBridge_CommandAddContext", Native_CommandAddContext},
                {"DBridge_CommandAddIntegration", Native_CommandAddIntegrationType},
                {"DBridge_SetCommandGuild", Native_SetCommandGuild},
                {"DBridge_DeployCommands", Native_DeployCommands},
                {"DBridge_ClearCommands", Native_ClearCommands},
                {"DBridge_CommandGetValue", Native_CommandGetValue},
                {"DBridge_CommandGetString", Native_CommandGetValue},
                {"DBridge_CommandGetUser", Native_CommandGetValue},
                {"DBridge_CommandGetChannel", Native_CommandGetValue},
                {"DBridge_CommandGetRole", Native_CommandGetValue},
                {"DBridge_CommandGetBoolean", Native_CommandGetValue},
                {"DBridge_CommandGetInteger", Native_CommandGetValue},
                {"DBridge_CommandGetNumber", Native_CommandGetValue},
                {"DBridge_CommandGetMentionable", Native_CommandGetValue},
                {"DBridge_CommandGetAttachment", Native_CommandGetValue},
                {"DBridge_CommandGetSubcommand", Native_CommandGetValue},
                {"DBridge_CommandGetGroup", Native_CommandGetValue},
                {"DBridge_RespondAutocomplete", Native_RespondAutocomplete},

                {"DBridge_CreateV2", Native_CreateV2},
                {"DBridge_DestroyV2", Native_DestroyV2},
                {"DBridge_V2AddText", Native_V2AddText},
                {"DBridge_V2AddSeparator", Native_V2AddSeparator},
                {"DBridge_V2AddContainer", Native_V2AddContainer},
                {"DBridge_V2AddSection", Native_V2AddSection},
                {"DBridge_V2AddSectionButton", Native_V2AddSectionButton},
                {"DBridge_V2AddMedia", Native_V2AddMedia},
                {"DBridge_V2AddActionRow", Native_V2AddActionRow},
                {"DBridge_V2AddFile", Native_V2AddFile},
                {"DBridge_V2CreateContainer", Native_V2CreateContainer},
                {"DBridge_V2ContainerAddText", Native_V2ContainerAddText},
                {"DBridge_V2ContainerAddSeparator", Native_V2ContainerAddSeparator},
                {"DBridge_V2ContainerAddActionRow", Native_V2ContainerAddActionRow},
                {"DBridge_V2ContainerAddMedia", Native_V2ContainerAddMedia},
                {"DBridge_V2CreateSection", Native_V2CreateSection},
                {"DBridge_V2SectionAddText", Native_V2SectionAddText},
                {"DBridge_V2SectionSetThumbnail", Native_V2SectionSetThumbnail},
                {"DBridge_V2SectionSetButton", Native_V2SectionSetButton},
                {"DBridge_SendV2", Native_SendV2},
                {"DBridge_EditV2", Native_EditV2},
                {"DBridge_RespondV2", Native_RespondV2},

                {"DBridge_FetchGuild", Native_FetchGuild},
                {"DBridge_FetchChannel", Native_FetchChannel},
                {"DBridge_FetchRole", Native_FetchRole},
                {"DBridge_FetchMember", Native_FetchMember},
                {"DBridge_FetchUser", Native_FetchUser},
                {"DBridge_GetGuildName", Native_GetGuildName},
                {"DBridge_GetGuildOwner", Native_GetGuildOwner},
                {"DBridge_GetGuildIcon", Native_GetGuildIcon},
                {"DBridge_GetGuildIconUrl", Native_GetGuildIconUrl},
                {"DBridge_GetGuildBanner", Native_GetGuildBanner},
                {"DBridge_GetGuildBannerUrl", Native_GetGuildBannerUrl},
                {"DBridge_GetGuildDescription", Native_GetGuildDescription},
                {"DBridge_GetGuildMembers", Native_GetGuildMembers},
                {"DBridge_GetChannelName", Native_GetChannelName},
                {"DBridge_GetChannelType", Native_GetChannelType},
                {"DBridge_GetChannelTopic", Native_GetChannelTopic},
                {"DBridge_GetChannelParent", Native_GetChannelParent},
                {"DBridge_GetChannelGuild", Native_GetChannelGuild},
                {"DBridge_GetChannelPosition", Native_GetChannelPosition},
                {"DBridge_GetChannelNsfw", Native_GetChannelNsfw},
                {"DBridge_GetChannelSlowmode", Native_GetChannelSlowmode},
                {"DBridge_GetRoleName", Native_GetRoleName},
                {"DBridge_GetRoleColor", Native_GetRoleColor},
                {"DBridge_GetRolePosition", Native_GetRolePosition},
                {"DBridge_GetRoleHoist", Native_GetRoleHoist},
                {"DBridge_GetRoleMentionable", Native_GetRoleMentionable},
                {"DBridge_GetRolePermissions", Native_GetRolePermissions},
                {"DBridge_GetRoleManaged", Native_GetRoleManaged},
                {"DBridge_GetMemberNick", Native_GetMemberNick},
                {"DBridge_GetMemberName", Native_GetMemberName},
                {"DBridge_GetMemberGlobalName", Native_GetMemberGlobal},
                {"DBridge_GetMemberAvatar", Native_GetMemberAvatar},
                {"DBridge_GetMemberAvatarUrl", Native_GetMemberAvatarUrl},
                {"DBridge_GetMemberJoinedAt", Native_GetMemberJoinedAt},
                {"DBridge_GetMemberRoleCount", Native_GetMemberRoleCount},
                {"DBridge_GetMemberRole", Native_GetMemberRole},
                {"DBridge_MemberHasRole", Native_MemberHasRole},
                {"DBridge_GetUserName", Native_GetUserName},
                {"DBridge_GetUserGlobal", Native_GetUserGlobal},
                {"DBridge_GetUserGlobalName", Native_GetUserGlobal},
                {"DBridge_GetUserAvatar", Native_GetUserAvatar},
                {"DBridge_GetUserAvatarUrl", Native_GetUserAvatarUrl},
                {"DBridge_IsUserBot", Native_IsUserBot},

                {"DBridge_CreateComponent", Native_CreateComponent},
                {"DBridge_DestroyComponent", Native_DestroyComponent},
                {"DBridge_ComponentSetId", Native_ComponentSetId},
                {"DBridge_ComponentSetLabel", Native_ComponentSetLabel},
                {"DBridge_ComponentSetDesc", Native_ComponentSetDesc},
                {"DBridge_ComponentSetContent", Native_ComponentSetContent},
                {"DBridge_ComponentSetPlaceholder", Native_ComponentSetPlaceholder},
                {"DBridge_ComponentSetValue", Native_ComponentSetValue},
                {"DBridge_ComponentSetUrl", Native_ComponentSetUrl},
                {"DBridge_ComponentSetEmoji", Native_ComponentSetEmoji},
                {"DBridge_ComponentSetSku", Native_ComponentSetSku},
                {"DBridge_ComponentSetStyle", Native_ComponentSetStyle},
                {"DBridge_ComponentSetDisabled", Native_ComponentSetDisabled},
                {"DBridge_ComponentSetRequired", Native_ComponentSetRequired},
                {"DBridge_ComponentSetRange", Native_ComponentSetRange},
                {"DBridge_ComponentSetLength", Native_ComponentSetLength},
                {"DBridge_ComponentSetAccent", Native_ComponentSetAccent},
                {"DBridge_ComponentSetSpoiler", Native_ComponentSetSpoiler},
                {"DBridge_ComponentSetDivider", Native_ComponentSetDivider},
                {"DBridge_ComponentSetSpacing", Native_ComponentSetSpacing},
                {"DBridge_ComponentAddOption", Native_ComponentAddOption},
                {"DBridge_ComponentAddOptionEx", Native_ComponentAddOptionEx},
                {"DBridge_ComponentAddMedia", Native_ComponentAddMedia},
                {"DBridge_ComponentAddChanType", Native_ComponentAddChanType},
                {"DBridge_ComponentAdd", Native_ComponentAdd},
                {"DBridge_ComponentAccessory", Native_ComponentAccessory},
                {"DBridge_SendComponent", Native_SendComponent},
                {"DBridge_GetDroppedEvents", Native_GetDroppedEvents},
                {"DBridge_GetDroppedRequests", Native_GetDroppedRequests},
                {"DBridge_GetProcessedEvents", Native_GetProcessedEvents},

                {"DBridge_GetVersionMajor", Native_GetVersionMajor},
                {"DBridge_GetVersionMinor", Native_GetVersionMinor},
                {"DBridge_GetVersionPatch", Native_GetVersionPatch},

                {nullptr, nullptr}};
    }

    int RegisterNatives(AMX *amx)
    {
        if (!amx)
            return AMX_ERR_PARAMS;

        return amx_Register(amx, natives, -1);
    }
}