#include "global.hpp"
#include "game_config.hpp"
#include "filesystem.hpp"
#include "language.hpp"
#include "loadscreen.hpp"
#include "editor.hpp"
#include <sys/stat.h>
#include "display.hpp"
#include "wml_exception.hpp"
#include "gettext.hpp"
#include "gui/dialogs/message.hpp"
#include "serialization/parser.hpp"
#include "formula_string_utils.hpp"
#include "sdl_filesystem.h"

#include "animation.hpp"
#include "builder.hpp"

#include <iomanip>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#ifndef _WIN32
typedef struct {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;

void CoCreateGuid(GUID* pguid)
{
	memset(pguid, 0, sizeof(GUID));
}
#else
#include <combaseapi.h>
#endif

const std::string studio_guid = "3641E31E-36BF-4E03-8879-DE33ADC07D68";
const std::string apps_sln_guid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942";

void tapp_capabilities::generate(std::stringstream& ss, const std::string& prefix) const
{
	VALIDATE(bundle_id.valid(), null_str);

	ss << prefix << "[generate]\n";
	ss << prefix << "\ttype = app\n";
	if (app != bundle_id.node(2)) {
		ss << prefix << "\tapp = " << app << "\n";
	}
	ss << prefix << "\tbundle_id = \"" << bundle_id.id() << "\"\n";
	if (ble) {
		ss << prefix << "\tble = yes\n";
	}
	if (healthkit) {
		ss << prefix << "\thealthkit = yes\n";
	}

	if (tdomains.size() > 1) {
		std::stringstream extra_textdomain;
		const std::string app_lib = app + "-lib";
		for (std::set<std::string>::const_iterator it = tdomains.begin(); it != tdomains.end(); ++ it) {
			const std::string& domain = *it;
			if (domain != app_lib) {
				if (!extra_textdomain.str().empty()) {
					extra_textdomain << ", ";
				}
				extra_textdomain << domain;
			}
		}
		ss << prefix << "\textra_textdomain = " << extra_textdomain.str() << "\n";
	}

	ss << prefix << "[/generate]\n";
}

bool is_apps_kit()
{
	// 1. resource directory
	const std::string& res_folder = game_config::path;
	const std::string APPS_RES = "apps-res";
	size_t pos = res_folder.find(APPS_RES);
	if (pos == std::string::npos || pos + APPS_RES.size() != res_folder.size()) {
		return false;
	}

	// 2. code directory
	const std::string& src_folder = game_config::apps_src_path;
	const std::string APPS_SRC = "apps-src";
	pos = src_folder.find(APPS_SRC);
	if (pos == std::string::npos || pos + APPS_SRC.size() != src_folder.size()) {
		return false;
	}

	// 3. apps.sln
	if (!apps_sln::apps_in().size()) {
		return false;
	}

	return true;
}

// private app will not in <apps-res>/absolute/apps.cfg
bool is_private_app(const std::string& app)
{
	static std::set<std::string> private_apps;
	if (private_apps.empty()) {
		private_apps.insert("studio");
	}

	return private_apps.find(app) != private_apps.end();
}

// reserve app can not be removed.
bool is_reserve_app(const std::string& app)
{
	static std::set<std::string> reserve_apps;
	if (reserve_apps.empty()) {
		reserve_apps.insert("blesmart");
		reserve_apps.insert("editor");
		reserve_apps.insert("kingdom");
		reserve_apps.insert("sleep");
		reserve_apps.insert("sesame");
		reserve_apps.insert("studio");
	}

	return reserve_apps.find(app) != reserve_apps.end();
}

namespace apps_sln {
std::set<std::string> apps_in()
{
	std::set<std::string> apps;
	std::stringstream ss;

	VALIDATE(!game_config::apps_src_path.empty(), null_str);
	const std::string sln = game_config::apps_src_path + "/apps/projectfiles/vc/apps.sln";

	tfile file(sln,  GENERIC_READ, OPEN_EXISTING);
	int fsize = file.read_2_data();
	if (!fsize) {
		return apps;
	}
	// i think length of appended data isn't more than 512 bytes.
	file.resize_data(fsize + 512, fsize);
	file.data[fsize] = '\0';

	const char* start2 = file.data;
	if (utils::bom_magic_started((const uint8_t*)file.data, fsize)) {
		start2 += BOM_LENGTH;
	}

	// studio
	ss.str("");
	ss << "Project(\"{" << apps_sln_guid << "}\") = \"";
	std::string prefix = ss.str();
	const char* ptr = NULL;
	{
		ptr = strstr(start2, prefix.c_str());
		while (ptr) {
			ptr += prefix.size();
			start2 = strchr(ptr, '\"');
			VALIDATE(start2, null_str);

			apps.insert(std::string(ptr, start2 - ptr));
			ptr = strstr(start2, prefix.c_str());
		}
	}

	return apps;
}

bool add_project(const std::string& app, const std::string& guid_str)
{
	std::set<std::string> apps;
	std::stringstream ss;

	VALIDATE(!game_config::apps_src_path.empty(), null_str);
	const std::string sln = game_config::apps_src_path + "/apps/projectfiles/vc/apps.sln";

	tfile file(sln, GENERIC_WRITE, OPEN_EXISTING);
	int fsize = file.read_2_data();
	if (!fsize) {
		return false;
	}
	// i think length of appended data isn't more than 512 bytes.
	file.resize_data(fsize + 512, fsize);
	file.data[fsize] = '\0';

	const char* start2 = file.data;
	if (utils::bom_magic_started((const uint8_t*)file.data, fsize)) {
		start2 += BOM_LENGTH;
	}

	// insert project at end
	std::string prefix = "EndProject";
	std::string postfix = "Global";
	const char* ptr = strstr(start2, prefix.c_str());
	while (ptr) {
		const char* ptr2 = utils::skip_blank_characters(ptr + prefix.size());
		if (!SDL_strncmp(ptr2, postfix.c_str(), postfix.size())) {
			std::stringstream ss;
			ss << "Project(\"{" << apps_sln_guid << "}\") = \"" << app << "\", \"" << app << ".vcxproj\", \"{" << guid_str << "}\"";
			ss << "\r\n";
			ss << "EndProject";
			ss << "\r\n";
			fsize = file.replace_span(ptr2 - file.data, 0, ss.str().c_str(), ss.str().size(), fsize);
			break;
		}
		ptr = strstr(ptr2, prefix.c_str());
	}

	// insert configuration
	prefix = "= postSolution";
	ptr = strstr(start2, prefix.c_str());
	if (ptr) {
		postfix = "EndGlobalSection";
		ptr = strstr(ptr, postfix.c_str());
		if (ptr) {
			ptr -= 1; // \t
		}
	}
	if (ptr) {
		std::stringstream ss;
		ss << "\t\t{" << guid_str << "}.Debug|Win32.ActiveCfg = Debug|Win32" << "\r\n";
		ss << "\t\t{" << guid_str << "}.Debug|Win32.Build.0 = Debug|Win32" << "\r\n";
		ss << "\t\t{" << guid_str << "}.Release|Win32.ActiveCfg = Release|Win32" << "\r\n";
		ss << "\t\t{" << guid_str << "}.Release|Win32.Build.0 = Release|Win32" << "\r\n";
		fsize = file.replace_span(ptr - file.data, 0, ss.str().c_str(), ss.str().size(), fsize);
	}

	// write data to new file
	posix_fseek(file.fp, 0);
	posix_fwrite(file.fp, file.data, fsize);

	return true;
}

bool remove_project(const std::string& app)
{
	std::set<std::string> apps;
	std::stringstream ss;

	VALIDATE(!game_config::apps_src_path.empty(), null_str);
	const std::string sln = game_config::apps_src_path + "/apps/projectfiles/vc/apps.sln";

	// const std::string sln3 = game_config::apps_src_path + "/apps/projectfiles/vc/apps.sln3";
	// SDL_CopyFiles(sln.c_str(), sln3.c_str());

	tfile file(sln,  GENERIC_WRITE, OPEN_EXISTING);
	int fsize = file.read_2_data();
	if (!fsize) {
		return false;
	}
	// i think length of appended data isn't more than 512 bytes.
	file.resize_data(fsize + 512, fsize);
	file.data[fsize] = '\0';

	const char* start2 = file.data;
	if (utils::bom_magic_started((const uint8_t*)file.data, fsize)) {
		start2 += BOM_LENGTH;
	}

	// studio
	ss.str("");
	ss << "Project(\"{" << apps_sln_guid << "}\") = \"" << app << "\"";
	std::string prefix = ss.str();
	const char* ptr = strstr(start2, prefix.c_str());
	if (!ptr) {
		return false;
	}
	ptr = strchr(ptr + prefix.size(), '{');
	VALIDATE(ptr, null_str);

	start2 = ptr + 1;
	ptr = strchr(start2, '}');

	const std::string guid_str(start2, ptr - start2);
	const int guid_size2 = 36;
	VALIDATE(guid_str.size() == guid_size2, null_str);
	start2 += guid_str.size();
	// move start2 to first char of this line
	while (start2[0] != '\r' && start2[0] != '\n') { start2 --; }
	start2 ++;

	//
	// delete like below lines:
	// Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "hello", "hello.vcxproj", "{43FDDD3E-D26A-4F52-B207-8DC03CB25396}"
	// EndProject
	//

	prefix = "EndProject";
	ptr = strstr(ptr, prefix.c_str());
	if (!ptr) {
		return false;
	}
	ptr += prefix.size();
	// move ptr to first char of next line.
	while (ptr[0] != '\r' && ptr[0] != '\n') { ptr ++; }
	while (ptr[0] == '\r' || ptr[0] == '\n') { ptr ++; }
	fsize = file.replace_span(start2 - file.data, ptr - start2, NULL, 0, fsize);

	// remove items in GlobalSection
	ss.str("");
	ss << "{" << guid_str << "}";
	prefix = ss.str();

	ptr = strstr(start2, prefix.c_str());
	if (!ptr) {
		return false;
	}
	start2 = ptr - 1;
	while (start2[0] == '\t' || start2[0] == ' ') { start2 --; }
	// start indicate postion that from delete.
	const char* start = start2 + 1;

	// move start2 to last line that has guid_str.
	while (ptr) {
		start2 = ptr;
		ptr = strstr(start2 + prefix.size(), prefix.c_str());
	}

	// move ptr to first char of next line.
	ptr = start2 + prefix.size();
	while (ptr[0] != '\r' && ptr[0] != '\n') { ptr ++; }
	while (ptr[0] == '\r' || ptr[0] == '\n') { ptr ++; }

	fsize = file.replace_span(start - file.data, ptr - start, NULL, 0, fsize);

	// write data to new file
	posix_fseek(file.fp, 0);
	posix_fwrite(file.fp, file.data, fsize);

	file.truncate(fsize);

	return true;
}

}

bool check_res_folder(const std::string& folder)
{
	std::stringstream ss;
	
	// <wok>\data\_main.cfg
	ss << folder << "\\data\\_main.cfg";
	if (!file_exists(ss.str())) {
		return false;
	}

	return true;
}

bool check_apps_src_folder(const std::string& folder)
{
	std::stringstream ss;
	
	ss << folder << "/apps/projectfiles/vc/apps.sln";
	if (!file_exists(ss.str())) {
		return false;
	}

	return true;
}

namespace editor_config
{
	config data_cfg;
	int type = BIN_WML;
	std::string campaign_id;

void reload_data_bin()
{
	const config& game_cfg = data_cfg.child("game_config");
	game_config::load_config(game_cfg? &game_cfg : NULL);
}

}

editor::wml2bin_desc::wml2bin_desc()
	: bin_name()
	, wml_nfiles(0)
	, wml_sum_size(0)
	, wml_modified(0)
	, bin_nfiles(0)
	, bin_sum_size(0)
	, bin_modified(0)
	, require_build(false)
{}

void editor::wml2bin_desc::refresh_checksum(const std::string& working_dir)
{
	VALIDATE(valid(), null_str);
	std::string bin_file = working_dir + "/xwml/";
	if (!app.empty()) {
		bin_file += game_config::generate_app_dir(app) + "/";
	}
	bin_file += bin_name;
	wml_checksum_from_file(bin_file, &bin_nfiles, &bin_sum_size, (uint32_t*)&bin_modified);
}

#define BASENAME_DATA		"data.bin"
#define BASENAME_GUI		"gui.bin"
#define BASENAME_LANGUAGE	"language.bin"

// file processor function only support prefixed with game_config::path.
int editor::tres_path_lock::deep = 0;
editor::tres_path_lock::tres_path_lock(editor& o)
	: original_(game_config::path)
{
	VALIDATE(!deep, null_str);
	deep ++;
	game_config::path = o.working_dir_;
}

editor::tres_path_lock::~tres_path_lock()
{
	game_config::path = original_;
	deep --;
}

editor::editor(const std::string& working_dir) 
	: cache_(game_config::config_cache::instance())
	, working_dir_(working_dir)
	, wml2bin_descs_()
{
}

void editor::set_working_dir(const std::string& dir)
{
	if (working_dir_ == dir) {
		return;
	}
	working_dir_ = dir;
}

bool editor::make_system_bins_exist()
{
	std::string file;
	std::vector<editor::BIN_TYPE> system_bins;
	for (editor::BIN_TYPE type = editor::BIN_MIN; type <= editor::BIN_SYSTEM_MAX; type = (editor::BIN_TYPE)(type + 1)) {
		if (type == MAIN_DATA) {
			file = working_dir_ + "/xwml/" + BASENAME_DATA;
		} else if (type == GUI) {
			file = working_dir_ + "/xwml/" + BASENAME_GUI;
		} else if (type == LANGUAGE) {
			file = working_dir_ + "/xwml/" + BASENAME_LANGUAGE;
		} else {
			VALIDATE(false, null_str);
		}
		if (!file_exists(file)) {
			system_bins.push_back(type);
		}
	}
	if (system_bins.empty()) {
		return true;
	}
	get_wml2bin_desc_from_wml(system_bins);
	const std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = wml2bin_descs();

	int count = (int)descs.size();
	for (int at = 0; at < count; at ++) {
		const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>& desc = descs[at];

		bool ret = false;
		try {
			ret = cfgs_2_cfg(desc.first, desc.second.bin_name, desc.second.app, true, desc.second.wml_nfiles, desc.second.wml_sum_size, (uint32_t)desc.second.wml_modified);
		} catch (twml_exception& /*e*/) {
			return false;
		}
		if (!ret) {
			return false;
		}
	}

	return true;
}

// check location:
//   1. heros_army of artifcal
//   2. service_heros of artifcal
//   3. wander_heros of artifcal
//   4. heros_army of unit
std::string editor::check_scenario_cfg(const config& scenario_cfg)
{
	std::set<std::string> holded_str;
	std::set<int> holded_number;
	std::set<std::string> officialed_str;
	std::map<std::string, std::set<std::string> > officialed_map;
	std::map<std::string, std::string> mayor_map;
	int number;
	std::vector<std::string> str_vec;
	std::vector<std::string>::const_iterator tmp;
	std::stringstream str;

	BOOST_FOREACH (const config& side, scenario_cfg.child_range("side")) {
		const std::string leader = side["leader"];
		BOOST_FOREACH (const config& art, side.child_range("artifical")) {
			officialed_str.clear();
			const std::string cityno = art["cityno"].str();
			mayor_map[cityno] = art["mayor"].str();

			str_vec = utils::split(art["heros_army"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			str_vec = utils::split(art["service_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
				officialed_str.insert(*tmp);
			}
			str_vec = utils::split(art["wander_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			officialed_map[cityno] = officialed_str;
		}
		BOOST_FOREACH (const config& u, side.child_range("unit")) {
			const std::string cityno = u["cityno"].str();
			std::map<std::string, std::set<std::string> >::iterator find_it = officialed_map.find(cityno);
			if (cityno != "0" && find_it == officialed_map.end()) {
				str << "." << scenario_cfg["id"].str() << ", heros_army=" << u["heros_army"].str() << " uses undefined cityno: " << cityno << "";
				return str.str();
			}
			str_vec = utils::split(u["heros_army"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
				if (find_it != officialed_map.end()) {
					find_it->second.insert(*tmp);
				}
			}
		}
		for (std::map<std::string, std::set<std::string> >::const_iterator it = officialed_map.begin(); it != officialed_map.end(); ++ it) {
			std::map<std::string, std::string>::const_iterator mayor_it = mayor_map.find(it->first);
			if (mayor_it->second.empty()) {
				continue;
			}
			if (mayor_it->second == leader) {
				str << "." << scenario_cfg["id"].str() << ", in cityno=" << it->first << " mayor(" << mayor_it->second << ") cannot be leader!";
				return str.str();
			}
			if (it->second.find(mayor_it->second) == it->second.end()) {
				str << "." << scenario_cfg["id"].str() << ", in ciytno=" << it->first << " mayor(" << mayor_it->second << ") must be in offical hero!";
				return str.str();
			}
		}
	}
	return "";
}

// check location:
//   1. heros_army of artifcal
//   2. service_heros of artifcal
//   3. wander_heros of artifcal
std::string editor::check_mplayer_bin(const config& mplayer_cfg)
{
	std::set<std::string> holded_str;
	std::set<int> holded_number;
	int number;
	std::vector<std::string> str_vec;
	std::vector<std::string>::const_iterator tmp;
	std::stringstream str;

	BOOST_FOREACH (const config& faction, mplayer_cfg.child_range("faction")) {
		BOOST_FOREACH (const config& art, faction.child_range("artifical")) {
			str_vec = utils::split(art["heros_army"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			str_vec = utils::split(art["service_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			str_vec = utils::split(art["wander_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
		}
	}
	return "";
}

// check location:
std::string editor::check_data_bin(const config& data_cfg)
{
	std::stringstream str;

	BOOST_FOREACH (const config& campaign, data_cfg.child_range("campaign")) {
		if (!campaign.has_attribute("id")) {
			str << "Compaign hasn't id!";
			return str.str();
		}
	}
	return "";
}

void editor::generate_app_bin_config()
{
	config bin_cfg;
	bool ret = true;
	std::stringstream ss;
	SDL_DIR* dir = SDL_OpenDir(working_dir_.c_str());
	if (!dir) {
		return;
	}
	SDL_dirent2* dirent;
	
	campaigns_config_.clear();
	cache_.clear_defines();
	while ((dirent = SDL_ReadDir(dir))) {
		if (SDL_DIRENT_DIR(dirent->mode)) {
			std::string app = game_config::extract_app_from_app_dir(dirent->name);
			if (app.empty()) {
				continue;
			}
			const std::string bin_file = std::string(dir->directory) + "/" + dirent->name + "/bin.cfg";
			if (!file_exists(bin_file)) {
				continue;
			}
			cache_.get_config(bin_file, bin_cfg);
			VALIDATE(!bin_cfg[BINKEY_ID_CHILD].str().empty(), bin_file + " hasn't no 'id_child' key!");
			VALIDATE(!bin_cfg[BINKEY_SCENARIO_CHILD].str().empty(), bin_file + " hasn't no 'scenario_child' key!");
			VALIDATE(!bin_cfg[BINKEY_PATH].str().empty(), bin_file + " hasn't no 'path' key!");
			std::string path = working_dir_ + "/" + bin_cfg[BINKEY_PATH];

			config& sub = campaigns_config_.add_child("bin");
			cache_.get_config(path, sub);
			sub["app"] = app;
			sub[BINKEY_ID_CHILD] = bin_cfg[BINKEY_ID_CHILD].str();
			sub[BINKEY_SCENARIO_CHILD] = bin_cfg[BINKEY_SCENARIO_CHILD].str();
			sub[BINKEY_PATH] = bin_cfg[BINKEY_PATH].str();
			sub[BINKEY_MACROS] = bin_cfg[BINKEY_MACROS].str();
			cache_.clear_defines();
		}
	}
	SDL_CloseDir(dir);
}

bool editor::cfgs_2_cfg(const editor::BIN_TYPE type, const std::string& name, const std::string& app, bool write_file, uint32_t nfiles, uint32_t sum_size, uint32_t modified, const std::map<std::string, std::string>& app_domains)
{
	config tmpcfg;

	tres_path_lock lock(*this);
	game_config::config_cache_transaction main_transaction;

	try {
		cache_.clear_defines();

		if (type == editor::TB_DAT) {
			VALIDATE(write_file, "write_file must be true when generate TB_DAT!");

			std::string str = name;
			const size_t pos_ext = str.rfind(".");
			str = str.substr(0, pos_ext);
			str = str.substr(terrain_builder::tb_dat_prefix.size());

			const config& tb_cfg = tbs_config_.find_child("tb", "id", str);
			cache_.add_define(tb_cfg["define"].str());
			cache_.get_config(working_dir_ + "/data/tb.cfg", tmpcfg);

			if (write_file) {
				const config& tb_parsed_cfg = tmpcfg.find_child("tb", "id", str);
				binary_paths_manager paths_manager(tb_parsed_cfg);
				terrain_builder(tb_parsed_cfg, nfiles, sum_size, modified);
			}

		} else if (type == editor::SCENARIO_DATA) {
			std::string name_str = name;
			const size_t pos_ext = name_str.rfind(".");
			name_str = name_str.substr(0, pos_ext);

			const config& app_cfg = campaigns_config_.find_child("bin", "app", app);
			const config& campaign_cfg = app_cfg.find_child(app_cfg[BINKEY_ID_CHILD], "id", name_str);

			if (!campaign_cfg["define"].empty()) {
				cache_.add_define(campaign_cfg["define"].str());
			}
			if (!app_cfg[BINKEY_MACROS].empty()) {
				cache_.get_config(working_dir_ + "/" + app_cfg[BINKEY_MACROS], tmpcfg);
			}
			cache_.get_config(working_dir_ + "/" + app_cfg[BINKEY_PATH] + "/" + name_str, tmpcfg);

			const config& refcfg = tmpcfg.child(app_cfg[BINKEY_SCENARIO_CHILD]);
			// check scenario config valid
			BOOST_FOREACH (const config& scenario, refcfg.child_range("scenario")) {
				std::string err_str = check_scenario_cfg(scenario);
				if (!err_str.empty()) {
					throw game::error(std::string("<") + name + std::string(">") + err_str);
				}
			}

			if (write_file) {
				const std::string xwml_app_path = working_dir_ + "/xwml/" + game_config::generate_app_dir(app);
				SDL_MakeDirectory(xwml_app_path.c_str());
				wml_config_to_file(xwml_app_path + "/" + name, refcfg, nfiles, sum_size, modified, app_domains);
			}

		} else if (type == editor::GUI) {
			// no pre-defined
			VALIDATE(write_file, "write_file must be true when generate GUI!");

			cache_.get_config(working_dir_ + "/data/gui", tmpcfg);
			if (write_file) {
				wml_config_to_file(working_dir_ + "/xwml/" + BASENAME_GUI, tmpcfg, nfiles, sum_size, modified, app_domains);
			}

		} else if (type == editor::LANGUAGE)  {
			// no pre-defined
			VALIDATE(write_file, "write_file must be true when generate LANGUAGE!");

			cache_.get_config(working_dir_ + "/data/languages", tmpcfg);
			if (write_file) {
				wml_config_to_file(working_dir_ + "/xwml/" + BASENAME_LANGUAGE, tmpcfg, nfiles, sum_size, modified, app_domains);
			}
		} else if (type == editor::EXTENDABLE)  {
			// no pre-defined
			VALIDATE(!write_file, "write_file must be true when generate EXTENDABLE!");

			// app's bin
			generate_app_bin_config();

			// terrain builder rule
			const std::string tb_cfg = working_dir_ + "/data/tb.cfg";
			if (file_exists(tb_cfg)) {
				cache_.get_config(tb_cfg, tbs_config_);
			}
		} else {
			// type == editor::MAIN_DATA
			cache_.add_define("CORE");
			cache_.get_config(working_dir_ + "/data", tmpcfg);

			// check scenario config valid
			std::string err_str = check_data_bin(tmpcfg);
			if (!err_str.empty()) {
				throw game::error(std::string("<") + BASENAME_DATA + std::string(">") + err_str);
			}

			if (write_file) {
				wml_config_to_file(working_dir_ + "/xwml/" + BASENAME_DATA, tmpcfg, nfiles, sum_size, modified, app_domains);
			}
			editor_config::data_cfg = tmpcfg;

			editor_config::reload_data_bin();
		} 
	}
	catch (game::error& e) {
		display* disp = display::get_singleton();
		gui2::show_error_message(disp->video(), _("Error loading game configuration files: '") + e.message + _("' (The game will now exit)"));
		return false;
	}
	return true;
}

void editor::reload_extendable_cfg()
{
	cfgs_2_cfg(EXTENDABLE, null_str, null_str, false);
	// cfgs_2_cfg will translate relative msgid without load textdomain.
	// result of cfgs_2_cfg used to known what campaign, not detail information.
	// To detail information, need load textdomain, so call t_string::reset_translations(), 
	// let next translate correctly.
	t_string::reset_translations();
}

std::vector<std::string> generate_tb_short_paths(const std::string& id, const config& cfg)
{
	std::stringstream ss;
	std::vector<std::string> short_paths;

	ss.str("");
	ss << "data/core/terrain-graphics-" << id;
	short_paths.push_back(ss.str());

	binary_paths_manager paths_manager(cfg);
	const std::vector<std::string>& paths = paths_manager.paths();
	if (paths.empty()) {
		ss.str("");
		ss << "data/core/images/terrain-" << id;
		short_paths.push_back(ss.str());
	} else {
		for (std::vector<std::string>::const_iterator it = paths.begin(); it != paths.end(); ++ it) {
			ss.str("");
			ss << *it << "images/terrain-" << id;
			short_paths.push_back(ss.str());
		}
	}

	return short_paths;
}

// @path: c:\kingdom-res\data
void editor::get_wml2bin_desc_from_wml(const std::vector<editor::BIN_TYPE>& system_bin_types)
{
	tres_path_lock lock(*this);

	editor::wml2bin_desc desc;
	file_tree_checksum dir_checksum;
	std::vector<std::string> short_paths;
	std::string bin_to_path = working_dir_ + "/xwml";

	std::vector<editor::BIN_TYPE> bin_types = system_bin_types;

	// tb-[tile].dat
	std::vector<config> tbs;
	size_t tb_index = 0;
	BOOST_FOREACH (const config& cfg, tbs_config_.child_range("tb")) {
		tbs.push_back(cfg);
		bin_types.push_back(editor::TB_DAT);
	}

	// search <data>/campaigns, and form [campaign].bin
	std::vector<tapp_bin> app_bins;
	size_t app_bin_index = 0;

	BOOST_FOREACH (const config& bcfg, campaigns_config_.child_range("bin")) {
		const std::string& key = bcfg[BINKEY_ID_CHILD].str();
		BOOST_FOREACH (const config& cfg, bcfg.child_range(key)) {
			app_bins.push_back(tapp_bin(cfg["id"].str(), bcfg["app"].str(), bcfg[BINKEY_PATH].str(), bcfg[BINKEY_MACROS].str()));
			bin_types.push_back(editor::SCENARIO_DATA);
		}
	}

	wml2bin_descs_.clear();

	for (std::vector<editor::BIN_TYPE>::const_iterator itor = bin_types.begin(); itor != bin_types.end(); ++ itor) {
		editor::BIN_TYPE type = *itor;

		short_paths.clear();
		bool calculated_wml_checksum = false;

		int filter = SKIP_MEDIA_DIR;
		if (type == editor::TB_DAT) {
			const std::string& id = tbs[tb_index]["id"].str();
			short_paths = generate_tb_short_paths(id, tbs[tb_index]);
			filter = 0;

			data_tree_checksum(short_paths, dir_checksum, filter);
			desc.wml_nfiles = dir_checksum.nfiles;
			desc.wml_sum_size = dir_checksum.sum_size;
			desc.wml_modified = dir_checksum.modified;

			struct stat st;
			const std::string terrain_graphics_cfg = working_dir_ + "/data/core/terrain-graphics-" + id + ".cfg";
			if (::stat(terrain_graphics_cfg.c_str(), &st) != -1) {
				if (st.st_mtime > desc.wml_modified) {
					desc.wml_modified = st.st_mtime;
				}
				desc.wml_sum_size += st.st_size;
				desc.wml_nfiles ++;
			}
			calculated_wml_checksum = true;

			desc.bin_name = terrain_builder::tb_dat_prefix + id + ".dat";
			tb_index ++;

		} else if (type == editor::SCENARIO_DATA) {
			tapp_bin& bin = app_bins[app_bin_index ++];
			if (!bin.macros.empty()) {
				short_paths.push_back(bin.macros);
			}
			short_paths.push_back(bin.path + "/" + bin.id);
			filter |= SKIP_GUI_DIR | SKIP_INTERNAL_DIR | SKIP_BOOK;

			desc.bin_name = bin.id + ".bin";
			desc.app = bin.app;

			bin_to_path = working_dir_ + "/xwml/" + game_config::generate_app_dir(bin.app);

		} else if (type == editor::GUI) {
			short_paths.push_back("data/gui");

			desc.bin_name = BASENAME_GUI;

		} else if (type == editor::LANGUAGE) {
			short_paths.push_back("data/languages");

			desc.bin_name = BASENAME_LANGUAGE;

		} else {
			// (type == editor::MAIN_DATA)
			// no pre-defined
			short_paths.push_back("data");
			filter |= SKIP_SCENARIO_DIR | SKIP_GUI_DIR;

			desc.bin_name = BASENAME_DATA;
		}

		if (!calculated_wml_checksum) {
			data_tree_checksum(short_paths, dir_checksum, filter);
			desc.wml_nfiles = dir_checksum.nfiles;
			desc.wml_sum_size = dir_checksum.sum_size;
			desc.wml_modified = dir_checksum.modified;
		}

		if (!wml_checksum_from_file(bin_to_path + "/" + desc.bin_name, &desc.bin_nfiles, &desc.bin_sum_size, (uint32_t*)&desc.bin_modified)) {
			desc.bin_nfiles = desc.bin_sum_size = desc.bin_modified = 0;
		}

		wml2bin_descs_.push_back(std::pair<BIN_TYPE, wml2bin_desc>(type, desc));
	}

	return;
}

struct tcopy_cookie
{
	tcopy_cookie(const std::string& dir)
		: current_path(dir)
	{
		char c = current_path.at(current_path.size() - 1);
		if (c == '\\' || c == '/') {
			current_path.erase(current_path.size() - 1);
		}
	}

	bool cb_copy_cookie(const std::string& dir, const SDL_dirent2* dirent);

	std::string current_path;
};

bool tcopy_cookie::cb_copy_cookie(const std::string& dir, const SDL_dirent2* dirent)
{
	if (SDL_DIRENT_DIR(dirent->mode)) {
		tcopy_cookie ccp2(current_path + "/" + dirent->name);
		const std::string cookie_cki = "__cookie.cki";
		{
			tfile lock(ccp2.current_path + "/" + cookie_cki, GENERIC_WRITE, CREATE_ALWAYS);
		}

		if (!walk_dir(ccp2.current_path, false, boost::bind(&tcopy_cookie::cb_copy_cookie, &ccp2, _1, _2))) {
			return false;
		}
	}
	return true;
}

bool is_studio_app(const std::string& app)
{
	return app == "studio";
}

int file_delete_line(tfile& file, const char* ptr, int& fsize)
{
	// start
	int start = ptr - file.data;
	while (file.data[start] != '\r' && file.data[start] != '\n' && start > 0) {
		start --;
	}
	if (file.data[start] == '\r' || file.data[start] == '\n') {
		start ++;
	}

	int stop = ptr - file.data;
	while (file.data[stop] != '\r' && file.data[stop] != '\n' && stop < fsize) {
		stop ++;
	}
	while ((file.data[stop] == '\r' || file.data[stop] == '\n') && stop < fsize) {
		stop ++;
	}
	fsize = file.replace_span(start, stop - start, NULL, 0, fsize);

	return start;
}

std::string guid_2_str(const GUID& guid)
{
	std::stringstream ss;

	ss << std::setw(8) << std::setfill('0') << std::setbase(16) << guid.Data1 << "-";
	ss << std::setw(4) << std::setfill('0') << std::setbase(16) << guid.Data2 << "-";
	ss << std::setw(4) << std::setfill('0') << std::setbase(16) << guid.Data3 << "-";
	ss << std::setw(2) << std::setfill('0') << std::setbase(16) << (int)guid.Data4[0];
	ss << std::setw(2) << std::setfill('0') << std::setbase(16) << (int)guid.Data4[1] << "-";
	for (int at = 2; at < 8; at ++) {
		ss << std::setw(2) << std::setfill('0') << std::setbase(16) << (int)guid.Data4[at];
	}

	return ss.str();
}

bool do_new_app(display& disp, const tapp_capabilities& capabilities, tnewer& newer)
{
	const std::string& app = capabilities.app;
	GUID guid;
	CoCreateGuid(&guid);
	std::string guid_str = guid_2_str(guid);

	std::string src_path = directory_name2(game_config::path) + "/apps-src";
	std::string projectfiles = src_path + "/apps/projectfiles";
	if (!SDL_IsDirectory(projectfiles.c_str())) {
		return false;
	}

	newer.set_app(app);
	if (!newer.handle(disp)) {
		return false;
	}

	std::vector<std::pair<std::string, std::string> > replaces;
	//
	// prepare project files
	//
	const std::string app_in_prj = "studio";

	std::string src = projectfiles + "/windows-prj/" + app_in_prj + ".vcxproj";
	std::string dst = projectfiles + "/vc/" + app + ".vcxproj";
	replaces.push_back(std::make_pair(app_in_prj, app));
	replaces.push_back(std::make_pair(studio_guid, guid_str));
	file_replace_string(src, dst, replaces);

	src = projectfiles + "/windows-prj/" + app_in_prj + ".vcxproj.filters";
	dst = projectfiles + "/vc/" + app + ".vcxproj.filters";
	replaces.clear();
	replaces.push_back(std::make_pair(app_in_prj, app));
	file_replace_string(src, dst, replaces);

	src = projectfiles + "/windows-prj/" + app_in_prj + ".vcxproj.user";
	dst = projectfiles + "/vc/" + app + ".vcxproj.user";
	SDL_CopyFiles(src.c_str(), dst.c_str());

	if (!apps_sln::add_project(app, guid_str)) {
		return false;
	}
	
	//
	// prepare source code
	//
	file_replace_string(src_path + "/apps/" + app + "/main.cpp", std::make_pair("\"studio-lib\"", std::string("\"") + app + "-lib\""));

	const std::string res_po_path = game_config::path + "/po/" + game_config::generate_app_dir(app);
	file_replace_string(res_po_path + "/" + app + "-lib/" + app + "-lib.pot", std::make_pair("studio", app));

/*
	const std::string apps_app = src_path + "/apps/" + app;

	src = projectfiles + "/windows-prj/src2/main.cpp";
	dst = apps_app + "/main.cpp";
	replaces.clear();
	replaces.push_back(std::make_pair("\"rose-lib\"", std::string("\"") + app + "-lib\""));
	file_replace_string(src, dst, replaces);
*/
	// absolute/apps.cfg
	// now, app_copiers doesn't include new app. cann not use trose::generate_2_cfg().
	{
		src = game_config::absolute_path + "/apps.cfg";
		uint32_t create_disposition = file_exists(src)? OPEN_EXISTING: CREATE_ALWAYS;
		tfile file(src, GENERIC_WRITE, create_disposition);
		if (!file.valid()) {
			return false;
		}
		int fsize = file.read_2_data();
		posix_fseek(file.fp, fsize);
		std::stringstream ss;
		ss << "\n";
		capabilities.generate(ss, null_str);

		posix_fwrite(file.fp, ss.str().c_str(), ss.str().size());
	}

	return true;
}




const std::string tapp_copier::windows_prj_alias = "windows_prj";
const std::string tapp_copier::android_prj_alias = "android_prj";
const std::string tapp_copier::ios_prj_alias = "ios_prj";
const std::string tapp_copier::app_windows_prj_alias = "app_windows_prj";
const std::string tapp_copier::app_android_prj_alias = "app_android_prj";
const std::string tapp_copier::app_ios_prj_alias = "app_ios_prj";

tapp_copier::tapp_copier(const config& cfg)
	: tapp_capabilities(cfg["app"].str(), cfg["bundle_id"].str(), cfg["ble"].to_bool(), cfg["healthkit"].to_bool())
	, path_(cfg["path"].str())
{
	VALIDATE(bundle_id.valid(), null_str);
	if (app.empty()) {
		app = bundle_id.node(2);
	}
	if (path_.empty()) {
		path_ = "..";
	}

	tdomains.insert(app + "-lib");
	std::vector<std::string> extra_tdomains = utils::split(cfg["extra_textdomain"].str());
	for (std::vector<std::string>::const_iterator it = extra_tdomains.begin(); it != extra_tdomains.end(); ++ it) {
		tdomains.insert(*it);
	}
}

void tapp_copier::app_complete_paths(std::map<std::string, std::string>& paths) const
{
	VALIDATE(!app.empty() && !paths.empty(), null_str);

	const std::string& res_path = ::alias_2_path(paths, ttask::res_alias);
	const std::string& src2_path = ::alias_2_path(paths, ttask::src2_alias);

	// app_res/app_src maybe overlay be *.cfg.
	VALIDATE(paths.find(ttask::app_res_alias) == paths.end(), null_str);
	VALIDATE(paths.find(ttask::app_src2_alias) == paths.end(), null_str);
	VALIDATE(paths.find(ttask::app_src_alias) == paths.end(), null_str);

	std::string app_res_path = res_path + "/" + path_;
	std::string app_src2_path = res_path + "/" + path_;
	if (is_studio_app(app)) {
		app_res_path += "/apps-res";
		app_src2_path += "/apps-src/apps";
	} else {
		app_res_path += "/" + app + "-res";
		app_src2_path += "/" + app + "-src/" + app;
	}
	app_res_path = utils::normalize_path(app_res_path);
	app_src2_path = utils::normalize_path(app_src2_path);

	VALIDATE(res_path != app_res_path && src2_path != app_src2_path, null_str);

	paths.insert(std::make_pair(ttask::app_res_alias, app_res_path));
	paths.insert(std::make_pair(ttask::app_src2_alias, app_src2_path));
	paths.insert(std::make_pair(ttask::app_src_alias, directory_name2(app_src2_path)));


	paths.insert(std::make_pair(windows_prj_alias, src2_path + "/projectfiles/windows-prj"));
	paths.insert(std::make_pair(app_windows_prj_alias, app_src2_path + "/projectfiles/vc"));
	paths.insert(std::make_pair(android_prj_alias, src2_path + "/projectfiles/android-prj"));
	paths.insert(std::make_pair(app_android_prj_alias, app_src2_path + "/projectfiles/android"));
	paths.insert(std::make_pair(ios_prj_alias, src2_path + "/projectfiles/ios-prj"));
	paths.insert(std::make_pair(app_ios_prj_alias, app_src2_path + "/projectfiles/Xcode-iOS"));
}


// const std::string& app_res_path = alias_2_path(app_res_alias)
// const std::string& app_ios_prj_path = alias_2_path(tapp_copier::app_ios_prj_alias)
static bool generate_ios_prj(display& disp, const tapp_copier& copier, const std::string& app_res_path, const std::string& app_ios_prj_path)
{
	const std::string app_in_prj = "studio";

	if (app_in_prj != copier.app) {
		// rename <app_ios_prj>/studio to <app_ios_prj>/<app>
		SDL_RenameFile((app_ios_prj_path + "/" + app_in_prj).c_str(), copier.app.c_str());
	}

	// rename <app_ios_prj>/studio.xcodeproj to <app_ios_prj>/<app>.xcodeproj
	const std::string app_xcodeproj = copier.app + ".xcodeproj";
	if (app_in_prj != copier.app) {
		SDL_RenameFile((app_ios_prj_path + "/" + app_in_prj + ".xcodeproj").c_str(), app_xcodeproj.c_str());
	}

	// <app_ios_prj>/<app>.xcodeproj/project.pbxproj
	const std::string pbxproj_tmp = "project.pbxproj.tmp";
	std::string src = app_ios_prj_path + "/" + app_xcodeproj + "/" + pbxproj_tmp;
	std::string dst = app_ios_prj_path + "/" + app_xcodeproj + "/project.pbxproj";
	SDL_RenameFile(dst.c_str(), pbxproj_tmp.c_str());
	{
		std::vector<std::string> vstr;
		tfile file(src, GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';

		std::string prefix;
		const char* ptr = NULL;
		if (!copier.healthkit) {
			// delete all line that include "studio.entitlements"
			prefix = app_in_prj + ".entitlements";
			ptr = strstr(file.data, prefix.c_str());
			while (ptr) {
				int start = file_delete_line(file, ptr, fsize);
				ptr = NULL;
				if (start < fsize) {
					ptr = strstr(file.data + start, prefix.c_str());
				}
			}

			// delete SystemCapabilities = { }
			prefix = "SystemCapabilities = {";
			ptr = strstr(file.data, prefix.c_str());
			if (!ptr) {
				return false;
			}
			int start = ptr - file.data;
			while (file.data[start] != '\r' && file.data[start] != '\n' && start > 0) {
				start --;
			}
			if (file.data[start] == '\r' || file.data[start] == '\n') {
				start ++;
			}

			prefix = "};";
			for (int layer = 0; layer < 2; layer ++) {
				ptr = strstr(ptr, prefix.c_str());
				if (!ptr) {
					return false;
				}
				ptr += prefix.size();
			}
			int stop = ptr - file.data;
			while (file.data[stop] != '\r' && file.data[stop] != '\n' && stop < fsize) {
				stop ++;
			}
			while ((file.data[stop] == '\r' || file.data[stop] == '\n') && stop < fsize) {
				stop ++;
			}
			fsize = file.replace_span(start, stop - start, NULL, 0, fsize);
		}

		if (app_in_prj != copier.app) {
			// studio
			prefix = app_in_prj;
			ptr = strstr(file.data, prefix.c_str());
			while (ptr) {
				fsize = file.replace_span(ptr - file.data, prefix.size(), copier.app.c_str(), copier.app.size(), fsize);
				// replace next
				ptr = strstr(ptr, prefix.c_str());
			}
		}

		// PRODUCT_BUNDLE_IDENTIFIER, new bundle_id maybe include "studio", so place after replace app.
		prefix = "PRODUCT_BUNDLE_IDENTIFIER = ";
		ptr = strstr(file.data, prefix.c_str());
		while (ptr) {
			ptr = utils::skip_blank_characters(ptr + prefix.size());
			const char* ptr2 = utils::until_c_style_characters(ptr);
			fsize = file.replace_span(ptr - file.data, ptr2 - ptr, copier.bundle_id.id().c_str(), copier.bundle_id.size(), fsize);
			// replace next
			ptr = strstr(ptr, prefix.c_str());
		}

		// write data to new file
		tfile file2(dst, GENERIC_WRITE, CREATE_ALWAYS);
		if (file2.valid()) {
			posix_fseek(file2.fp, 0);
			posix_fwrite(file2.fp, file.data, fsize);
		}
	}
	SDL_DeleteFiles(src.c_str());

	// <app_ios_prj>/Info.plist
	const std::string plist_tmp = "Info.plist.tmp";
	src = app_ios_prj_path + "/" + plist_tmp;
	dst = app_ios_prj_path + "/Info.plist";
	SDL_RenameFile(dst.c_str(), plist_tmp.c_str());
	{
		std::vector<std::string> vstr;
		tfile file(src, GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 1024 bytes.
		file.resize_data(fsize + 1024);
		file.data[fsize] = '\0';

		std::string prefix;
		const char* ptr = NULL;
		if (!copier.healthkit) {
			// delete all line that include "studio.entitlements"
			prefix = "<string>healthkit</string>";
			ptr = strstr(file.data, prefix.c_str());
			while (ptr) {
				int start = file_delete_line(file, ptr, fsize);
				ptr = NULL;
				if (start < fsize) {
					ptr = strstr(file.data + start, prefix.c_str());
				}
			}
		}

		// write data to new file
		tfile file2(dst, GENERIC_WRITE, CREATE_ALWAYS);
		if (file2.valid()) {
			posix_fseek(file2.fp, 0);
			posix_fwrite(file2.fp, file.data, fsize);
		}
	}
	SDL_DeleteFiles(src.c_str());

	src = app_ios_prj_path + "/" + copier.app + "/" + app_in_prj + ".entitlements";
	if (copier.healthkit) {
		SDL_RenameFile(src.c_str(), (copier.app + ".entitlements").c_str());
	} else {
		SDL_DeleteFiles(src.c_str());
	}

	if (app_in_prj != copier.app) {
		// Images.xcassets
		src = app_res_path + "/" + game_config::generate_app_dir(copier.app) + "/Images.xcassets";
		dst = app_ios_prj_path + "/" + copier.app;
		SDL_CopyFiles(src.c_str(), dst.c_str());
	}

	return true;
}

void texporter::app_complete_paths()
{
	copier_.app_complete_paths(paths_);
}

bool texporter::app_post_handle(display& disp, const tsubtask& subtask, const bool last)
{
	if (!last) {
		return true;
	}

	if (!generate_window_prj(disp)) {
		return false;
	}

	if (!generate_android_prj(disp)) {
		return false;
	}

	if (!generate_ios_prj(disp, copier_, alias_2_path(app_res_alias), alias_2_path(tapp_copier::app_ios_prj_alias))) {
		return false;
	}

	return true;
}

std::pair<std::string, std::string> texporter::app_get_replace(const tsubtask& subtask)
{
	VALIDATE(!copier_.app.empty(), null_str);

	const std::string replacee = "<new_>";
	return std::make_pair(replacee, copier_.app);
}

bool texporter::generate_window_prj(display& disp) const
{
	const std::string app_in_prj = "studio";
	bool is_apps = is_studio_app(copier_.app);

	const std::string& windows_prj_path = alias_2_path(tapp_copier::windows_prj_alias);
	const std::string& app_windows_prj_path = alias_2_path(tapp_copier::app_windows_prj_alias);

	// *.sln
	std::string src = windows_prj_path + "/apps.sln";
	std::string dst = app_windows_prj_path + "/" + (is_apps? "apps": copier_.app) + ".sln";
	std::string guid_str;
	{
		tfile file(app_windows_prj_path + "/" + copier_.app + ".vcxproj", GENERIC_READ, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		const char* start2 = file.data;
		if (utils::bom_magic_started((const uint8_t*)file.data, fsize)) {
			start2 += BOM_LENGTH;
		}

		// studio
		std::string prefix = "<ProjectGuid>{";
		const char* ptr = strstr(start2, prefix.c_str());
		if (!ptr) {
			return false;
		}
		std::string postfix = "}</ProjectGuid>";
		ptr += prefix.size();
		const char* ptr2 = strstr(ptr, postfix.c_str());
		const int guid_size2 = 36;
		guid_str.assign(ptr, ptr2 - ptr);
		if (guid_str.size() != guid_size2) {
			return false;
		}
	}
	{
		tfile file(src,  GENERIC_READ, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512, fsize);
		file.data[fsize] = '\0';

		const char* start2 = file.data;
		if (utils::bom_magic_started((const uint8_t*)file.data, fsize)) {
			start2 += BOM_LENGTH;
		}

		// studio
		std::string prefix = app_in_prj;
		const char* ptr = NULL;
		if (prefix != copier_.app) {
			ptr = strstr(start2, prefix.c_str());
			while (ptr) {
				fsize = file.replace_span(ptr - file.data, prefix.size(), copier_.app.c_str(), copier_.app.size(), fsize);
				// replace next
				ptr = strstr(ptr, prefix.c_str());
			}
		}

		// guid of studio: 3641E31E-36BF-4E03-8879-DE33ADC07D68
		prefix = studio_guid;
		if (prefix != guid_str) {
			ptr = strstr(start2, prefix.c_str());
			while (ptr) {
				fsize = file.replace_span(ptr - file.data, prefix.size(), guid_str.c_str(), guid_str.size(), fsize);
				// replace next
				ptr = strstr(ptr, prefix.c_str());
			}
		}

		// write data to new file
		tfile file2(dst,  GENERIC_WRITE, CREATE_ALWAYS);
		if (file2.valid()) {
			posix_fseek(file2.fp, 0);
			posix_fwrite(file2.fp, file.data, fsize);
		}
	}

	// *.vcxproj.user
	src = windows_prj_path + "/" + app_in_prj + ".vcxproj.user";
	dst = app_windows_prj_path + "/" + copier_.app + ".vcxproj.user";

	std::vector<std::pair<std::string, std::string> > replaces;
	replaces.push_back(std::make_pair("apps-res", is_apps? "apps-res": copier_.app + "-res"));
	file_replace_string(src, dst, replaces);
	
	return true;
}

bool texporter::generate_android_prj(display& disp) const
{
	std::stringstream ss;
	const std::string app_in_prj = "studio";
	const tdomain default_bundle_id("com.leagor.studio");

	// <app_android_prj>/app/src/main/AndroidManifest.xml
	const std::string& app_android_prj_path = alias_2_path(tapp_copier::app_android_prj_alias);
	const std::string androidmanifest_xml_tmp = "AndroidManifest.xml.tmp";
	std::string src = app_android_prj_path + "/app/src/main/" + androidmanifest_xml_tmp;
	std::string dst = app_android_prj_path + "/app/src/main/AndroidManifest.xml";
	SDL_RenameFile(dst.c_str(), androidmanifest_xml_tmp.c_str());
	{
		tfile file(src, GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';
		// replace with app's bundle id.
		const char* prefix = "\"http://schemas.android.com/apk/res/android\"";
		const char* ptr = strstr(file.data, prefix);
		if (!ptr) {
			return false;
		}
		ptr = utils::skip_blank_characters(ptr + strlen(prefix) + 1);
		if (memcmp(ptr, "package=\"", 9)) {
			return false;
		}
		ptr += 9;
		if (memcmp(ptr, default_bundle_id.id().c_str(), default_bundle_id.size())) {
			return false;
		}
		if (app_in_prj != copier_.app) {
			fsize = file.replace_span(ptr - file.data, default_bundle_id.size(), copier_.bundle_id.id().c_str(), copier_.bundle_id.size(), fsize);
		}
		file.data[fsize] = '\0';

		if (copier_.ble) {
			// insert ble permission.
			const char* prefix2 = "<uses-permission android:name=\"android.permission.MODIFY_AUDIO_SETTINGS\"/>";
			ptr = strstr(ptr, prefix2);
			if (!ptr) {
				return false;
			}
			ptr = utils::skip_blank_characters(ptr + strlen(prefix2) + 1);
			std::stringstream permission;
			permission << "<uses-permission android:name=\"android.permission.BLUETOOTH\" />\r\n";
			permission << "    <uses-permission android:name=\"android.permission.BLUETOOTH_ADMIN\" />\r\n";
			permission << "    <uses-permission android:name=\"android.permission.BLUETOOTH_PRIVILEGED\" />\r\n";
			permission << "    <uses-permission android:name=\"android.permission.ACCESS_COARSE_LOCATION\"/>\r\n";
			permission << "\r\n    ";
			fsize = file.replace_span(ptr - file.data, 0, permission.str().c_str(), permission.str().size(), fsize);
		}

		// write data to new file
		tfile file2(dst, GENERIC_WRITE, CREATE_ALWAYS);
		if (file2.valid()) {
			posix_fseek(file2.fp, 0);
			posix_fwrite(file2.fp, file.data, fsize);
		}
	}
	SDL_DeleteFiles(src.c_str());

	// <app_android_prj>/app/src/main/java/com/leagor/studio/app.java
	if (app_in_prj != copier_.app) {
		{
			ss.str("");
			ss << "/app/src/main/java/" + default_bundle_id.node(0) + "/" + default_bundle_id.node(1) + "/" + default_bundle_id.node(2) + "/app.java";
			tfile file(app_android_prj_path + ss.str(), GENERIC_WRITE, OPEN_EXISTING);
			int fsize = file.read_2_data();
			if (!fsize) {
				return false;
			}
			// i think length of appended data isn't more than 512 bytes.
			file.resize_data(fsize + 512);
			file.data[fsize] = '\0';
			// replace with app's bundle id.
			const char* prefix = "package";
			const char* ptr = strstr(file.data, prefix);
			if (!ptr) {
				return false;
			}
			ptr = utils::skip_blank_characters(ptr + strlen(prefix) + 1);
			fsize = file.replace_span(ptr - file.data, default_bundle_id.size(), copier_.bundle_id.id().c_str(), copier_.bundle_id.size(), fsize);

			std::string app_java_dir = app_android_prj_path + "/app/src/main/java/" + copier_.bundle_id.node(0) + "/" + copier_.bundle_id.node(1) + "/" + copier_.bundle_id.node(2);
			SDL_MakeDirectory(app_java_dir.c_str());
			tfile file2(app_java_dir + "/app.java", GENERIC_WRITE, CREATE_ALWAYS);
			posix_fwrite(file2.fp, file.data, fsize);
		}
		ss.str("");
		if (default_bundle_id.node(0) != copier_.bundle_id.node(0)) {
			ss << "/app/src/main/java/" + default_bundle_id.node(0);
		} else if (default_bundle_id.node(1) != copier_.bundle_id.node(1)) {
			ss << "/app/src/main/java/" + default_bundle_id.node(0) + "/" + default_bundle_id.node(1);
		} else {
			ss << "/app/src/main/java/" + default_bundle_id.node(0) + "/" + default_bundle_id.node(1) + "/" + default_bundle_id.node(2);
		}
		SDL_DeleteFiles((app_android_prj_path + ss.str()).c_str());
	}

	// <app_android_prj>/app/build.gradle
	if (app_in_prj != copier_.app) {
		const std::string build_gradle_tmp = "build.gradle.tmp";
		src = app_android_prj_path + "/app/" + build_gradle_tmp;
		dst = app_android_prj_path + "/app/build.gradle";
		SDL_RenameFile(dst.c_str(), build_gradle_tmp.c_str());
		{
			tfile file(src, GENERIC_WRITE, OPEN_EXISTING);
			int fsize = file.read_2_data();
			if (!fsize) {
				return false;
			}
			// i think length of appended data isn't more than 512 bytes.
			file.resize_data(fsize + 512);
			file.data[fsize] = '\0';
			// replace with app's bundle id.
			const char* prefix = "defaultConfig {";
			const char* ptr = strstr(file.data, prefix);
			if (!ptr) {
				return false;
			}
			ptr = utils::skip_blank_characters(ptr + strlen(prefix) + 1);
			if (memcmp(ptr, "applicationId", 13)) {
				return false;
			}
			ptr += 13;
			ptr = utils::skip_blank_characters(ptr);
			if (ptr[0] != '\"') {
				return false;
			}
			ptr ++;
			if (memcmp(ptr, default_bundle_id.id().c_str(), default_bundle_id.size())) {
				return false;
			}
			fsize = file.replace_span(ptr - file.data, default_bundle_id.size(), copier_.bundle_id.id().c_str(), copier_.bundle_id.size(), fsize);
			file.data[fsize] = '\0';

			// write data to new file
			tfile file2(dst, GENERIC_WRITE, CREATE_ALWAYS);
			if (file2.valid()) {
				posix_fseek(file2.fp, 0);
				posix_fwrite(file2.fp, file.data, fsize);
			}
		}
		SDL_DeleteFiles(src.c_str());
	}

	// <android-prj>/app/src/main/res/values/strings.xml
	if (app_in_prj != copier_.app) {
		const std::string strings_xml_tmp = "strings.xml.tmp";
		src = app_android_prj_path + "/app/src/main/res/values/" + strings_xml_tmp;
		dst = app_android_prj_path + "/app/src/main/res/values/strings.xml";
		SDL_RenameFile(dst.c_str(), strings_xml_tmp.c_str());
		{
			tfile file(src, GENERIC_WRITE, OPEN_EXISTING);
			int fsize = file.read_2_data();
			if (!fsize) {
				return false;
			}
			// i think length of appended data isn't more than 512 bytes.
			file.resize_data(fsize + 512);
			file.data[fsize] = '\0';
			// replace with app's bundle id.
			const char* prefix = "<string name=\"app_name\">";
			const char* ptr = strstr(file.data, prefix);
			if (!ptr) {
				return false;
			}
			ptr = utils::skip_blank_characters(ptr + strlen(prefix));
			fsize = file.replace_span(ptr - file.data, app_in_prj.size(), copier_.app.c_str(), copier_.app.size(), fsize);

			// write data to new file
			tfile file2(dst, GENERIC_WRITE, CREATE_ALWAYS);
			if (file2.valid()) {
				posix_fseek(file2.fp, 0);
				posix_fwrite(file2.fp, file.data, fsize);
			}
		}
		SDL_DeleteFiles(src.c_str());
	}

	// <android_prj>/app/jni/Android.mk
	if (app_in_prj != copier_.app) {
		const std::string android_mk_tmp = "Android.mk.tmp";
		src = app_android_prj_path + "/app/jni/" + android_mk_tmp;
		dst = app_android_prj_path + "/app/jni/Android.mk";
		SDL_RenameFile(dst.c_str(), android_mk_tmp.c_str());
		{
			std::vector<std::string> vstr;
			tfile file(src, GENERIC_WRITE, OPEN_EXISTING);
			int fsize = file.read_2_data();
			if (!fsize) {
				return false;
			}
			// i think length of appended data isn't more than 512 bytes.
			file.resize_data(fsize + 512);
			file.data[fsize] = '\0';

			std::string prefix;
			const char* ptr = NULL;

			if (app_in_prj != copier_.app) {
				// studio
				prefix = app_in_prj;
				ptr = strstr(file.data, prefix.c_str());
				while (ptr) {
					fsize = file.replace_span(ptr - file.data, prefix.size(), copier_.app.c_str(), copier_.app.size(), fsize);
					// replace next
					ptr = strstr(ptr, prefix.c_str());
				}
			}

			// write data to new file
			tfile file2(dst, GENERIC_WRITE, CREATE_ALWAYS);
			if (file2.valid()) {
				posix_fseek(file2.fp, 0);
				posix_fwrite(file2.fp, file.data, fsize);
			}
		}
		SDL_DeleteFiles(src.c_str());
	}

	std::string app_src_path = alias_2_path(app_src_alias);
	std::string app_src2_path = alias_2_path(app_src2_alias);
	std::replace(app_src_path.begin(), app_src_path.end(), path_sep(false), path_sep(true));
	// <app_src>/scripts/android_set_variable.tpl
	{
		std::vector<std::pair<std::string, std::string> > replaces;
		replaces.push_back(std::make_pair("%_APP_SRC%", app_src_path));
		file_replace_string(app_src_path + "/scripts/android_set_variable.tpl", app_src_path + "/scripts/android_set_variable.bat", replaces);

		tfile file2(app_src_path + "/scripts/android_set_variable.bat", GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file2.read_2_data();
		// i think length of appended data isn't more than 512 bytes.
		file2.resize_data(fsize + 512);
		file2.data[fsize] = '\0';

		// app directory
		std::stringstream app_dir;
		app_dir << "\n\nset " << copier_.app << "=" << app_src2_path << "/projectfiles/android/app";
		
		std::string app_dir_str = app_dir.str();
		std::replace(app_dir_str.begin(), app_dir_str.end(), path_sep(false), path_sep(true));
		fsize = file2.replace_span(fsize, 0, app_dir_str.c_str(), app_dir_str.size(), fsize);

		posix_fseek(file2.fp, 0);
		posix_fwrite(file2.fp, file2.data, fsize);
	}
	if (!is_studio_app(copier_.app)) {
		SDL_DeleteFiles((app_src_path + "/scripts/android_set_variable.tpl").c_str());
	}

	return true;
}





void tstudio_extra_exporter::app_complete_paths()
{
	copier_.app_complete_paths(paths_);
}


void tandroid_res::app_complete_paths()
{
	copier_.app_complete_paths(paths_);
}

bool tandroid_res::app_can_execute(const tsubtask& subtask, const bool last)
{
	if (!last) {
		return true;
	}
	return is_directory(get_android_res_path());
}

bool tandroid_res::app_post_handle(display& disp, const tsubtask& subtask, const bool last)
{
	if (!last) {
		return true;
	}

	{
		// generate __cookie.cki on every directory.
		// "<apps-src/apps>/projectfiles/android/app/src/main/assets/res"
		tcopy_cookie ccp(alias_2_path(app_src2_alias) + "/projectfiles/android/app/src/main/assets/res");
		walk_dir(ccp.current_path, false, boost::bind(&tcopy_cookie::cb_copy_cookie, &ccp, _1, _2));
	}

	std::stringstream ss;
	{
		// generate __cookie.cki on every directory.
		// <blesmart-src>/projectfiles/android/<new_>/src/main/assets/res
		tcopy_cookie ccp(get_android_res_path());
		walk_dir(ccp.current_path, false, boost::bind(&tcopy_cookie::cb_copy_cookie, &ccp, _1, _2));
	}

	return true;
}

std::pair<std::string, std::string> tandroid_res::app_get_replace(const tsubtask& subtask)
{
	VALIDATE(!copier_.app.empty(), null_str);

	const std::string replacee = "<new_>";
	return std::make_pair(replacee, copier_.app);
}

std::string tandroid_res::get_android_res_path()
{
	std::stringstream ss;

	ss << alias_2_path(ttask::src2_alias);
	ss << "/projectfiles/android/";
	ss << copier_.app;
	ss << "/src/main/assets/res";

	return ss.str();
}


const std::string tios_kit::kit_alias = "kit";
const std::string tios_kit::studio_alias = "studio";

bool tios_kit::app_post_handle(display& disp, const tsubtask& subtask, const bool last)
{
	if (!last) {
		return true;
	}

	// const std::string& app_res_path = alias_2_path(app_res_alias)
	// const std::string& app_ios_prj_path = alias_2_path(tapp_copier::app_ios_prj_alias)
	const std::string& studio_path = alias_2_path(studio_alias);
	if (!generate_ios_prj(disp, copier_, studio_path, studio_path + "/projectfiles/Xcode-iOS")) {
		return false;
	}

	return true;
}

std::pair<std::string, std::string> tios_kit::app_get_replace(const tsubtask& subtask)
{
	const std::string replacee = "<new_>";
	return std::make_pair(replacee, "studio");
}


const std::string tnewer::windows_prj_alias = "windows_prj";

void tnewer::app_complete_paths()
{
	const std::string& src2_path = ::alias_2_path(paths_, ttask::src2_alias);

	paths_.insert(std::make_pair(windows_prj_alias, src2_path + "/projectfiles/windows-prj"));
}

std::pair<std::string, std::string> tnewer::app_get_replace(const tsubtask& subtask)
{
	VALIDATE(!app_.empty(), null_str);

	const std::string replacee = "<new_>";
	return std::make_pair(replacee, app_);
}



std::pair<std::string, std::string> tremover::app_get_replace(const tsubtask& subtask)
{
	VALIDATE(!app_.empty(), null_str);

	const std::string replacee = "<new_>";
	return std::make_pair(replacee, app_);
}



const std::string tvalidater::windows_prj_alias = "windows_prj";

void tvalidater::app_complete_paths()
{
	const std::string& src2_path = ::alias_2_path(paths_, ttask::src2_alias);

	paths_.insert(std::make_pair(windows_prj_alias, src2_path + "/projectfiles/windows-prj"));
}

std::pair<std::string, std::string> tvalidater::app_get_replace(const tsubtask& subtask)
{
	VALIDATE(!app_.empty(), null_str);

	const std::string replacee = "<new_>";
	return std::make_pair(replacee, app_);
}
