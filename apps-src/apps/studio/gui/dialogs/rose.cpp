/* $Id: title_screen.cpp 48740 2011-03-05 10:01:34Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "studio-lib"

#include "gui/dialogs/rose.hpp"

#include "display.hpp"
#include "game_config.hpp"
#include "preferences.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/dialogs/browse.hpp"
#include "gui/dialogs/capabilities.hpp"
#include "gui/dialogs/menu.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/tree_view.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/toggle_panel.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/track.hpp"
#include "gui/widgets/window.hpp"
#include "preferences_display.hpp"
#include "help.hpp"
#include "version.hpp"
#include "filesystem.hpp"
#include "loadscreen.hpp"
#include <hero.hpp>
#include <time.h>
#include "sound.hpp"

#include <boost/bind.hpp>

#include <algorithm>

#include "sdl_image.h"

namespace gui2 {

REGISTER_DIALOG(studio, rose)

trose::trose(display& disp, hero& player_hero)
	: disp_(disp)
	, player_hero_(player_hero)
	, window_(NULL)
	, build_msg_data_(build_msg_data(build_normal, null_str))
	, current_copier_(NULL)
{
}

trose::~trose()
{
}

static const char* menu_items[] = {
	"edit_dialog",
	"player",
	"edit_theme",
	"language",
	"chat",
	"preferences",
	"design"
};
static int nb_items = sizeof(menu_items) / sizeof(menu_items[0]);

static std::vector<int> ids;
void trose::pre_show(CVideo& video, twindow& window)
{
	ids.clear();
	window_ = &window;

	set_restore(false);
	window.set_escape_disabled(true);

	// Set the version number
	tcontrol* control = find_widget<tcontrol>(&window, "revision_number", false, false);
	if (control) {
		control->set_label(_("V") + game_config::version);
	}
	window.canvas()[0].set_variable("revision_number", variant(_("Version") + std::string(" ") + game_config::version));
	window.canvas()[0].set_variable("background_image",	variant("misc/white-background.png"));

	/***** Set the logo *****/
	tcontrol* logo = find_widget<tcontrol>(&window, "logo", false, false);
	if (logo) {
		logo->set_label(game_config::logo_png);
	}

	tbutton* b;
	for (int item = 0; item < nb_items; item ++) {
		b = find_widget<tbutton>(&window, menu_items[item], false, false);
		if (!b) {
			continue;
		}
		std::string str;
		if (!strcmp(menu_items[item], "player")) {
			str = "hero-256/100.png";

		} else {
			str = std::string("icons/") + menu_items[item] + ".png";
		}

		for (int i = 0; i < 4; i ++) {
			b->canvas()[i].set_variable("image", variant(str));
		}
	}

	for (int item = 0; item < nb_items; item ++) {
		std::string id = menu_items[item];
		int retval = twindow::NONE;
		if (id == "edit_dialog") {
			retval = EDIT_DIALOG;
		} else if (id == "player") {
			retval = PLAYER;
		} else if (id == "edit_theme") {
			retval = EDIT_THEME;
		} else if (id == "language") {
			retval = CHANGE_LANGUAGE;
		} else if (id == "chat") {
			retval = MESSAGE;
		} else if (id == "preferences") {
			retval = EDIT_PREFERENCES;
		} else if (id == "design") {
			retval = DESIGN;
		}

		connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, id, false)
			, boost::bind(
				&trose::set_retval
				, this
				, boost::ref(window)
				, retval));

		if (retval == PLAYER) {
			find_widget<tbutton>(&window, id, false).set_visible(twidget::INVISIBLE);
		}
	}

	tlobby::thandler::join();

	refresh_explorer_tree(window);
	pre_base(window);
}

void trose::refresh_explorer_tree(twindow& window)
{
	cookies_.clear();

	ttree_view* explorer = find_widget<ttree_view>(&window, "explorer", false, true);
	ttree_view_node& root = explorer->get_root_node();
	root.clear();
	VALIDATE(explorer->empty(), null_str);

	ttree_view_node& htvi = add_explorer_node(game_config::path, root, file_name(game_config::path), true);
	::walk_dir(editor_.working_dir(), false, boost::bind(
				&trose::walk_dir2
				, this
				, _1, _2, &htvi));
	htvi.sort_children(boost::bind(&trose::compare_sort, this, _1, _2));
	htvi.unfold();
    explorer->invalidate_layout(true);
}

void trose::pre_base(twindow& window)
{
	tbuild::pre_show(find_widget<ttrack>(&window, "task_status", false));

	connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "browse", false)
			, boost::bind(
				&trose::set_working_dir
				, this
				, boost::ref(window)));
	find_widget<tbutton>(&window, "browse", false).set_active(false);

	connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "refresh", false)
			, boost::bind(
				&trose::do_refresh
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "build", false)
			, boost::bind(
				&trose::do_normal_build
				, this
				, boost::ref(window)));

	reload_mod_configs(disp_);
	fill_items(window);
}

