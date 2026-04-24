#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "havok/hks_api.hpp"
#include "havok/lua_api.hpp"

#define UUID_FILE "uuid"

#define VERSION_TUPLE {0,6,5}

#include <apclient.hpp>
#include <apuuid.hpp>
#include "../game/game.hpp"

//TMP
#include <windows.h>

using nlohmann::json;

namespace archipelago
{
	const std::string GAME_NAME = "Black Ops 3 - Zombies";
	
	//Full Remote Items
	const int items_handling = 0b111;

	const int64_t loc_error = (int64_t)(-1);
	
	APClient* ap = nullptr;

	//For the Lobby Connection
	APClient* temp_ap = nullptr;

	bool socket_connected = false;

	bool ap_sync_queued = false;

	bool awaitingReconnect = false;

	bool first_item_call = true;

	int baseID = 0;

	int lastItem = 0;

	std::set<double> deathLinkTimestamps;

	std::string slot = "";
	std::string seed = "";
	bool deathLinkEnabled = false;
	double lastReceivedDeathlinkTime = 0.0;

	struct DvarSetting {
		std::string jsonName;
		std::string dvarName;
		enum class Type { String, Int, Bool, StrArray } type;
	};

	struct DvarSetting slot_settings[] = {
		{"map_specific_machines", "ARCHIPELAGO_MAP_SPECIFIC_MACHINES", DvarSetting::Type::Bool},
		{"perk_limit_default_modifier", "ARCHIPELAGO_PERK_LIMIT_DEFAULT_MODIFIER", DvarSetting::Type::Int},
		{"randomized_shield_parts", "ARCHIPELAGO_RANDOMIZED_SHIELD_PARTS", DvarSetting::Type::Bool},
		{"mystery_box_special_items", "ARCHIPELAGO_MYSTERY_BOX_SPECIAL", DvarSetting::Type::Bool},
		{"mystery_box_regular_items", "ARCHIPELAGO_MYSTERY_BOX_REGULAR", DvarSetting::Type::Bool},
		{"mystery_box_expanded", "ARCHIPELAGO_MYSTERY_BOX_EXPANDED", DvarSetting::Type::Bool},
		{"difficulty_rng_moon_digger", "ARCHIPELAGO_DIFFICULTY_RNG_MOON_DIGGER", DvarSetting::Type::Bool},
		{"difficulty_rng_moon_box", "ARCHIPELAGO_DIFFICULTY_RNG_MOON_BOX", DvarSetting::Type::Bool},
		{"difficulty_gorod_egg_cooldown", "ARCHIPELAGO_DIFFICULTY_GOROD_EGG_COOLDOWN", DvarSetting::Type::Bool},
		{"difficulty_gorod_dragon_wings", "ARCHIPELAGO_DIFFICULTY_GOROD_DRAGON_WINGS", DvarSetting::Type::Bool},
		{"difficulty_ee_checkpoints", "ARCHIPELAGO_DIFFICULTY_EE_CHECKPOINTS", DvarSetting::Type::Int},
		{"difficulty_round_checkpoints", "ARCHIPELAGO_DIFFICULTY_ROUND_CHECKPOINTS", DvarSetting::Type::Int},
		{"rolled_bows", "ARCHIPELAGO_ROLLED_BOW_", DvarSetting::Type::StrArray},
		{"rolled_masks", "ARCHIPELAGO_ROLLED_MASK_", DvarSetting::Type::StrArray},
		{"goal_items", "ARCHIPELAGO_GOAL_ITEM_", DvarSetting::Type::StrArray},
		{"goal_items_required", "ARCHIPELAGO_GOAL_ITEMS_REQUIRED", DvarSetting::Type::Int},
		{"attachments_randomized", "ARCHIPELAGO_ATTACHMENT_RANDO_ENABLED", DvarSetting::Type::Bool},
		{"attachments_sight_weight", "ARCHIPELAGO_ATTACHMENT_RANDO_SIGHT_SIZE_WEIGHT", DvarSetting::Type::Int},
		{"camo_randomized", "ARCHIPELAGO_CAMO_RANDOMIZED", DvarSetting::Type::Bool},
		{"camo_mixed", "ARCHIPELAGO_CAMO_MIXED", DvarSetting::Type::Bool},
		{"camo_pap_randomized", "ARCHIPELAGO_CAMO_PAP_RANDOMIZED", DvarSetting::Type::Bool},
		{"camo_pap_mixed", "ARCHIPELAGO_CAMO_PAP_MIXED", DvarSetting::Type::Bool},
		{"camo_joined", "ARCHIPELAGO_CAMO_JOINED", DvarSetting::Type::Bool},
		{"reticle_randomized", "ARCHIPELAGO_RETICLE_RANDOMIZED", DvarSetting::Type::Bool},
		{"reticle_pap_randomized", "ARCHIPELAGO_RETICLE_PAP_RANDOMIZED", DvarSetting::Type::Bool},
		{"reticle_joined", "ARCHIPELAGO_RETICLE_JOINED", DvarSetting::Type::Bool},
		{"deathlink_enabled", "ARCHIPELAGO_DEATHLINK_ENABLED", DvarSetting::Type::Bool},
		{"deathlink_send_mode", "ARCHIPELAGO_DEATHLINK_SEND_MODE", DvarSetting::Type::Int},
		{"deathlink_recv_mode", "ARCHIPELAGO_DEATHLINK_RECV_MODE", DvarSetting::Type::Int},
	};

