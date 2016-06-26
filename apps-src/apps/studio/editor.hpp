#ifndef STUDIO_EDITOR_HPP_INCLUDED
#define STUDIO_EDITOR_HPP_INCLUDED

#include "config_cache.hpp"
#include "config.hpp"
#include "version.hpp"
#include "task.hpp"

#include <set>

#include <boost/bind.hpp>
#include <boost/function.hpp>

class display;

class tapp_capabilities
{
public:
	tapp_capabilities(const std::string& app, const std::string& bundle_id, bool ble = false, const bool healthkit = false)
		: app(app)
		, bundle_id(bundle_id)
		, ble(ble)
		, healthkit(healthkit)
	{}

	bool operator==(const tapp_capabilities& that) const
	{
		if (app != that.app) return false;
		if (bundle_id.id() != that.bundle_id.id()) return false;
		if ((ble && !that.ble) || (!ble && that.ble)) return false;
		if ((healthkit && !that.healthkit) || (!healthkit && that.healthkit)) return false;
		return true;
	}
	bool operator!=(const tapp_capabilities& that) const { return !operator==(that); }

	void reset(const tapp_capabilities& that)
	{
		app = that.app;
		bundle_id.reset(that.bundle_id.id());
		ble = that.ble;
		healthkit = that.healthkit;
	}

	void generate(std::stringstream& ss, const std::string& prefix) const;

public:
	std::string app;
	tdomain bundle_id;
	bool ble;
	bool healthkit;
	std::set<std::string> tdomains;
};

bool is_apps_kit();
bool is_studio_app(const std::string& app);
bool is_private_app(const std::string& app);
bool is_reserve_app(const std::string& app);

bool check_res_folder(const std::string& folder);
bool check_apps_src_folder(const std::string& folder);

namespace apps_sln {
std::set<std::string> apps_in();
bool add_project(const std::string& app, const std::string& guid_str);
bool remove_project(const std::string& app);
}

enum {BIN_WML, BIN_BUILDINGRULE};
#define BINKEY_ID_CHILD			"id_child"
#define BINKEY_SCENARIO_CHILD	"scenario_child"
#define BINKEY_PATH				"path"
#define BINKEY_MACROS			"macros"

namespace editor_config
{
	extern config data_cfg;
	extern std::string campaign_id;
	extern int type;

	void reload_data_bin();
}

class editor
{
public:
	class tres_path_lock
	{
	public:
		tres_path_lock(editor& o);
		~tres_path_lock();

	private:
		static int deep;
		std::string original_;
	};

	enum BIN_TYPE {BIN_MIN = 0, MAIN_DATA = BIN_MIN, GUI, LANGUAGE, BIN_SYSTEM_MAX = LANGUAGE, TB_DAT, SCENARIO_DATA, EXTENDABLE};
	struct wml2bin_desc {
		wml2bin_desc();
		std::string bin_name;
		std::string app;
		uint32_t wml_nfiles;
		uint32_t wml_sum_size;
		time_t wml_modified;
		uint32_t bin_nfiles;
		uint32_t bin_sum_size;
		time_t bin_modified;
		bool require_build;

		bool valid() const { return !bin_name.empty(); }
		void refresh_checksum(const std::string& working_dir);
	};
	struct tapp_bin {
		tapp_bin(const std::string& id, const std::string& app, const std::string& path, const std::string& macros)
			: id(id)
			, app(app)
			, path(path)
			, macros(macros)
		{}
		std::string id;
		std::string app;
		std::string path;
		std::string macros;
	};

	editor(const std::string& working_dir);

	void set_working_dir(const std::string& dir);
	const std::string& working_dir() { return working_dir_; }

	bool make_system_bins_exist();

	bool cfgs_2_cfg(const BIN_TYPE type, const std::string& name, const std::string& app, bool write_file, uint32_t nfiles = 0, uint32_t sum_size = 0, uint32_t modified = 0, const std::map<std::string, std::string>& app_domains = std::map<std::string, std::string>());
	void get_wml2bin_desc_from_wml(const std::vector<editor::BIN_TYPE>& system_bin_types);
	void reload_extendable_cfg();
	std::string check_scenario_cfg(const config& scenario_cfg);
	std::string check_mplayer_bin(const config& mplayer_cfg);
	std::string check_data_bin(const config& data_cfg);