ttree_view_node& trose::add_explorer_node(const std::string& dir, ttree_view_node& parent, const std::string& name, bool isdir)
{
	string_map tree_group_field;
	std::map<std::string, string_map> tree_group_item;

	tree_group_field["label"] = name;
	tree_group_item["text"] = tree_group_field;
	tree_group_field["label"] = isdir? "misc/dir.png": "misc/file.png";
	tree_group_item["type"] = tree_group_field;
	ttree_view_node& htvi = parent.add_child("node", tree_group_item, twidget::npos, isdir);
	htvi.icon()->set_label("fold-common");

	cookies_.insert(std::make_pair(&htvi, tcookie(dir, name, isdir)));
	if (isdir) {
		htvi.icon()->set_callback_state_change(boost::bind(&trose::icon_toggled, this, _1));
	}
	htvi.connect_signal<event::RIGHT_BUTTON_CLICK>(boost::bind(
		&trose::right_click_explorer, this, boost::ref(htvi), _3, _4));
	htvi.connect_signal<event::RIGHT_BUTTON_CLICK>(boost::bind(
		&trose::right_click_explorer, this, boost::ref(htvi), _3, _4), event::tdispatcher::back_post_child);

	return htvi;
}

bool trose::compare_sort(const ttree_view_node& a, const ttree_view_node& b)
{
	const tcookie& a2 = cookies_.find(&a)->second;
	const tcookie& b2 = cookies_.find(&b)->second;

	if (a2.isdir && !b2.isdir) {
		// tvi1 is directory, tvi2 is file
		return true;
	} else if (!a2.isdir && b2.isdir) {
		// tvi1 is file, tvi2 if directory
		return false;
	} else {
		// both lvi1 and lvi2 are directory or file, use compare string.
		return SDL_strcasecmp(a2.name.c_str(), b2.name.c_str()) <= 0? true: false;
	}
}

bool trose::walk_dir2(const std::string& dir, const SDL_dirent2* dirent, ttree_view_node* root)
{
	bool isdir = SDL_DIRENT_DIR(dirent->mode);
	add_explorer_node(dir, *root, dirent->name, isdir);

	return true;
}

void trose::icon_toggled(twidget* widget)
{
	ttoggle_button* toggle = dynamic_cast<ttoggle_button*>(widget);
	ttree_view_node& node = ttree_view_node::node_from_icon(*toggle);
	if (!toggle->get_value() && node.empty()) {
		const tcookie& cookie = cookies_.find(&node)->second;
		::walk_dir(cookie.dir + '/' + cookie.name, false, boost::bind(
				&trose::walk_dir2
				, this
				, _1, _2, &node));
		node.sort_children(boost::bind(&trose::compare_sort, this, _1, _2));
	}
}

bool trose::handle(int tag, tsock::ttype type, const config& data)
{
	if (tag != tlobby::tag_chat) {
		return false;
	}

	if (type != tsock::t_data) {
		return false;
	}
	if (const config& c = data.child("whisper")) {
		tbutton* b = find_widget<tbutton>(window_, "message", false, false);
		if (b->label().empty()) {
			b->set_label("misc/red-dot12.png");
		}
		sound::play_UI_sound(game_config::sounds::receive_message);
	}
	return false;
}

void trose::set_retval(twindow& window, int retval)
{
	if (is_building()) {
		return;
	}
	window.set_retval(retval);
}