	std::list<int64_t> checkedLocationsList = { };

	std::string get_save_path(const std::string& seed);
	void save_slot_data(const std::string& seed, const std::string& data);
	std::string load_slot_data(const std::string& seed);

	//Utility Functions
	void APLogPrint(std::string message)
	{
		game::minlog.WriteLine(message.c_str());
		std::string luaThreadCode = "Archi.LogMessage(\"" + message + "\");";
		hks::execute_raw_lua(luaThreadCode, "APLogThread");
	}
	void APDebugLogPrint(std::string message)
	{
		game::minlog.WriteLine(message.c_str());
		std::string luaThreadCode = "Archi.LogDebugMessage(\"" + message + "\");";
		hks::execute_raw_lua(luaThreadCode, "APLogThread");
	}

	void APSetDvar(std::string var, std::string val)
	{
		std::string luaThreadCode = "Engine.SetDvar( \""+var+"\", \"" + val + "\" )";
		hks::execute_raw_lua(luaThreadCode, "APSetDvarThread");
	}

	void APSetDvarInt(std::string var, int val)
	{
		std::string luaThreadCode = "Engine.SetDvar( \"" + var + "\", " + std::to_string(val) + " )";
		hks::execute_raw_lua(luaThreadCode, "APSetDvarThread");
	}

	//Actual C++ functions
	void disconnect_ap()
	{
		if (ap) delete ap;
		ap = nullptr;
	}

