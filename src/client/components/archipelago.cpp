#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "havok/hks_api.hpp"
#include "havok/lua_api.hpp"

#define UUID_FILE "uuid"

#define VERSION_TUPLE {0,6,5}

#include <apclient.hpp>
#include <apuuid.hpp>
#include "console.hpp"

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

	std::string slot = "";
	std::string seed = "";

	struct DvarSetting {
		std::string jsonName;
		std::string dvarName;
		enum class Type { String, Int, Bool } type;
	};

	struct DvarSetting slot_settings[] = {
		{"map_specific_wallbuys", "ARCHIPELAGO_MAP_SPECIFIC_WALLBUYS", DvarSetting::Type::Bool},
		{"map_specific_machines", "ARCHIPELAGO_MAP_SPECIFIC_MACHINES", DvarSetting::Type::Bool},
		{"special_rounds_enabled", "ARCHIPELAGO_SPECIAL_ROUNDS_ENABLED", DvarSetting::Type::Bool},
		{"perk_limit_default_modifier", "ARCHIPELAGO_PERK_LIMIT_DEFAULT_MODIFIER", DvarSetting::Type::Int},
		{"randomized_shield_parts", "ARCHIPELAGO_RANDOMIZED_SHIELD_PARTS", DvarSetting::Type::Bool},
	};

	std::list<int64_t> checkedLocationsList = { };

	static std::unordered_map<std::string, std::string> settings{
		{"the_giant_enabled","AP_THE_GIANT_ENABLED"}
	};

	std::string get_save_path(const std::string& seed);
	void save_slot_data(const std::string& seed, const std::string& data);
	std::string load_slot_data(const std::string& seed);

	//Utility Functions
	void APLogPrint(std::string message)
	{
		std::string luaThreadCode = "Archi.LogMessage(\"" + message + "\");";
		hks::execute_raw_lua(luaThreadCode, "APLogThread");
	}
	void APDebugLogPrint(std::string message)
	{
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

	void check_connection_ap(std::string uri = "", std::string slot = "", std::string uuidFile = ".\\mods\\bo3_archipelago\\zone\\uuid", std::string password = "")
	{

		std::string uuid = ap_get_uuid(UUID_FILE);

		if (temp_ap)
		{	
			delete temp_ap;
		}
		temp_ap = nullptr;

		std::string luaThreadCode = "UpdateConnectionStatus(\"Connecting...\")";
		hks::execute_raw_lua(luaThreadCode, "SetConnectionValidatedThread");

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

			std::list<std::string> tags;
			tags.push_back("TextOnly");
			temp_ap->ConnectSlot(slot, password, items_handling, tags, VERSION_TUPLE);
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
				else {
					
				}
				hks::execute_raw_lua(luaThreadCode, "SetConnectionValidatedThread");
				});

		for (int i = 0; i < 5; i++)
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

	void connect_ap(std::string uri = "",std::string slot="",std::string uuidFile = ".\\mods\\bo3_archipelago\\zone\\uuid", std::string password = "")
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
				std::list<std::string> tags;
				ap->ConnectSlot(slot, password, items_handling, tags, VERSION_TUPLE);
			});
		ap->set_slot_connected_handler([](const json& data) {

			//Mandatory values
			if (!data.contains("base_id") || !data.contains("seed") || !data.contains("slot")) {
				//TODO Disconnect/End Game or something :/
			}

			std::string idStr;
			data.at("base_id").get_to(idStr);


			baseID = std::stoi(idStr);

			data.at("seed").get_to(archipelago::seed);
			data.at("slot").get_to(archipelago::slot);

			// Map slot setting data values to dvars
			for (const auto& mapping : slot_settings)
			{
				if (data.contains(mapping.jsonName))
				{
					switch (mapping.type)
					{
						case DvarSetting::Type::String:
						{
							std::string val;
							data.at(mapping.jsonName).get_to(val);
							APSetDvar(mapping.dvarName, val);
							break;
						}
						case DvarSetting::Type::Int:
						{
							APSetDvarInt(mapping.dvarName, (int) data[mapping.jsonName]);
							break;
						}
						case DvarSetting::Type::Bool:
						{
							bool val = data[mapping.jsonName] == true;
							if (val) {
								APSetDvarInt(mapping.dvarName, 1);
							} else {
								APSetDvarInt(mapping.dvarName, 0);
							}
							break;
						}
					}
				}
			}

			APSetDvar("ARCHIPELAGO_SETTINGS_READY", "TRUE");
			APSetDvar("ARCHIPELAGO_SEED", archipelago::seed);

			for (auto& [jsonName, dVar] : settings)
			{
				if (data.contains(jsonName))
				{
					std::string val;
					data.at(jsonName).get_to(val);
					APSetDvar(dVar, val);
				}
			}

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

				std::string luaThreadCode = "Archi.ItemGetEvent(\""+itemname+"\");";
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

		ap->set_bounced_handler([](const json& cmd) {
			//TODO Implement Deathlink
			});

	}

	//Lua State Functions 

	int connect(lua::lua_State* s)
	{
		std::string url = lua::lua_tostring(s, 1);
		std::string slot = lua::lua_tostring(s, 2);
		std::string uuidPath = lua::lua_tostring(s, 3);

		std::string password = "";

		connect_ap(url, slot,uuidPath + UUID_FILE,password);

		return 1;
	}


	int checkConnection(lua::lua_State* s)
	{
		std::string url = lua::lua_tostring(s, 1);
		std::string slot = lua::lua_tostring(s, 2);
		std::string uuidPath = lua::lua_tostring(s, 3);

		//TODO: Support Password Argument
		std::string password = "";

		check_connection_ap(url, slot, uuidPath + UUID_FILE, password);

		return 1;
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

	class component final : public component_interface
	{
	public:
		void lua_start() override
		{
			const lua::luaL_Reg ArchipelagoLibrary[] =
			{
				{"Connect", connect},
				{"CheckConnection",checkConnection},
				{"Disconnect",disconnect},
				{"Poll",poll},
				{"CheckLocation",checkLocation},
				{"Say",say},
				{"StoreSaveData",storeSaveData},
				{"LoadSaveData",loadSaveData},
				{"GetSeed",getSeed},
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
				APDebugLogPrint("Saved slot data for seed: " + seed);
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