void trose::fill_items(twindow& window)
{
	if (!file_exists(editor_.working_dir() + "/data/core/_main.cfg") || !is_directory(editor_.working_dir() + "/xwml")) {
		return;
	}

	editor_.reload_extendable_cfg();

	std::vector<editor::BIN_TYPE> system_bins;
	for (editor::BIN_TYPE type = editor::BIN_MIN; type <= editor::BIN_SYSTEM_MAX; type = (editor::BIN_TYPE)(type + 1)) {
		system_bins.push_back(type);
	}
	editor_.get_wml2bin_desc_from_wml(system_bins);
	const std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();

	bool enable_build = false;
	std::stringstream ss;
	tlistbox* list = find_widget<tlistbox>(&window, "items", false, true);
	list->set_row_align(false);
	list->clear();
	for (std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >::const_iterator it = descs.begin(); it != descs.end(); ++ it) {
		const editor::wml2bin_desc& desc = it->second;

		string_map list_item;
		std::map<std::string, string_map> list_item_item;

		ss.str("");
		if (desc.wml_nfiles == desc.bin_nfiles && desc.wml_sum_size == desc.bin_sum_size && desc.wml_modified == desc.bin_modified) {
			ss << tintegrate::generate_img("misc/ok-tip.png");
		} else {
			ss << tintegrate::generate_img("misc/alert-tip.png");
		}
		ss << desc.bin_name;
		list_item["label"] = ss.str();
		list_item_item.insert(std::make_pair("filename", list_item));

		list_item["label"] = desc.app;
		list_item_item.insert(std::make_pair("app", list_item));

		ss.str("");
		ss << "(" << desc.wml_nfiles << ", " << desc.wml_sum_size << ", " << desc.wml_modified << ")";
		list_item["label"] = ss.str();
		list_item_item.insert(std::make_pair("wml_checksum", list_item));

		ss.str("");
		ss << "(" << desc.bin_nfiles << ", " << desc.bin_sum_size << ", " << desc.bin_modified << ")";
		list_item["label"] = ss.str();
		list_item_item.insert(std::make_pair("bin_checksum", list_item));

		list->add_row(list_item_item);

		twidget* panel = list->get_row_panel(list->get_item_count() - 1);
		ttoggle_button* prefix = find_widget<ttoggle_button>(panel, "prefix", false, true);
		if (build_msg_data_.type == build_export || build_msg_data_.type == build_ios_kit) {
			enable_build = true;

		} else if (it->first == editor::MAIN_DATA && (desc.wml_nfiles != desc.bin_nfiles || desc.wml_sum_size != desc.bin_sum_size || desc.wml_modified != desc.bin_modified)) {	
			enable_build = true;

		} else if (it->first == editor::SCENARIO_DATA && (desc.wml_nfiles != desc.bin_nfiles || desc.wml_sum_size != desc.bin_sum_size || desc.wml_modified != desc.bin_modified)) {
			const std::string id = file_main_name(desc.bin_name);
			if (editor_config::campaign_id.empty() || id == editor_config::campaign_id) {
				enable_build = true;
			}
		}
		if (enable_build) {
			prefix->set_value(true);
		}
		prefix->set_callback_state_change(boost::bind(&trose::check_build_toggled, this, _1));
	}

	list->invalidate_layout(true);

	find_widget<tbutton>(&window, "build", false).set_active(enable_build);
}

void trose::set_build_active(twindow& window)
{
	tlistbox* list = find_widget<tlistbox>(&window, "items", false, true);
	int count = list->get_item_count();
	for (int at = 0; at < count; at ++) {
		twidget* panel = list->get_row_panel(at);
		ttoggle_button* prefix = find_widget<ttoggle_button>(panel, "prefix", false, true);
		if (prefix->get_value()) {
			find_widget<tbutton>(&window, "build", false).set_active(true);
			return;
		}
	}
	find_widget<tbutton>(&window, "build", false).set_active(false);
}

void trose::check_build_toggled(twidget* widget)
{
	set_build_active(*window_);
}

void trose::export_app(tapp_copier& copier)
{
	std::stringstream ss;
	utils::string_map symbols;

	symbols["app"] = copier.app;
	symbols["dst"] = copier.exporter->alias_2_path(ttask::app_src2_alias);
	ss.str("");
	ss << vgettext2("Do you want to export $app package to $dst?", symbols); 

	int res = gui2::show_message(disp_.video(), "", ss.str(), gui2::tmessage::yes_no_buttons);
	if (res != gui2::twindow::OK) {
		return;
	}

	absolute_draw();
	bool fok = copier.exporter->handle(disp_);
	if (fok && is_studio_app(copier.app)) {
		fok = copier.studio_extra_exporter->handle(disp_);
	}
	if (!fok) {
		symbols["src"] = copier.exporter->alias_2_path(ttask::src2_alias);
		symbols["result"] = fok? _("Success"): _("Fail");
		ss.str("");
		ss << vgettext2("Export $app package from \"$src\" to \"$dst\", $result!", symbols);
		gui2::show_message(disp_.video(), null_str, ss.str());
		return;
	}

	generate_app_cfg(copier.exporter->alias_2_path(ttask::app_res_alias), copier.app);

	current_copier_ = &copier;
	{
		// build
		build_msg_data_.set(build_export, copier.app);
		on_change_working_dir(*window_, copier.exporter->alias_2_path(ttask::app_res_alias));
		do_build(*window_);
	}
}