	void check_connection_ap(std::string uri = "", std::string slot = "", std::string password = "", std::string uuidFile = ".\\mods\\bo3_archipelago\\zone\\uuid")
	{
		std::string uuid = ap_get_uuid(UUID_FILE);

		if (temp_ap) delete temp_ap;
		temp_ap = nullptr;

		temp_ap = new APClient(uuid, GAME_NAME, uri.empty() ? APClient::DEFAULT_URI : uri);

		temp_ap->set_socket_connected_handler([]() {
			});
		temp_ap->set_socket_disconnected_handler([]() {
			});

		temp_ap->set_socket_error_handler([](const std::string& error) {
				//On Error don't try and reconnect
				std::string escaped_error = error;
				size_t pos = 0;
				while ((pos = escaped_error.find('"', pos)) != std::string::npos) {
					escaped_error.replace(pos, 1, "'");
					pos += 1;
				}

				std::string luaThreadCode = "UpdateConnectionStatus(\"Error " + escaped_error + "\")";
				hks::execute_raw_lua(luaThreadCode, "SetConnectionValidatedThread");
			});

		temp_ap->set_room_info_handler([slot, password]() {
			temp_ap->ConnectSlot(slot, password, items_handling, {"TextOnly"}, VERSION_TUPLE);
			});
		temp_ap->set_slot_connected_handler([](const json& data) {
			//Mandatory values
			if (!data.contains("base_id") || !data.contains("seed") || !data.contains("slot")) {

			}

			std::string luaThreadCode = "UpdateConnectionStatus(\"Validated\")";
			hks::execute_raw_lua(luaThreadCode, "SetConnectionValidatedThread");
		});

		temp_ap->set_slot_refused_handler([](const std::list<std::string>& errors) {
				//On Error don't try and reconnect
				std::string luaThreadCode = "UpdateConnectionStatus(\"Error\")";
				if (std::find(errors.begin(), errors.end(), "InvalidSlot") != errors.end()) {
					luaThreadCode = "UpdateConnectionStatus(\"InvalidSlot\")";
				}
				if (std::find(errors.begin(), errors.end(), "InvalidPassword") != errors.end()) {
					luaThreadCode = "UpdateConnectionStatus(\"InvalidPassword\")";

				}
				hks::execute_raw_lua(luaThreadCode, "SetConnectionValidatedThread");
				});

		for (int i = 0; i < 10; i++)
		{
			if (temp_ap){
			
				temp_ap->poll();
				Sleep(1000);
			}
		}
		if (temp_ap) 
		{	
			delete temp_ap;
		}
		temp_ap = nullptr;
			
	}