	std::vector<std::pair<BIN_TYPE, wml2bin_desc> >& wml2bin_descs() { return wml2bin_descs_; }
	const std::vector<std::pair<BIN_TYPE, wml2bin_desc> >& wml2bin_descs() const { return wml2bin_descs_; }

	const config& campaigns_config() const { return campaigns_config_; }
	const std::string& working_dir() const { return working_dir_; }

private:
	void generate_app_bin_config();

private:
	std::string working_dir_;
	config campaigns_config_;
	config tbs_config_;
	game_config::config_cache& cache_;
	std::vector<std::pair<BIN_TYPE, wml2bin_desc> > wml2bin_descs_;
};

class tapp_copier;

class texporter: public ttask
{
public:
	texporter(const config& cfg, void* copier)
		: ttask(cfg)
		, copier_(*(reinterpret_cast<tapp_copier*>(copier)))
	{}

private:
	void app_complete_paths() override;
	bool app_post_handle(display& disp, const tsubtask& subtask, const bool last) override;
	std::pair<std::string, std::string> app_get_replace(const tsubtask& subtask) override;

	bool generate_android_prj(display& disp) const;
	bool generate_window_prj(display& disp) const;

private:
	tapp_copier& copier_;
};

class tstudio_extra_exporter: public ttask
{
public:
	tstudio_extra_exporter(const config& cfg, void* copier)
		: ttask(cfg)
		, copier_(*(reinterpret_cast<tapp_copier*>(copier)))
	{}

private:
	void app_complete_paths() override;

private:
	tapp_copier& copier_;
};

class tandroid_res: public ttask
{
public:
	tandroid_res(const config& cfg, void* copier)
		: ttask(cfg)
		, copier_(*(reinterpret_cast<tapp_copier*>(copier)))
	{}

private:
	void app_complete_paths() override;
	bool app_can_execute(const tsubtask& subtask, const bool last) override;
	bool app_post_handle(display& disp, const tsubtask& subtask, const bool last) override;
	std::pair<std::string, std::string> app_get_replace(const tsubtask& subtask) override;

	std::string get_android_res_path();

private:
	tapp_copier& copier_;
};

class tios_kit: public ttask
{
public:
	static const std::string kit_alias;
	static const std::string studio_alias;

	tios_kit(const config& cfg, void* copier)
		: ttask(cfg)
		, copier_(*(reinterpret_cast<tapp_copier*>(copier)))
	{}

private:
	bool app_post_handle(display& disp, const tsubtask& subtask, const bool last) override;
	std::pair<std::string, std::string> app_get_replace(const tsubtask& subtask) override;

private:
	tapp_copier& copier_;
};

class tapp_copier: public tapp_capabilities
{
public:
	static const std::string windows_prj_alias;
	static const std::string android_prj_alias;
	static const std::string ios_prj_alias;
	static const std::string app_windows_prj_alias;
	static const std::string app_android_prj_alias;
	static const std::string app_ios_prj_alias;

	tapp_copier(const config& cfg);

	void app_complete_paths(std::map<std::string, std::string>& paths) const;
	bool generate_ios_prj(display& disp, const ttask& task) const;

public:
	std::unique_ptr<texporter> exporter;
	std::unique_ptr<tstudio_extra_exporter> studio_extra_exporter;
	std::unique_ptr<tandroid_res> android_res_copier;
	std::unique_ptr<tios_kit> ios_kiter;

private:
	std::string path_;
};

class tnewer: public ttask
{
public:
	static const std::string windows_prj_alias;

	tnewer(const config& cfg, void*)
		: ttask(cfg)
	{}
	void set_app(const std::string& app) { app_ = app; }

private:
	void app_complete_paths() override;
	std::pair<std::string, std::string> app_get_replace(const tsubtask& subtask) override;

private:
	std::string app_;
};

bool do_new_app(display& disp, const tapp_capabilities& desc, tnewer& newer);

class tremover: public ttask
{
public:
	tremover(const config& cfg, void*)
		: ttask(cfg)
	{}
	void set_app(const std::string& app) { app_ = app; }

private:
	std::pair<std::string, std::string> app_get_replace(const tsubtask& subtask) override;

private:
	std::string app_;
};

class tvalidater: public ttask
{
public:
	static const std::string windows_prj_alias;

	tvalidater(const config& cfg, void*)
		: ttask(cfg)
	{}
	void set_app(const std::string& app) { app_ = app; }

private:
	void app_complete_paths() override;
	std::pair<std::string, std::string> app_get_replace(const tsubtask& subtask) override;

private:
	std::string app_;
};

#endif