bool trose::copy_android_res(const tapp_copier& copier, bool silent)
{
	std::stringstream ss;
	utils::string_map symbols;

	bool fok = copier.android_res_copier->handle(disp_);
	if (!silent) {
		symbols["app"] = copier.app;
		symbols["src"] = copier.android_res_copier->alias_2_path(ttask::app_res_alias);
		symbols["dst"] = copier.android_res_copier->alias_2_path(tapp_copier::app_android_prj_alias);
		symbols["result"] = fok? _("Success"): _("Fail");
		ss.str("");
		ss << vgettext2("Copy $app|'s resource from \"$src\" to \"$dst\", $result!", symbols);
		gui2::show_message(disp_.video(), null_str, ss.str());
	}
	return fok;
}

bool trose::export_ios_kit(const tapp_copier& copier)
{
	std::stringstream ss;
	utils::string_map symbols;

	bool fok = copier.ios_kiter->handle(disp_);
	if (!fok) {
		symbols["dst"] = copier.ios_kiter->alias_2_path(tios_kit::kit_alias);
		symbols["result"] = fok? _("Success"): _("Fail");
		ss.str("");
		ss << vgettext2("Create iOS kit on \"$dst\", $result!", symbols);
		gui2::show_message(disp_.video(), null_str, ss.str());
	}
	generate_app_cfg(copier.ios_kiter->alias_2_path(tios_kit::studio_alias), copier.app);

	current_copier_ = &copier;
	{
		// build
		build_msg_data_.set(build_ios_kit, copier.app);
		on_change_working_dir(*window_, copier.ios_kiter->alias_2_path(tios_kit::studio_alias));
		do_build(*window_);
	}

	return fok;
}

void trose::on_change_working_dir(twindow& window, const std::string& dir)
{
	editor_.set_working_dir(dir);
	fill_items(window);

	task_status_->set_dirty();
	require_set_task_bar_ = true;
}

void trose::build_on_app_changed(const std::string& app, twindow& window, bool remove)
{
	// update three-app.cfg
	std::set<std::string> apps;
	for (std::vector<std::unique_ptr<tapp_copier> >::const_iterator it = app_copiers.begin(); it != app_copiers.end(); ++ it) {
		const tapp_copier& copier = **it;
		if (!remove || copier.app != app) {
			apps.insert(copier.app);
		}
	}
	if (!remove) {
		apps.insert(app);
	}
	validater_res(apps);

	build_msg_data_.set(remove? build_remove: build_new, app);

	fill_items(window);
	do_build(window);
}

void trose::generate_2_cfg() const
{
	std::stringstream fp_ss;

	fp_ss << "#\n";
	fp_ss << "# NOTE: it is generated by rose studio, don't edit yourself.\n";
	fp_ss << "#\n";
	fp_ss << "\n";

	for (std::vector<std::unique_ptr<tapp_copier> >::const_iterator it = app_copiers.begin(); it != app_copiers.end(); ++ it) {
		const tapp_copier& app = **it;
		if (is_private_app(app.app)) {
			continue;
		}
		if (!fp_ss.str().empty()) {
			fp_ss << "\n";
		}
		app.generate(fp_ss, null_str);
	}

	const std::string file_name = game_config::absolute_path + "/apps.cfg";
	posix_file_t fp = INVALID_FILE;
	posix_fopen(file_name.c_str(), GENERIC_WRITE, CREATE_ALWAYS, fp);
	VALIDATE(fp != INVALID_FILE, null_str);

	posix_fwrite(fp, fp_ss.str().c_str(), fp_ss.str().length());
	posix_fclose(fp);
}