	void connect_ap(std::string uri = "", std::string slot = "", std::string password = "", std::string uuidFile = ".\\mods\\bo3_archipelago\\zone\\uuid")
	{
		std::string uuid = ap_get_uuid(UUID_FILE);

		if (ap) delete ap;
		ap = nullptr;
		ap = new APClient(uuid, GAME_NAME, uri.empty() ? APClient::DEFAULT_URI : uri);

		/*
		 TODO: DataPackage Cache
		*/

		ap->set_socket_connected_handler([]() {
			socket_connected = true;
			});
		ap->set_socket_disconnected_handler([]() {
			socket_connected = false;
			std::string luaThreadCode = "Archi.SocketDisconnected();";
			hks::execute_raw_lua(luaThreadCode, "APSocketDisconnectedThread");
			archipelago::awaitingReconnect = true;
			APLogPrint("Socket disconnected");
			});

		ap->set_room_info_handler([slot, password]() {
				ap->ConnectSlot(slot, password, items_handling, {}, VERSION_TUPLE);
			});
		ap->set_slot_connected_handler([](const json& data) {
			//Mandatory values
			if (!data.contains("base_id") || !data.contains("seed") || !data.contains("slot")) {
				//TODO Disconnect/End Game or something :/
			}

			// Check location ids if we don't have them already
			ap->get_missing_locations();
			ap->get_checked_locations();
			
			std::string idStr;
			data.at("base_id").get_to(idStr);
			baseID = std::stoi(idStr);

			data.at("seed").get_to(archipelago::seed);
			data.at("slot").get_to(archipelago::slot);

			archipelago::deathLinkEnabled = data.value("deathlink_enabled", false);

			if (archipelago::deathLinkEnabled) {
				ap->ConnectUpdate(false, items_handling, true, {"DeathLink"});
			}

			// Map slot setting data values to dvars
			for (const auto& mapping : slot_settings)
			{
				if (data.contains(mapping.jsonName))
				{
					try {
						switch (mapping.type) {
							case DvarSetting::Type::StrArray: {
								size_t i = 0;
								for (const auto& value : data.at(mapping.jsonName)) {
									APSetDvar(mapping.dvarName + std::to_string(i), value.get<std::string>());
									++i;
								}
								break;
							}
							case DvarSetting::Type::String: {
								std::string val;
								data.at(mapping.jsonName).get_to(val);
								APSetDvar(mapping.dvarName, val);
								break;
							}
							case DvarSetting::Type::Int: {
								APSetDvarInt(mapping.dvarName, data.at(mapping.jsonName).get<int>());
								break;
							}
							case DvarSetting::Type::Bool: {
								bool val = data.at(mapping.jsonName).get<bool>();
								APSetDvarInt(mapping.dvarName, val ? 1 : 0);
								break;
							}
						}
					} catch (const json::exception& e) {
						APLogPrint("Failed to decode slot data - " + mapping.jsonName);
						continue;
					}
				}
			}

			APSetDvar("ARCHIPELAGO_SEED", archipelago::seed);
			APSetDvar("ARCHIPELAGO_SETTINGS_READY", "TRUE");
			std::string luaThreadCode = "Archi.SlotConnected();";
			hks::execute_raw_lua(luaThreadCode, "SlotConnectedThread");
			});
		ap->set_slot_disconnected_handler([]() {
			APLogPrint("Slot Disconnected");
			});
		ap->set_slot_refused_handler([](const std::list<std::string>& errors) {
			if (std::find(errors.begin(), errors.end(), "InvalidSlot") != errors.end()) {
				//APLogPrint("Slot Invalid");
			}
			else {
				APLogPrint("Connection refused:");
				for (const auto& error : errors) APLogPrint(error.c_str());
			}
			});
		ap->set_location_checked_handler([](const std::list<int64_t>& locations) {
			for (const auto& location : locations) {
				std::string luaThreadCode = "Archi.LocationCheckedEvent("+ std::to_string(location - baseID) +");";
				hks::execute_raw_lua(luaThreadCode, "LocationCheckedThread");
			}
		});
		ap->set_items_received_handler([](const std::list<APClient::NetworkItem>& items) {
			if (!ap->is_data_package_valid()) {
				// NOTE: this should not happen since we ask for data package before connecting
				if (!ap_sync_queued) ap->Sync();
				ap_sync_queued = true;
				return;
			}

			std::list<APClient::NetworkItem> valid_items = std::list<APClient::NetworkItem>(items);

			// TODO: Remove filler items so they don't trigger everytime we load in

			for (const auto& item : valid_items) {
				archipelago::lastItem += 1;

				std::string itemname = ap->get_item_name(item.item);

				std::string sender = ap->get_player_alias(item.player);
				std::string location = ap->get_location_name(item.location);

				std::string luaThreadCode = "Archi.ItemGetEvent(\""+itemname+"\", \"" + sender + "\", \"" + location + "\")";
				hks::execute_raw_lua(luaThreadCode, "ItemGetThread");
			}
		});

		ap->set_data_package_changed_handler([](const json& data) {
			});
		ap->set_print_handler([](const std::string& msg) {
			APLogPrint(msg);
			});
		ap->set_print_json_handler([](const std::list<APClient::TextNode>& msg) {
			APLogPrint(ap->render_json(msg, APClient::RenderFormat::TEXT).c_str());
			});

		ap->set_bounced_handler([](const json& packet) {
			try {
				if(!packet.contains("tags")) {
					return;
				}

				std::list<std::string> tags = packet.at("tags").get<std::list<std::string>>();
				bool deathlink = (std::find(tags.begin(), tags.end(), "DeathLink") != tags.end());

				if (deathlink) {
					auto data = packet.at("data");
					std::string cause = data.value("cause", "");
					std::string source = data.value("source", "");
					double timestamp = data.at("time").get<double>();

					// Max approximate values to ignore
					double a = -1;

					for (double b : archipelago::deathLinkTimestamps) {
						if (fabs(timestamp - b) < std::numeric_limits<double>::epsilon() * fmax(fabs(timestamp), fabs(b))) { // double equality with some leeway because of conversion back and forth from/to JSON
							a = b;
						}
					}

					if (a != -1) {
						archipelago::deathLinkTimestamps.erase(a);
						return;
					}

					archipelago::lastReceivedDeathlinkTime = timestamp;

					// Send deathlink recv
					std::string luaThreadCode = "Archi.DeathlinkRecv(" + std::to_string(timestamp) + ")";
					hks::execute_raw_lua(luaThreadCode, "DeathlinkRecvThread");
				}
			} catch (const json::exception& e) {
				APLogPrint("Weird bounced packet received: " + std::string(e.what()));
				return;
			}
		});

		ap->set_location_info_handler([](const std::list<APClient::NetworkItem> valid_items) {
			for (const auto& item : valid_items) {
				std::string itemname = ap->get_item_name(item.item);
				std::string sender = ap->get_player_alias(item.player);
				std::string location = ap->get_location_name(item.location);

				std::string luaThreadCode = "Archi.LocationScoutCb(\"" + itemname + "\",\"" + sender + "\",\"" + location + "\")";
				hks::execute_raw_lua(luaThreadCode, "LocationScoutCbThread");
			}
		});
	}

