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

	int baseID = 0;

	int lastItem = 0;

	std::string slot = "";
	std::string seed = "";

	std::list<int64_t> checkedLocationsList = { };

	static std::unordered_map<std::string, int> loc_name_to_id{
	{"(The Giant) Round 01",1},
	{"(The Giant) Round 02",2},
	{"(The Giant) Round 03",3},
	{"(The Giant) Round 04",4},
	{"(The Giant) Round 05",5},
	{"(The Giant) Round 06",6},
	{"(The Giant) Round 07",7},
	{"(The Giant) Round 08",8},
	{"(The Giant) Round 09",9},
	{"(The Giant) Round 10",10},
	{"(The Giant) Round 11",11},
	{"(The Giant) Round 12",12},
	{"(The Giant) Round 13",13},
	{"(The Giant) Round 14",14},
	{"(The Giant) Round 15",15},
	{"(The Giant) Round 16",16},
	{"(The Giant) Round 17",17},
	{"(The Giant) Round 18",18},
	{"(The Giant) Round 19",19},
	{"(The Giant) Round 20",20},
	{"(The Giant) Round 21",21},
	{"(The Giant) Round 22",22},
	{"(The Giant) Round 23",23},
	{"(The Giant) Round 24",24},
	{"(The Giant) Round 25",25},
	{"(The Giant) Round 26",26},
	{"(The Giant) Round 27",27},
	{"(The Giant) Round 28",28},
	{"(The Giant) Round 29",29},
	{"(The Giant) Round 30",30},
	{"(The Giant) Round 31",31},
	{"(The Giant) Round 32",32},
	{"(The Giant) Round 33",33},
	{"(The Giant) Round 34",34},
	{"(The Giant) Round 35",35},
	{"(The Giant) Round 36",36},
	{"(The Giant) Round 37",37},
	{"(The Giant) Round 38",38},
	{"(The Giant) Round 39",39},
	{"(The Giant) Round 40",40},
	{"(The Giant) Round 41",41},
	{"(The Giant) Round 42",42},
	{"(The Giant) Round 43",43},
	{"(The Giant) Round 44",44},
	{"(The Giant) Round 45",45},
	{"(The Giant) Round 46",46},
	{"(The Giant) Round 47",47},
	{"(The Giant) Round 48",48},
	{"(The Giant) Round 49",49},
	{"(The Giant) Round 50",50},
	{"(The Giant) Round 51",51},
	{"(The Giant) Round 52",52},
	{"(The Giant) Round 53",53},
	{"(The Giant) Round 54",54},
	{"(The Giant) Round 55",55},
	{"(The Giant) Round 56",56},
	{"(The Giant) Round 57",57},
	{"(The Giant) Round 58",58},
	{"(The Giant) Round 59",59},
	{"(The Giant) Round 60",60},
	{"(The Giant) Round 61",61},
	{"(The Giant) Round 62",62},
	{"(The Giant) Round 63",63},
	{"(The Giant) Round 64",64},
	{"(The Giant) Round 65",65},
	{"(The Giant) Round 66",66},
	{"(The Giant) Round 67",67},
	{"(The Giant) Round 68",68},
	{"(The Giant) Round 69",69},
	{"(The Giant) Round 70",70},
	{"(The Giant) Round 71",71},
	{"(The Giant) Round 72",72},
	{"(The Giant) Round 73",73},
	{"(The Giant) Round 74",74},
	{"(The Giant) Round 75",75},
	{"(The Giant) Round 76",76},
	{"(The Giant) Round 77",77},
	{"(The Giant) Round 78",78},
	{"(The Giant) Round 79",79},
	{"(The Giant) Round 80",80},
	{"(The Giant) Round 81",81},
	{"(The Giant) Round 82",82},
	{"(The Giant) Round 83",83},
	{"(The Giant) Round 84",84},
	{"(The Giant) Round 85",85},
	{"(The Giant) Round 86",86},
	{"(The Giant) Round 87",87},
	{"(The Giant) Round 88",88},
	{"(The Giant) Round 89",89},
	{"(The Giant) Round 90",90},
	{"(The Giant) Round 91",91},
	{"(The Giant) Round 92",92},
	{"(The Giant) Round 93",93},
	{"(The Giant) Round 94",94},
	{"(The Giant) Round 95",95},
	{"(The Giant) Round 96",96},
	{"(The Giant) Round 97",97},
	{"(The Giant) Round 98",98},
	{"(The Giant) Round 99",99},
	};

	static std::unordered_map<std::string, std::string> settings{
		{"the_giant_enabled","AP_THE_GIANT_ENABLED"}
	};

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
				std::string luaThreadCode = "UpdateConnectionStatus(\"Error\")";
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

			/* TODO: Pull settings once i add some
			for (auto& [jsonName, dVar] : settings)
			{
				if (data.contains(jsonName))
				{
					std::string val;
					data.at(jsonName).get_to(val);
					APSetDvar(dVar, val);
				}
			}*/

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
			
			/* TODO: Get this to actually work
			* 
			* //If we are awaiting the big reconnect dump of items, skip to after the last item we know about
			if (archipelago::awaitingReconnect)
			{
				if (items.size() > archipelago::lastItem)
				{
					auto lastIter = valid_items.begin();
					std::advance(lastIter, archipelago::lastItem);
					valid_items.erase(valid_items.begin(), lastIter);
				}

				archipelago::awaitingReconnect = false;
			}*/
			

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

	int64_t getIdFromString(std::string name)
	{
		int result;
		try
		{
			result = loc_name_to_id.at(name);
		}
		catch (std::out_of_range e)
		{
			//Not a valid item recieved
			return archipelago::loc_error;
		}
		return (int64_t)(result+baseID);
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

	int checkLocation(lua::lua_State* s)
	{
		std::string loc = lua::lua_tostring(s, 1);


		int64_t loc_id = getIdFromString(loc);
		if (loc_id != archipelago::loc_error)
		{
			checkedLocationsList.push_back(loc_id);
		}
		else
		{
			console::print_info("ERROR getting id");
		}
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
				{nullptr, nullptr},
			};
			hks::hksI_openlib(game::UI_luaVM, "Archipelago", ArchipelagoLibrary, 0, 1);
		}
	};
}

REGISTER_COMPONENT(archipelago::component)