void trose::right_click_explorer(ttree_view_node& node, bool& handled, bool& halt)
{
	if (is_building()) {
		return;
	}
	if (!node.parent_node().is_root_node()) {
		return;
	}
	if (app_copiers.empty()) {
		return;
	}
	if (!is_apps_kit()) {
		return;
	}
	if (game_config::apps_src_path.empty()) {
		return;
	}

	std::stringstream ss;
	utils::string_map symbols;
	std::vector<gui2::tmenu::titem> items, sub_items;
	
	const int app_wight = 100;
	int at = 0, app_index = 1;
	const int new_app_at = at ++;
	items.push_back(gui2::tmenu::titem(new_app_at, std::string(_("New app")) + "..."));

	for (std::vector<std::unique_ptr<tapp_copier> >::const_iterator it = app_copiers.begin(); it != app_copiers.end(); ++ it) {
		const tapp_copier& app = **it;
		symbols["app"] = app.app;
		sub_items.clear();
		int at2 = 0;
		sub_items.push_back(gui2::tmenu::titem(app_index * app_wight + at2 ++, vgettext2("Export", symbols) + "..."));
		sub_items.push_back(gui2::tmenu::titem(app_index * app_wight + at2 ++, vgettext2("Edit capabilities", symbols) + "..."));
		sub_items.push_back(gui2::tmenu::titem(app_index * app_wight + at2 ++, vgettext2("Copy resource to android's apk", symbols) + "..."));
		if (!is_reserve_app(app.app)) {
			sub_items.push_back(gui2::tmenu::titem(app_index * app_wight + at2 ++, vgettext2("Remove", symbols) + "..."));
		} else {
			at2 ++;
		}

		items.push_back(gui2::tmenu::titem(twidget::npos, app.app, sub_items));
		app_index ++;
	}

	const int tools_index = app_index;
	{
		sub_items.clear();
		int at2 = 0;
		sub_items.push_back(gui2::tmenu::titem(tools_index * app_wight + at2 ++, vgettext2("Create iOS kit", symbols) + "..."));

		items.push_back(gui2::tmenu::titem(twidget::npos, _("Tools"), sub_items));
		app_index ++;
	}

	int x, y;
	SDL_GetMouseState(&x, &y);
	int selected;
	{
		gui2::tmenu dlg(disp_, items, twidget::npos);
		dlg.show(disp_.video(), 0, x, y);
		int retval = dlg.get_retval();
		if (dlg.get_retval() != gui2::twindow::OK) {
			return;
		}
		// absolute_draw();
		selected = dlg.selected_val();
	}
	if (selected == new_app_at) {
		tapp_capabilities capabilities(null_str, null_str);
		{
			gui2::tcapabilities dlg(disp_, app_copiers, twidget::npos);
			dlg.show(disp_.video());
			if (dlg.get_retval() != gui2::twindow::OK) {
				return;
			}
			capabilities = dlg.get_capabilities();
		}
		if (do_new_app(disp_, capabilities, *(newer.get()))) {
			build_on_app_changed(capabilities.app, *window_, false);
		}

	} else if (selected >= app_wight && selected < ((int)app_copiers.size() + 1) * app_wight) {
		const int app_at = (selected / app_wight) - 1;
		tapp_copier& current_app = *(app_copiers[app_at].get());
		if (selected % app_wight == 0) {
			// export
			export_app(current_app);

		} else if (selected % app_wight == 1) {
			// edit capabilities
			gui2::tcapabilities dlg(disp_, app_copiers, app_at);
			dlg.show(disp_.video());
			if (dlg.get_retval() != gui2::twindow::OK) {
				return;
			}
			VALIDATE(!is_private_app(current_app.app), null_str);

			current_app.reset(dlg.get_capabilities());
			generate_2_cfg();

		} else if (selected % app_wight == 2) {
			// copy resource to android's apk
			symbols["app"] = current_app.app;
			ss.str("");
			ss << vgettext2("Do you want to copy $app|'s resource to .apk?", symbols); 

			int res = gui2::show_message(disp_.video(), "", ss.str(), gui2::tmessage::yes_no_buttons);
			if (res != gui2::twindow::OK) {
				return;
			}
			absolute_draw();

			copy_android_res(current_app, false);

		} else {
			if (is_reserve_app(current_app.app)) {
				return;
			}
			symbols["app"] = current_app.app;
			ss.str("");
			ss << vgettext2("Do you want to remove $app from work kit?", symbols); 
			int res = gui2::show_message(disp_.video(), "", ss.str(), gui2::tmessage::yes_no_buttons);
			if (res != gui2::twindow::OK) {
				return;
			}

			// maybe run Visual Studio, so remove app from apps.sln first.
			apps_sln::remove_project(current_app.app);
			{
				remover->set_app(current_app.app);
				remover->handle(disp_);
			}

			build_on_app_changed(current_app.app, *window_, true);
		}
	} else if (selected >= tools_index * app_wight && selected < (tools_index + 1) * app_wight) {
		if (selected % app_wight == 0) {
			// Create iOS disk
			const tapp_copier* studio_copier = NULL;
			for (std::vector<std::unique_ptr<tapp_copier> >::const_iterator it = app_copiers.begin(); it != app_copiers.end(); ++ it) {
				const tapp_copier& copier = **it;
				if (copier.app == "studio") {
					studio_copier = &copier;
				}
			}

			ss.str("");
			symbols["dst"] = studio_copier->ios_kiter->alias_2_path(tios_kit::kit_alias);
			ss << vgettext2("Do you want to create iOS kit on $dst?", symbols); 
			int res = gui2::show_message(disp_.video(), "", ss.str(), gui2::tmessage::yes_no_buttons);
			if (res != gui2::twindow::OK) {
				return;
			}

			absolute_draw();
			export_ios_kit(*studio_copier);
		}
	}

	handled = halt = true;
}