	//Lua State Functions 

	int connect(lua::lua_State* s)
	{
		std::string url = lua::lua_tostring(s, 1);
		std::string slot = lua::lua_tostring(s, 2);
		std::string password = lua::lua_tostring(s, 3);
		std::string uuidPath = lua::lua_tostring(s, 4);

		connect_ap(url, slot, password, uuidPath + UUID_FILE);

		return 1;
	}


	int checkConnection(lua::lua_State* s)
	{
		std::string url = lua::lua_tostring(s, 1);
		std::string slot = lua::lua_tostring(s, 2);
		std::string password = lua::lua_tostring(s, 3);
		std::string uuidPath = lua::lua_tostring(s, 4);

		check_connection_ap(url, slot, password, uuidPath + UUID_FILE);

		return 1;
	}

	int getMissingLocations(lua::lua_State* s)
	{
		if (!ap) return 0;

		auto missing = ap->get_missing_locations();
		std::string result = "{";
		for (const auto& loc : missing) {
			result += std::to_string(loc - baseID) + ",";
		}
		if (!missing.empty()) result.pop_back();
		result += "}";

		std::string luaThreadCode = "Archi.ReceiveMissingLocations(" + result + ")";
		hks::execute_raw_lua(luaThreadCode, "GetLocsThread");
		return 0;
	}

	int getCheckedLocations(lua::lua_State* s)
	{
		if (!ap) return 0;

		auto checked = ap->get_checked_locations();
		std::string result = "{";
		for (const auto& loc : checked) {
			result += std::to_string(loc - baseID) + ",";
		}
		if (!checked.empty()) result.pop_back();
		result += "}";

		std::string luaThreadCode = "Archi.ReceiveCheckedLocations(" + result + ")";
		hks::execute_raw_lua(luaThreadCode, "GetLocsThread");
		return 0;
	}

	int storeSaveData(lua::lua_State* s)
	{
		std::string data = lua::lua_tostring(s, 1);
		save_slot_data(archipelago::seed, data);
		return 1;
	}

	int loadSaveData(lua::lua_State* s)
	{
		std::string data = load_slot_data(archipelago::seed);
		lua::lua_pushstring(s, data.c_str());
		return 1;
	}

	int getSeed(lua::lua_State* s)
	{
		lua::lua_pushstring(s, archipelago::seed.c_str());
		return 1;
	}

	int goalReached(lua::lua_State* s)
	{
		ap->StatusUpdate(APClient::ClientStatus::GOAL);
		return 1;
	}

	int disconnect(lua::lua_State* s)
	{
		disconnect_ap();
		return 1;
	}