void trose::set_working_dir(twindow& window)
{
	std::string desire_dir;
	{
		gui2::tbrowse::tparam param(gui2::tbrowse::TYPE_DIR, true, null_str, _("Choose a Working Directory to Build"));
		gui2::tbrowse dlg(disp_, param);
		dlg.show(disp_.video());
		int res = dlg.get_retval();
		if (res != gui2::twindow::OK) {
			return;
		}
		desire_dir = param.result;
	}
	if (desire_dir == editor_.working_dir()) {
		return;
	}
	if (!check_res_folder(desire_dir)) {
		std::stringstream err;
		err << desire_dir << " isn't valid res directory";
		gui2::show_message(disp_.video(), null_str, err.str());
		return;
	}

	on_change_working_dir(window, desire_dir);
}

void trose::do_refresh(twindow& window)
{
	fill_items(window);
}

void trose::do_normal_build(twindow& window)
{
	build_msg_data_.set(build_normal, null_str);
	do_build(window);
}

void trose::do_build(twindow& window)
{
	do_build2();
}

void trose::app_work_start()
{
	twindow& window = *window_;
	tlistbox& list = find_widget<tlistbox>(&window, "items", false);

	find_widget<tbutton>(&window, "refresh", false).set_active(false);
	find_widget<tbutton>(&window, "build", false).set_active(false);
	// find_widget<tbutton>(&window, "browse", false).set_active(false);
	list.set_active(false);

	std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();
	int count = list.get_item_count();
	for (int at = 0; at < count; at ++) {
		twidget* panel = list.get_row_panel(at);
		ttoggle_button* prefix = find_widget<ttoggle_button>(panel, "prefix", false, true);
		descs[at].second.require_build = prefix->get_value();

		find_widget<tcontrol>(panel, "status", false).set_label(null_str);

		// prefix->set_value(false);
	}
}

void trose::app_work_done()
{
	twindow& window = *window_;
	tbutton& refresh = find_widget<tbutton>(&window, "refresh", false);
	tbutton& build = find_widget<tbutton>(&window, "build", false);
	tlistbox& list = find_widget<tlistbox>(&window, "items", false);
	// find_widget<tbutton>(&window, "browse", false).set_active(true);

	int count = list.get_item_count();
	for (int at = 0; at < count; at ++) {
		twidget* panel = list.get_row_panel(at);
		ttoggle_button* prefix = find_widget<ttoggle_button>(panel, "prefix", false, true);
		// prefix->set_value(true);
	}

	list.set_active(true);
	refresh.set_active(true);
	build.set_active(true);
	require_set_task_bar_ = true;

	main_->Post(RTC_FROM_HERE, this, MSG_BUILD_FINISHED, NULL);
}

void trose::OnMessage(rtc::Message* msg)
{
	const int build_type = build_msg_data_.type;
	const std::string app = build_msg_data_.app;
	twindow& window = *window_;

	build_msg_data_.set(build_normal, null_str);

	switch (msg->message_id) {
	case MSG_BUILD_FINISHED:
		if (build_type == build_export) {
			// copy res to android/res
			copy_android_res(*current_copier_, true);

			std::stringstream ss;
			utils::string_map symbols;

			symbols["src"] = current_copier_->exporter->alias_2_path(ttask::src2_alias);
			symbols["dst"] = current_copier_->exporter->alias_2_path(ttask::app_src2_alias);
			symbols["result"] = _("Success");
			ss.str("");
			ss << vgettext2("Export $app package from \"$src\" to \"$dst\", $result!", symbols); 
			gui2::show_message(disp_.video(), null_str, ss.str());

		} else if (build_type == build_new) {
			reload_mod_configs(disp_);
			refresh_explorer_tree(*window_);

			std::stringstream ss;
			utils::string_map symbols;

			symbols["app"] = app;
			symbols["result"] = _("Success");
			ss.str("");
			ss << vgettext2("New $app, $result!", symbols) << "\n\n";
			ss << _("If you are runing Visual Studio and opening apps.sln, please execute \"Close Solution\", then open again.");
			gui2::show_message(disp_.video(), null_str, ss.str());

		} else if (build_type == build_remove) {
			for (std::vector<std::unique_ptr<tapp_copier> >::iterator it = app_copiers.begin(); it != app_copiers.end(); ++ it) {
				const tapp_copier& copier = **it;
				if (copier.app == app) {
					app_copiers.erase(it);
					break;
				}
			}
			generate_2_cfg();
			reload_mod_configs(disp_);
			refresh_explorer_tree(*window_);

			std::stringstream ss;
			utils::string_map symbols;

			symbols["app"] = app;
			symbols["result"] = _("Success");
			ss.str("");
			ss << vgettext2("Remove $app, $result!", symbols) << "\n\n";
			ss << _("If you are runing Visual Studio and opening apps.sln, please execute \"Close Solution\", then open again.");
			gui2::show_message(disp_.video(), null_str, ss.str());

		} else if (build_type == build_ios_kit) {
			std::stringstream ss;
			utils::string_map symbols;

			symbols["dst"] = current_copier_->ios_kiter->alias_2_path(tios_kit::kit_alias);
			symbols["result"] = _("Success");
			ss.str("");
			ss << vgettext2("Create iOS kit on \"$dst\", $result!", symbols);
			gui2::show_message(disp_.video(), null_str, ss.str());

		} 
		break;
	}

	if (build_type == build_export || build_type == build_ios_kit) {
		on_change_working_dir(window, game_config::path);
	} else {
		fill_items(window);
	}
}

void trose::app_handle_desc(const bool started, const int at, const bool ret)
{
	tlistbox& list = find_widget<tlistbox>(window_, "items", false);
	std::stringstream ss;
	std::string str;
	std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();

		editor::wml2bin_desc& desc = descs[at].second;
		twidget* panel = list.get_row_panel(at);

		if (!started) {
			tcontrol* filename = find_widget<tcontrol>(panel, "filename", false, true);
			ss.str("");
			ss << tintegrate::generate_img(ret? "misc/ok-tip.png": "misc/alert-tip.png");
			ss << desc.bin_name;
			filename->set_label(ss.str());

			tcontrol* wml_checksum = find_widget<tcontrol>(panel, "wml_checksum", false, true);
			ss.str("");
			ss << "(" << desc.wml_nfiles << ", " << desc.wml_sum_size << ", " << desc.wml_modified << ")";
			wml_checksum->set_label(ss.str());

			tcontrol* bin_checksum = find_widget<tcontrol>(panel, "bin_checksum", false, true);
			desc.refresh_checksum(editor_.working_dir());
			ss.str("");
			ss << "(" << desc.bin_nfiles << ", " << desc.bin_sum_size << ", " << desc.bin_modified << ")";
			bin_checksum->set_label(ss.str());
		}
		tcontrol* status = find_widget<tcontrol>(panel, "status", false, true);
		if (started) {
			str = "misc/operating.png";
		} else {
			str = ret? "misc/success.png": "misc/fail.png";
		}
		status->set_label(str);

	list.invalidate_layout(true);
}

void trose::generate_gui_app_main_cfg(const std::string& res_path, const std::set<std::string>& apps) const
{
	// if necessary, generate <apps-res>/data/gui/app/app-xxx/_main.cfg
	std::stringstream ss, fp_ss;

	for (std::set<std::string>::const_iterator it = apps.begin(); it != apps.end(); ++ it) {
		const std::string& app = *it;
		fp_ss.str("");
		fp_ss << "#\n";
		fp_ss << "# NOTE: it is generated by rose studio, don't edit yourself.\n";
		fp_ss << "#\n";
		fp_ss << "\n";

		// {gui/app-xxx/widget/}
		// {gui/app-xxx/window/}
		// {gui/app-xxx/theme/}
		fp_ss << "{gui/" << game_config::generate_app_dir(app) << "/widget/}\n";
		fp_ss << "{gui/" << game_config::generate_app_dir(app) << "/window/}\n";
		fp_ss << "{gui/" << game_config::generate_app_dir(app) << "/theme/}";

		ss.str("");
		ss << res_path << "/data/gui/";
		ss << game_config::generate_app_dir(app);
		ss << "/_main.cfg";
		if (file_exists(ss.str())) {
			tfile file2(ss.str(), GENERIC_READ, OPEN_EXISTING);
			int fsize = file2.read_2_data();
			if (fsize == fp_ss.str().size() && !memcmp(fp_ss.str().c_str(), file2.data, fsize)) {
				continue;
			}
		}

		tfile fp(ss.str(), GENERIC_WRITE, CREATE_ALWAYS);
		if (!fp.valid()) {
			return;
		}
		posix_fwrite(fp.fp, fp_ss.str().c_str(), fp_ss.str().length());
	}
}