	int poll(lua::lua_State* s)
	{
		if (ap) ap->poll();

		int size = archipelago::checkedLocationsList.size();
		if (ap && size > 0) {
			if (ap->LocationChecks(archipelago::checkedLocationsList)) {
				archipelago::checkedLocationsList.clear();
			}
		}
		return 1;
	}

	int say(lua::lua_State *s)
	{
		const std::string message = lua::lua_tostring(s, 1);
		ap->Say(message);
		return 1;
	}

	int checkLocation(lua::lua_State* s)
	{
		lua::HksNumber loc = lua::lua_tonumber(s, 1);
		int64_t loc_id = static_cast<int64_t>(loc);
		checkedLocationsList.push_back(loc_id + baseID);
		return 1;
	}

	int socketConnected(lua::lua_State* s)
	{
		lua::lua_pushboolean(s, socket_connected);
		return 1;
	}

	int sendLocationScout(lua::lua_State* s)
	{
		lua::HksNumber loc = lua::lua_tonumber(s, 1);
		int64_t loc_id = static_cast<int64_t>(loc);
		ap->LocationScouts({ archipelago::baseID + loc_id });
		return 1;
	}

	int sendDeathlink(lua::lua_State* s)
	{
		if (archipelago::deathLinkEnabled) {
			auto now = std::chrono::system_clock::now();
			double nowDouble = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 1000;

			// Bit of leniance for networking stuff to prevent unintended deathlinks sending
			if (nowDouble - archipelago::lastReceivedDeathlinkTime < 2.0) {
				return 1;
			}
		
			// Ignore own packet later
			archipelago::deathLinkTimestamps.insert(nowDouble);
		
			auto data = nlohmann::json{
				{"time", nowDouble},
				{"cause", ""},
				{"source", ap->get_player_alias(ap->get_player_number())}
			};
			ap->Bounce(data, {}, {}, {"DeathLink"});
		}

		return 1;
	}

	class component final : public component_interface
	{
	public:
		void lua_start() override
		{
			const lua::luaL_Reg ArchipelagoLibrary[] =
			{
				{"Connect", connect},
				{"CheckConnection",checkConnection},
				{"GetCheckedLocations",getCheckedLocations},
				{"GetMissingLocations",getMissingLocations},
				{"Disconnect",disconnect},
				{"Poll",poll},
				{"CheckLocation",checkLocation},
				{"Say",say},
				{"StoreSaveData",storeSaveData},
				{"LoadSaveData",loadSaveData},
				{"GetSeed",getSeed},
				{"GoalReached",goalReached},
				{"SendDeathlink",sendDeathlink},
				{"SendLocationScout",sendLocationScout},
				{nullptr, nullptr},
			};
			hks::hksI_openlib(game::UI_luaVM, "Archipelago", ArchipelagoLibrary, 0, 1);
		}
	};

	std::string get_save_path(const std::string& seed)
	{
		std::filesystem::path save_dir = ".\\archipelago_saves";
		std::filesystem::create_directories(save_dir);
		return (save_dir / (seed + ".json")).string();
	}

	void save_slot_data(const std::string& seed, const std::string& data)
	{
		try {
			std::string save_path = get_save_path(seed);
			std::ofstream file(save_path);
			if (file.is_open()) {
				file << data;
				file.close();
			}
		}
		catch (const std::exception& e) {
			APLogPrint("Error saving slot data: " + std::string(e.what()));
		}
	}

	std::string load_slot_data(const std::string& seed)
	{
		try {
			std::string save_path = get_save_path(seed);
			if (std::filesystem::exists(save_path)) {
				std::ifstream file(save_path);
				if (file.is_open()) {
					std::stringstream buffer;
					buffer << file.rdbuf();
					file.close();
					APDebugLogPrint("Loaded slot data for seed: " + seed);
					return buffer.str();
				}
			}
		}
		catch (const std::exception& e) {
			APLogPrint("Error loading slot data: " + std::string(e.what()));
		}
		return ""; // Return empty string if load fails
	}
}

REGISTER_COMPONENT(archipelago::component)