void trose::generate_app_cfg(const std::string& res_path, const std::set<std::string>& apps) const
{
	std::string base_dir, app_dir;
	std::stringstream fp_ss;
	
	enum {app_cfg_data, app_cfg_core, app_cfg_gui, app_count};

	for (int type = app_cfg_data; type < app_count; type ++) {
		fp_ss.str("");
		fp_ss << "#\n";
		fp_ss << "# NOTE: it is generated by rose studio, don't edit yourself.\n";
		fp_ss << "#\n";
		fp_ss << "\n";
		for (std::set<std::string>::const_iterator it = apps.begin(); it != apps.end(); ++ it) {
			const std::string& app = *it;
			const std::string app_short_dir = std::string("app-") + app;
			if (it != apps.begin()) {
				fp_ss << "\n";
			}
			fp_ss << "{";
			if (type == app_cfg_data) {
				base_dir = res_path + "/data";
				fp_ss << app_short_dir << "/_main.cfg";

			} else if (type == app_cfg_core) {
				base_dir = res_path + "/data/core";
				fp_ss << "core/" << app_short_dir << "/_main.cfg";

			} else if (type == app_cfg_gui) {
				base_dir = res_path + "/data/gui";
				fp_ss << "gui/" << app_short_dir << "/_main.cfg";

			} else {
				VALIDATE(false, null_str);
			}
			fp_ss << "}";
		}

		// if _main.cfg doesn't exist, create a empty app.cfg. it is necessary for arthiture.
		std::string file = base_dir + "/app.cfg";

		if (file_exists(file)) {
			tfile file2(file, GENERIC_READ, OPEN_EXISTING);
			int fsize = file2.read_2_data();
			if (fsize == fp_ss.str().size() && !memcmp(fp_ss.str().c_str(), file2.data, fsize)) {
				continue;
			}
		}

		tfile fp(file, GENERIC_WRITE, CREATE_ALWAYS);
		posix_fwrite(fp.fp, fp_ss.str().c_str(), fp_ss.str().length());
	}
}

void trose::reload_mod_configs(display& disp)
{
	generate_cfg.clear();
	app_copiers.clear();
	tdomains.clear();

	if (check_res_folder(game_config::path)) {
		game_config::config_cache_transaction transaction;
		game_config::config_cache& cache = game_config::config_cache::instance();
		cache.clear_defines();

		cache.get_config(game_config::absolute_path + "/generate.cfg", generate_cfg);
		config temp;
		cache.get_config(game_config::absolute_path + "/apps.cfg", temp);
		generate_cfg.append(temp);
	}

	if (generate_cfg.empty()) {
		// on iOS/Android, there is no generate.cfg/apps.cfg.
		return;
	}

	const config* export_cfg = NULL, *studio_extra_export_cfg = NULL, *android_res_cfg = NULL, *ios_kit_cfg = NULL;
	BOOST_FOREACH (const config& c, generate_cfg.child_range("generate")) {
		const std::string& type = c["type"].str();
		if (type == "app") {
			app_copiers.push_back(std::unique_ptr<tapp_copier>(new tapp_copier(c)));

		} else if (type == "new_app") {
			newer = ttask::create_task<tnewer>(c, "new", NULL);

		} else if (type == "remove_app") {
			remover = ttask::create_task<tremover>(c, "remove", NULL);

		} else if (type == "validate_res") {
			validater = ttask::create_task<tvalidater>(c, "validate", NULL);

		} else if (type == "export") {
			export_cfg = &c;

		} else if (type == "studio_extra_export") {
			studio_extra_export_cfg = &c;

		} else if (type == "android_res") {
			android_res_cfg = &c;

		} else if (type == "ios_kit") {
			ios_kit_cfg = &c;
		}
	}

	VALIDATE(export_cfg && studio_extra_export_cfg && android_res_cfg, null_str);

	std::set<std::string> apps;
	tdomains.insert(std::make_pair("rose-lib", null_str));
	tdomains.insert(std::make_pair("editor-lib", null_str));

	for (std::vector<std::unique_ptr<tapp_copier> >::const_iterator it = app_copiers.begin(); it != app_copiers.end(); ++ it) {
		tapp_copier& copier = **it;

		copier.exporter = ttask::create_task<texporter>(*export_cfg, "export", &copier);
		copier.android_res_copier = ttask::create_task<tandroid_res>(*android_res_cfg, "android_res", &copier);
		if (is_studio_app(copier.app)) {
			copier.studio_extra_exporter = ttask::create_task<tstudio_extra_exporter>(*studio_extra_export_cfg, "export", &copier);
			copier.ios_kiter = ttask::create_task<tios_kit>(*ios_kit_cfg, "export", &copier);
		}

		for (std::set<std::string>::const_iterator it = copier.tdomains.begin(); it != copier.tdomains.end(); ++ it) {
			tdomains.insert(std::make_pair(*it, copier.app));
		}
		apps.insert(copier.app);
	}

	VALIDATE(validater.get(), "Must define validate_res.");

	validater_res(apps);
}

void trose::validater_res(const std::set<std::string>& apps)
{
	if (!is_apps_kit()) {
		return;
	}

	for (std::set<std::string>::const_iterator it = apps.begin(); it != apps.end(); ++ it) {
		const std::string& app = *it;
		validater->set_app(app);
		validater->handle(disp_);
	}
	generate_gui_app_main_cfg(game_config::path, apps);
	generate_app_cfg(game_config::path, apps);
}

} // namespace gui2

