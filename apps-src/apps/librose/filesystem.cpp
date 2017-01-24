/* $Id: filesystem.cpp 47496 2010-11-07 15:53:11Z silene $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * @file
 * File-IO
 */
#define GETTEXT_DOMAIN "rose-lib"

#include "global.hpp"

// Include files for opendir(3), readdir(3), etc.
// These files may vary from platform to platform,
// since these functions are NOT ANSI-conforming functions.
// They may have to be altered to port to new platforms

//for mkdir
#include <sys/stat.h>

#ifdef _WIN32
#include <shlobj.h>	// CSIDL_PROGRAM_FILES
#include <direct.h>
#include <cctype>
#else /* !_WIN32 */
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#ifndef ANDROID
#include <sys/param.h> // statfs 
#include <sys/mount.h> // statfs
#else
#include <sys/vfs.h> // statfs 
#endif
#endif /* !_WIN32 */

// for getenv
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <set>
#include <boost/algorithm/string.hpp>

// for strerror
#include <cstring>

#include "config.hpp"
#include "filesystem.hpp"
#include "rose_config.hpp"
#include "log.hpp"
#include "loadscreen.hpp"
#include "scoped_resource.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "posix2.h"
#include "saes.hpp"
#include "version.hpp"
#include "wml_exception.hpp"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

static lg::log_domain log_filesystem("filesystem");
#define DBG_FS LOG_STREAM(debug, log_filesystem)
#define LOG_FS LOG_STREAM(info, log_filesystem)
#define WRN_FS LOG_STREAM(warn, log_filesystem)
#define ERR_FS LOG_STREAM(err, log_filesystem)

namespace {
	// const mode_t AccessMode = 00770;

	// These are the filenames that get special processing
	const std::string maincfg_filename = "_main.cfg";
	const std::string finalcfg_filename = "_final.cfg";
	const std::string initialcfg_filename = "_initial.cfg";

}

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBase.h>
#endif

static std::set<std::string> predef_dirs;
static std::map<int, std::set<std::string> > predef_files;

bool ends_with(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() && std::equal(suffix.begin(),suffix.end(),str.end()-suffix.size());
}

void get_files_in_dir(const std::string &directory,
					  std::vector<std::string>* files,
					  std::vector<std::string>* dirs,
					  file_name_option mode,
					  int filter,
					  file_reorder_option reorder,
					  file_tree_checksum* checksum)
{
	// If we have a path to find directories in,
	// then convert relative pathnames to be rooted
	// on the wesnoth path
	if (!SDL_IsRootPath(directory.c_str())) {
		std::string dir = game_config::path + "/" + directory;
		if (is_directory(dir)) {
			get_files_in_dir(dir,files,dirs,mode,filter,reorder,checksum);
		}
		return;
	}

	if (reorder == DO_REORDER) {
		std::string maincfg;
		if (directory.empty() || directory[directory.size()-1] == '/') {
			maincfg = directory + maincfg_filename;
		} else {
			maincfg = (directory + "/") + maincfg_filename;
		}

		if (file_exists(maincfg)) {
			if (files != NULL) {
				if (mode == ENTIRE_FILE_PATH) {
					files->push_back(maincfg);
				} else {
					files->push_back(maincfg_filename);
				}
			}
			return;
		}
	}

	SDL_DIR* dir = SDL_OpenDir(directory.c_str());
	if (dir == NULL) {
		return;
	}

	SDL_dirent2* entry;
	while ((entry = SDL_ReadDir(dir)) != NULL) {
		if (entry->name[0] == '.') {
			continue;
		}

		// generic Unix
		const std::string basename = entry->name;

		std::string fullname;
		if (directory.empty() || directory[directory.size()-1] == '/') {
			fullname = directory + basename;
		} else {
			fullname = directory + "/" + basename;
		}

		if (SDL_DIRENT_DIR(entry->mode)) {
			if ((filter & SKIP_MEDIA_DIR) && (basename == "images"|| basename == "sounds" || basename == "music")) {
				continue;
			}
			if ((filter & SKIP_SCENARIO_DIR) && (basename == "scenarios"|| basename == "maps" || basename == "music")) {
				continue;
			}
			if ((filter & SKIP_GUI_DIR) && basename == "gui") {
				continue;
			}
			if ((filter & SKIP_INTERNAL_DIR) && basename == "units-internal") {
				continue;
			}
			if ((filter & SKIP_BOOK) && basename == "book") {
				continue;
			}

			if (reorder == DO_REORDER && file_exists(fullname + "/" + maincfg_filename)) {
				if (files != NULL) {
					if (mode == ENTIRE_FILE_PATH) {
						files->push_back(fullname + "/" + maincfg_filename);
					} else {
						files->push_back(basename + "/" + maincfg_filename);
					}
				} else {
					// Show what I consider strange
					LOG_FS << fullname << "/" << maincfg_filename << " not used now but skip the directory \n";
				}
			} else if (dirs != NULL) {
				if (mode == ENTIRE_FILE_PATH) {
					dirs->push_back(fullname);
				} else {
					dirs->push_back(basename);
				}
			}
		} else {
			if (files != NULL) {
				if (mode == ENTIRE_FILE_PATH)
					files->push_back(fullname);
				else
					files->push_back(basename);
			}
			if (checksum != NULL) {
				if (entry->mtime > checksum->modified) {
					checksum->modified = entry->mtime;
				}
				checksum->sum_size += entry->size;
				checksum->nfiles++;
			}
		}
	}

	SDL_CloseDir(dir);

	if (files != NULL) {
		std::sort(files->begin(),files->end());
	}

	if (dirs != NULL) {
		std::sort(dirs->begin(),dirs->end());
	}

	if (files != NULL && reorder == DO_REORDER) {
		// move finalcfg_filename, if present, to the end of the vector
		for (unsigned int i = 0; i < files->size(); i++) {
			if (ends_with((*files)[i], "/" + finalcfg_filename)) {
				files->push_back((*files)[i]);
				files->erase(files->begin()+i);
				break;
			}
		}
		// move initialcfg_filename, if present, to the beginning of the vector
		int foundit = -1;
		for (unsigned int i = 0; i < files->size(); i++)
			if (ends_with((*files)[i], "/" + initialcfg_filename)) {
				foundit = i;
				break;
			}
		if (foundit > 0) {
			std::string initialcfg = (*files)[foundit];
			for (unsigned int i = foundit; i > 0; i--)
				(*files)[i] = (*files)[i-1];
			(*files)[0] = initialcfg;
		}
	}
}

std::string get_prefs_file()
{
	return get_user_data_dir_utf8() + "/preferences";
}

std::string create_if_not(const std::string& dir_utf8) 
{
	std::string dir_ansi = dir_utf8;
#ifdef _WIN32
	conv_ansi_utf8(dir_ansi, false);
#endif
	if (get_dir(dir_ansi).empty()) {
		return "";
	}
	return dir_utf8;
}

std::string get_saves_dir()
{
	const std::string dir_utf8 = get_user_data_dir_utf8() + "/saves";
	return create_if_not(dir_utf8);
}

std::string get_addon_campaigns_dir()
{
	const std::string dir_utf8 = get_user_data_dir_utf8() + "/data/add-ons";
	return create_if_not(dir_utf8);
}

std::string get_intl_dir()
{
	return game_config::path + "/translations";
}

std::string get_screenshot_dir()
{
	const std::string dir_utf8 = get_user_data_dir_utf8() + "/screenshots";
	return create_if_not(dir_utf8);
}

std::string get_next_filename(const std::string& name, const std::string& extension)
{
	std::string next_filename;
	int counter = 0;

	do {
		std::stringstream filename;

		filename << name;
		filename.width(3);
		filename.fill('0');
		filename.setf(std::ios_base::right);
		filename << counter << extension;
		counter++;
		next_filename = filename.str();
	} while(file_exists(next_filename) && counter < 1000);
	return next_filename;
}


std::string get_dir(const std::string& dir_path)
{
	if (is_directory(dir_path)) {
		return dir_path;
	}
	if (SDL_MakeDirectory(dir_path.c_str())) {
		return dir_path;
	}
	return null_str;
}

// This deletes a directory with no hidden files and subdirectories.
// Also deletes a single file.
bool delete_directory(const std::string& path)
{
#ifdef _WIN32
	return SDL_DeleteFiles(path.c_str());
#endif

	bool ret = true;
	std::vector<std::string> files;
	std::vector<std::string> dirs;

	get_files_in_dir(path, &files, &dirs, ENTIRE_FILE_PATH);

	if(!files.empty()) {
		for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i) {
			errno = 0;
			if(remove((*i).c_str()) != 0) {
				LOG_FS << "remove(" << (*i) << "): " << strerror(errno) << "\n";
				ret = false;
			}
		}
	}

	if(!dirs.empty()) {
		for(std::vector<std::string>::const_iterator j = dirs.begin(); j != dirs.end(); ++j) {
			if(!delete_directory(*j))
				ret = false;
		}
	}

	errno = 0;
	if(remove(path.c_str()) != 0) {
		LOG_FS << "remove(" << path << "): " << strerror(errno) << "\n";
		ret = false;
	}
	return ret;
}

std::string get_cwd()
{
	char buf[1024];
	const char* const res = getcwd(buf,sizeof(buf));
	if(res != NULL) {
		std::string str(res);

#ifdef _WIN32
		std::replace(str.begin(),str.end(),'\\','/');
#endif

		return str;
	} else {
		return "";
	}
}

std::string get_exe_dir()
{
#ifndef _WIN32
	char buf[1024];
	size_t path_size = readlink("/proc/self/exe", buf, 1024);
	if(path_size == static_cast<size_t>(-1))
		return std::string();
	buf[path_size] = 0;
	return std::string(dirname(buf));
#else
	return get_cwd();
#endif
}

bool create_directory_if_missing(const std::string& dirname)
{
	if (dirname.empty()) {
		return false;
	}

	if (is_directory(dirname)) {
		return true;
	} else if (file_exists(dirname)) {
		return false;
	}

	return SDL_MakeDirectory(dirname.c_str());
}

bool create_directory_if_missing_recursive(const std::string& dirname)
{
	DBG_FS<<"creating recursive directory: "<<dirname<<'\n';
	if (is_directory(dirname) == false && dirname.empty() == false)
	{
		std::string tmp_dirname = dirname;
		// remove trailing slashes or backslashes
		while ((tmp_dirname[tmp_dirname.size()-1] == '/' ||
			  tmp_dirname[tmp_dirname.size()-1] == '\\') &&
			  tmp_dirname.size()>0)
		{
			tmp_dirname.erase(tmp_dirname.size()-1);
		}

		// create the first non-existing directory
		size_t pos = tmp_dirname.rfind("/");

		// we get the most right directory and *skip* it
		// we are creating it when we get back here
		if (tmp_dirname.rfind('\\') != std::string::npos &&
			tmp_dirname.rfind('\\') > pos )
			pos = tmp_dirname.rfind('\\');

		if (pos != std::string::npos)
			create_directory_if_missing_recursive(tmp_dirname.substr(0,pos));

		return create_directory_if_missing(tmp_dirname);
	}
	return create_directory_if_missing(dirname);
}

void preprocess_res_explorer()
{
	Uint32 start = SDL_GetTicks();

	// copy font directory from res/fonts to user_data_dir/fonts
	std::vector<std::string> v;
	v.push_back("Andagii.ttf");
	v.push_back("DejaVuSans.ttf");
	v.push_back("wqy-zenhei.ttc");
	create_directory_if_missing(game_config::preferences_dir_utf8 + "/fonts");
	for (std::vector<std::string>::const_iterator it = v.begin(); it != v.end(); ++ it) {
		const std::string src = game_config::path + "/fonts/" + *it;
		const std::string dst = game_config::preferences_dir_utf8 + "/fonts/" + *it;
		if (!file_exists(dst)) {
			tfile src_file(src, GENERIC_READ, OPEN_EXISTING);
			tfile dst_file(dst, GENERIC_WRITE, CREATE_ALWAYS);
			if (src_file.valid() && dst_file.valid()) {
				int32_t fsize = src_file.read_2_data();
				posix_fwrite(dst_file.fp, src_file.data, fsize);
			}
		}
	}

	posix_print("create fonts directory, used %u\n", SDL_GetTicks() - start);
}

static std::string user_data_dir, user_data_dir_utf8, user_config_dir, cache_dir;

static void setup_user_data_dir();

void set_preferences_dir(std::string path)
{
#ifdef _WIN32
	if (path.empty()) {
		game_config::preferences_dir = get_cwd() + "/userdata";
	} else if (path.size() > 2 && path[1] == ':') {
		//allow absolute path override
		game_config::preferences_dir = path;
	} else {
		WCHAR wc[MAX_PATH];
		char my_documents_path[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, wc))) {
/*			WideCharToMultiByte(CP_ACP, 0, wc, -1, my_documents_path, MAX_PATH, NULL, NULL);
            std::string mygames_path = std::string(my_documents_path) + "/" + "My Games";
			boost::algorithm::replace_all(mygames_path, std::string("\\"), std::string("/"));
			create_directory_if_missing(mygames_path);
			game_config::preferences_dir = mygames_path + "/" + path;
*/
			// unicode to utf8
			// WCHAR wc[MAX_PATH];
			// SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, wc);
			WideCharToMultiByte(CP_UTF8, 0, wc, -1, my_documents_path, MAX_PATH, NULL, NULL);
			std::string mygames_path = std::string(my_documents_path) + "/" + "My Games";
			boost::algorithm::replace_all(mygames_path, std::string("\\"), std::string("/"));
			game_config::preferences_dir_utf8 = mygames_path + "/" + path;

			game_config::preferences_dir = game_config::preferences_dir_utf8;
		} else {
			game_config::preferences_dir = get_cwd() + "/" + path;
		}
	}
	// conv_ansi_utf8(game_config::preferences_dir, true);

#elif defined(ANDROID)
	// SDL_GetPrefPath use SDL_AndroidGetInternalStoragePath(). format: /data/data/com.leagor.sleep/files
	// SDL_AndroidGetExternalStoragePath(), format: /storage/emulated/0/Android/data/com.leagor.sleep/files
	game_config::preferences_dir = SDL_AndroidGetExternalStoragePath();
	if (!path.empty()) {
		game_config::preferences_dir += "/" + path;
	}

	// non-win32, assume no tow-code character.
	game_config::preferences_dir_utf8 = game_config::preferences_dir;

#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
    game_config::preferences_dir = getenv("HOME");
#else
    game_config::preferences_dir = get_cwd() + "/..";
#endif
    if (!path.empty()) {
        game_config::preferences_dir += "/" + path;
    }
    // non-win32, assume no tow-code character.
	game_config::preferences_dir_utf8 = game_config::preferences_dir;
#else

	std::string path2 = ".wesnoth" + game_config::wesnoth_version.str();

#ifdef _X11
	const char *home_str = getenv("HOME");

	if (path.empty()) {
		char const *xdg_data = getenv("XDG_DATA_HOME");
		if (!xdg_data || xdg_data[0] == '\0') {
			if (!home_str) {
				path = path2;
				goto other;
			}
			user_data_dir = home_str;
			user_data_dir += "/.local/share";
		} else user_data_dir = xdg_data;
		user_data_dir += "/wesnoth/";
		user_data_dir += game_config::wesnoth_version.str();
		create_directory_if_missing_recursive(user_data_dir);
		game_config::preferences_dir = user_data_dir;
	} else {
		other:
		std::string home = home_str ? home_str : ".";

		if (path[0] == '/')
			game_config::preferences_dir = path;
		else
			game_config::preferences_dir = home + "/" + path;
	}
#else
	if (path.empty()) path = path2;

	const char* home_str = getenv("HOME");
	std::string home = home_str ? home_str : ".";

	if (path[0] == '/')
		game_config::preferences_dir = path;
	else
		game_config::preferences_dir = home + std::string("/") + path;
#endif
	// non-win32, assume no tow-code character.
	game_config::preferences_dir_utf8 = game_config::preferences_dir;
#endif /*_WIN32*/

	game_config::preferences_dir = utils::normalize_path(game_config::preferences_dir);
	user_data_dir = game_config::preferences_dir;
	posix_print("set_preferences, user_data_dir: %s\n", user_data_dir.c_str());

	user_data_dir_utf8 = game_config::preferences_dir_utf8;
	setup_user_data_dir();
}

static void setup_user_data_dir()
{
	const std::string& dir_path = user_data_dir_utf8;

	const bool res = create_directory_if_missing(dir_path);
	// probe read permissions (if we could make the directory)
	if (!res || !is_directory(dir_path)) {
		ERR_FS << "could not open or create preferences directory at " << dir_path << '\n';
		return;
	}

	// Create user data and add-on directories
	create_directory_if_missing(dir_path + "/editor");
	create_directory_if_missing(dir_path + "/editor/maps");
	create_directory_if_missing(dir_path + "/data");
	create_directory_if_missing(dir_path + "/data/add-ons");
	create_directory_if_missing(dir_path + "/saves");
	create_directory_if_missing(dir_path + "/persist");
}

const std::string& get_user_data_dir()
{
	VALIDATE(!user_data_dir.empty(), null_str);
	return user_data_dir;
}

const std::string& get_user_data_dir_utf8()
{
	VALIDATE(!user_data_dir_utf8.empty(), null_str);
	return user_data_dir_utf8;
}

const std::string &get_user_config_dir()
{
	if (user_config_dir.empty())
	{
#if defined(_X11)
		char const *xdg_config = getenv("XDG_CONFIG_HOME");
		if (!xdg_config || xdg_config[0] == '\0') {
			xdg_config = getenv("HOME");
			if (!xdg_config) {
				user_config_dir = get_user_data_dir();
				return user_config_dir;
			}
			user_config_dir = xdg_config;
			user_config_dir += "/.config";
		} else user_config_dir = xdg_config;
		user_config_dir += "/wesnoth";
		create_directory_if_missing_recursive(user_config_dir);
#else
		user_config_dir = get_user_data_dir();
#endif
	}
	return user_config_dir;
}

static std::string read_stream(std::istream& s)
{
	std::stringstream ss;
	ss << s.rdbuf();
	return ss.str();
}

std::istream *istream_file(const std::string &fname, bool to_utf16)
{
	LOG_FS << "Streaming " << fname << " for reading.\n";
	if (fname.empty())
	{
		ERR_FS << "Trying to open file with empty name.\n";
		std::ifstream *s = new std::ifstream();
		s->clear(std::ios_base::failbit);
		return s;
	}
	std::ifstream *s = NULL;
#ifndef _WIN32
	// TODO: Should also be done for Windows; but *nix systems should
	// already be sufficient to catch most offenders.
	if (fname[0] != '/') {
		WRN_FS << "Trying to open file with relative path: '" << fname << "'.\n";
#if 0
		std::ifstream *s = new std::ifstream();
		s->clear(std::ios_base::failbit);
		return s;
#endif
	}
	s = new std::ifstream(fname.c_str(),std::ios_base::binary);
#else
	if (to_utf16) {
		// utf8 ---> utf16
		int wlen = MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, NULL, 0);
		WCHAR *wc = new WCHAR[wlen];
		MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, wc, wlen);
			
		s = new std::ifstream(wc, std::ios_base::binary);
		delete [] wc;
	} else {
		s = new std::ifstream(fname.c_str(), std::ios_base::binary);
	}
#endif
	if (s->is_open())
		return s;
	ERR_FS << "Could not open '" << fname << "' for reading.\n";
	return s;

}

std::string read_file(const std::string &fname)
{
	tfile lock(fname, GENERIC_READ, OPEN_EXISTING);
	int32_t fsize = lock.read_2_data();
	if (!fsize) {
		return null_str;
	}
	std::string ret(lock.data, fsize);
	return ret;
}

void write_file(const std::string& fname, const char* data, int len)
{
	posix_file_t fp;
	posix_fopen(fname.c_str(), GENERIC_WRITE, CREATE_ALWAYS, fp);
	if (fp == INVALID_FILE) {
		return;
	}
	posix_fwrite(fp, data, len);
	posix_fclose(fp);
}

std::string read_map(const std::string& name)
{
	std::string res;
	std::string map_location = get_wml_location("maps/" + name);
	if(!map_location.empty()) {
		res = read_file(map_location);
	}

	if (res.empty()) {
		res = read_file(get_user_data_dir() + "/editor/maps/" + name);
	}

	return res;
}

bool is_directory(const std::string& dir)
{
	if (dir.empty()) {
		return false;
	}

	return SDL_IsDirectory(dir.c_str());
}

bool file_exists(const std::string& name)
{
	if (name.empty()) {
		return false;
	}

	return SDL_IsFile(name.c_str());
}

void file_remove(const std::string& name)
{
	SDL_DeleteFiles(name.c_str());
}

void file_rename(const std::string& from, const std::string& to)
{
	// from and to require utf8 code.
	std::string from2 = from;
	std::string to2 = to;
#ifdef _WIN32
	conv_ansi_utf8(from2, false);
	conv_ansi_utf8(to2, false);
#endif

	rename(from2.c_str(), to2.c_str());
}

time_t file_create_time(const std::string& fname)
{
/*
	struct stat buf;
	if (::stat(fname.c_str(),&buf) == -1) {
		return 0;
	}

	return buf.st_mtime;
*/
	SDL_dirent dirent;
	if (!SDL_GetStat(fname.c_str(), &dirent)) {
		return 0;
	}
	return dirent.mtime;
}

int64_t file_size(const std::string& fname, bool to_utf16)
{
	if (fname.empty()) {
		return 0;
	}

	int64_t fsize;
	posix_fsize_byname(fname.c_str(), fsize);

	return fsize;
}

int64_t disk_free_space(const std::string& root_name)
{
	int64_t ret = -1;

#ifdef _WIN32
	const char* c_str = root_name.c_str();
	if (root_name.size() >= 2 && c_str[1] == ':') {
		std::string name2 = root_name.substr(0, 2);
		name2 += "\\";
		DWORD sectors_per_cluster, bytes_per_sector, free_clusters, clusters;

		GetDiskFreeSpaceA(root_name.c_str(), &sectors_per_cluster, &bytes_per_sector, &free_clusters, &clusters);
		ret = 1i64 * sectors_per_cluster * bytes_per_sector * free_clusters;
	}
// #elif (defined(__APPLE__) && TARGET_OS_IPHONE)
#else
	struct statfs buf;  
	if (statfs("/var", &buf) >= 0){
		posix_print("disk_free_space, root_name: %s, f_bsize: %i, f_bfree: %i\n", root_name.c_str(), buf.f_bsize, buf.f_bfree);
		ret = (int64_t)(buf.f_bsize * buf.f_bfree);  
	} 
#endif

	return ret;
}

/**
 * Returns true if the file ends with '.gz'.
 *
 * @param filename                The name to test.
 */
bool is_gzip_file(const std::string& filename)
{
	return (filename.length() > 3
		&& filename.substr(filename.length() - 3) == ".gz");
}

file_tree_checksum::file_tree_checksum()
	: nfiles(0), sum_size(0), modified(0)
{}

file_tree_checksum::file_tree_checksum(const config& cfg) :
	nfiles	(lexical_cast_default<size_t>(cfg["nfiles"])),
	sum_size(lexical_cast_default<size_t>(cfg["size"])),
	modified(lexical_cast_default<time_t>(cfg["modified"]))
{
}

void file_tree_checksum::write(config& cfg) const
{
	cfg["nfiles"] = lexical_cast<std::string>(nfiles);
	cfg["size"] = lexical_cast<std::string>(sum_size);
	cfg["modified"] = lexical_cast<std::string>(modified);
}

bool file_tree_checksum::operator==(const file_tree_checksum &rhs) const
{
	return nfiles == rhs.nfiles && sum_size == rhs.sum_size &&
		modified == rhs.modified;
}

static void get_file_tree_checksum_internal(const std::string& path, file_tree_checksum& res, int filter)
{

	std::vector<std::string> dirs;
	get_files_in_dir(path,NULL,&dirs, ENTIRE_FILE_PATH, filter, DONT_REORDER, &res);
	loadscreen::increment_progress();

	for (std::vector<std::string>::const_iterator j = dirs.begin(); j != dirs.end(); ++j) {
		get_file_tree_checksum_internal(*j, res, filter);
	}
}

const file_tree_checksum& data_tree_checksum(bool reset, int filter)
{
	static file_tree_checksum checksum;
	if (reset)
		checksum.reset();
	if(checksum.nfiles == 0) {
		get_file_tree_checksum_internal("data/", checksum, filter);
		get_file_tree_checksum_internal(get_user_data_dir() + "/data/", checksum, filter);
		LOG_FS << "calculated data tree checksum: "
			   << checksum.nfiles << " files; "
			   << checksum.sum_size << " bytes\n";
	}

	return checksum;
}

void data_tree_checksum(const std::vector<std::string>& paths, file_tree_checksum& checksum, int filter)
{
	checksum.reset();
	for (std::vector<std::string>::const_iterator it = paths.begin(); it != paths.end(); ++ it) {
		get_file_tree_checksum_internal(*it, checksum, filter);
	}
}

std::string file_name(const std::string& file)
// Analogous to POSIX basename(3), but for C++ string-object pathnames
{
#ifdef _WIN32
	static const std::string dir_separators = "\\/:";
#else
	static const std::string dir_separators = "/";
#endif

	std::string::size_type pos = file.find_last_of(dir_separators);

	if (pos == std::string::npos) {
		return file;
	}
	if (pos >= file.size() - 1) {
		return "";
	}

	return file.substr(pos + 1);
}

// terminate with directory symbol, for example c:/ddksample/apps-src/
std::string directory_name(const std::string& file)
{
#ifdef _WIN32
	static const std::string dir_separators = "\\/:";
#else
	static const std::string dir_separators = "/";
#endif

	std::string::size_type pos = file.find_last_of(dir_separators);

	if (pos == std::string::npos) {
		return "";
	}

	return file.substr(0, pos + 1);
}

std::string directory_name2(const std::string& file)
{
#ifdef _WIN32
	static const std::string dir_separators = "\\/:";
#else
	static const std::string dir_separators = "/";
#endif

	std::string::size_type pos = file.find_last_of(dir_separators);

	if (pos == std::string::npos) {
		return "";
	}

	return file.substr(0, pos);
}

std::string file_main_name(const std::string& file)
{
	std::string::size_type pos = file.find_last_of(".");

	if (pos == std::string::npos) {
		return file;
	}

	return file.substr(0, pos);
}

std::string file_ext_name(const std::string& file)
{
	std::string::size_type pos = file.find_last_of(".");

	if (pos == std::string::npos) {
		return "";
	}
	if (pos >= file.size() - 1) {
		return "";
	}

	return file.substr(pos + 1);
}

namespace {

#define PRIORITIEST_BINARY_PATHS	1
std::vector<std::string> binary_paths;

typedef std::map<std::string,std::vector<std::string> > paths_map;
paths_map binary_paths_cache;

}

static void init_binary_paths()
{
	if (binary_paths.empty()) {
		binary_paths.push_back(""); // userdata directory
		// here is self-add paths
		binary_paths.push_back(game_config::app_dir + "/");
		binary_paths.push_back("data/core/");
	}
}

binary_paths_manager::binary_paths_manager() : paths_()
{}

binary_paths_manager::binary_paths_manager(const config& cfg) : paths_()
{
	set_paths(cfg);
}

binary_paths_manager::~binary_paths_manager()
{
	cleanup();
}

void binary_paths_manager::set_paths(const config& cfg)
{
	cleanup();
	init_binary_paths();

	int inserted = 0;
	BOOST_FOREACH (const config &bp, cfg.child_range("binary_path")) {
		std::string path = bp["path"].str();
		if (path.find("..") != std::string::npos) {
			ERR_FS << "Invalid binary path '" << path << "'\n";
			continue;
		}
		if (!path.empty() && path[path.size()-1] != '/') {
			path += "/";
		}
		std::vector<std::string>::iterator it = std::find(binary_paths.begin(), binary_paths.end(), path);
		if (it == binary_paths.end()) {
			it = binary_paths.begin();
			std::advance(it, PRIORITIEST_BINARY_PATHS + inserted);
			binary_paths.insert(it, path);

			VALIDATE(std::find(paths_.begin(), paths_.end(), path) == paths_.end(), null_str);

			paths_.push_back(path);
			inserted ++;
		}
	}
}

void binary_paths_manager::cleanup()
{
	binary_paths_cache.clear();

	for (std::vector<std::string>::const_iterator i = paths_.begin(); i != paths_.end(); ++i) {
		std::vector<std::string>::iterator it2 = std::find(binary_paths.begin(), binary_paths.end(), *i);
		binary_paths.erase(it2);
	}
	paths_.clear();
}

void clear_binary_paths_cache()
{
	binary_paths_cache.clear();
}

const std::vector<std::string>& get_binary_paths(const std::string& type)
{
	const paths_map::const_iterator itor = binary_paths_cache.find(type);
	if(itor != binary_paths_cache.end()) {
		return itor->second;
	}

	if (type.find("..") != std::string::npos) {
		// Not an assertion, as language.cpp is passing user data as type.
		ERR_FS << "Invalid WML type '" << type << "' for binary paths\n";
		static std::vector<std::string> dummy;
		return dummy;
	}

	std::vector<std::string>& res = binary_paths_cache[type];

	init_binary_paths();

	BOOST_FOREACH (const std::string &path, binary_paths)
	{
		if (path.empty()) {
			res.push_back(get_user_data_dir() + "/");
			res.push_back(get_user_data_dir() + "/" + type + "/");
			res.push_back(game_config::path + "/");
		} else {
			res.push_back(game_config::path + "/" + path + type + "/");
		}
	}
	return res;
}

std::string get_binary_file_location(const std::string& type, const std::string& filename)
{
	DBG_FS << "Looking for '" << filename << "'.\n";

	if (filename.empty()) {
		LOG_FS << "  invalid filename (type: " << type <<")\n";
		return std::string();
	}

	// Some parts of Wesnoth enjoy putting ".." inside filenames. This is
	// bad and should be fixed. But in the meantime, deal with them in a dumb way.
	std::string::size_type pos = filename.rfind("../");
	if (pos != std::string::npos) {
		std::string nf = filename.substr(pos + 3);
		LOG_FS << "Illegal path '" << filename << "' replaced by '" << nf << "'\n";
		return get_binary_file_location(type, nf);
	}

	if (filename.find("..") != std::string::npos) {
		ERR_FS << "Illegal path '" << filename << "' (\"..\" not allowed).\n";
		return std::string();
	}

	BOOST_FOREACH (const std::string &path, get_binary_paths(type))
	{
		const std::string file = path + filename;
		DBG_FS << "  checking '" << path << "'\n";
		if(file_exists(file)) {
			DBG_FS << "  found at '" << file << "'\n";
			return file;
		}
	}

	DBG_FS << "  not found\n";
	return std::string();
}

std::string get_binary_dir_location(const std::string &type, const std::string &filename)
{
	DBG_FS << "Looking for '" << filename << "'.\n";

	if (filename.empty()) {
		LOG_FS << "  invalid filename (type: " << type <<")\n";
		return std::string();
	}

	if (filename.find("..") != std::string::npos) {
		ERR_FS << "Illegal path '" << filename << "' (\"..\" not allowed).\n";
		return std::string();
	}

	BOOST_FOREACH (const std::string &path, get_binary_paths(type))
	{
		const std::string file = path + filename;
		DBG_FS << "  checking '" << path << "'\n";
		if (is_directory(file)) {
			DBG_FS << "  found at '" << file << "'\n";
			return file;
		}
	}

	DBG_FS << "  not found\n";
	return std::string();
}

std::string get_wml_location(const std::string &filename, const std::string &current_dir)
{
	DBG_FS << "Looking for '" << filename << "'.\n";

	std::string result;

	if (filename.empty()) {
		LOG_FS << "  invalid filename\n";
		return result;
	}

	if (filename.find("..") != std::string::npos) {
		ERR_FS << "Illegal path '" << filename << "' (\"..\" not allowed).\n";
		return result;
	}

	bool already_found = false;

	if (filename[0] == '~') 
	{
		// If the filename starts with '~', look in the user data directory.
		result = get_user_data_dir() + "/data/" + filename.substr(1);
		DBG_FS << "  trying '" << result << "'\n";

		already_found = file_exists(result) || is_directory(result);
	} else if (filename.size() >= 2 && filename[0] == '.' && filename[1] == '/') {
		// If the filename begins with a "./", look in the same directory
		// as the file currrently being preprocessed.
		result = current_dir + filename.substr(2);
	} else if (filename[0] == '^') {
		// If the filename starts with '^', look in the topest directory.
		result = game_config::path + "/" + filename.substr(1);
	} else if (!game_config::path.empty()) {
		result = game_config::path + "/data/" + filename;
	}

	if (result.empty() || (!already_found && !file_exists(result) && !is_directory(result))) {
		result.clear();
	}

	return result;
}

std::string get_short_wml_path(const std::string &filename)
{
	std::string match = get_user_data_dir() + "/data/";
	if (filename.find(match) == 0) {
		return "~" + filename.substr(match.size());
	}
	match = game_config::path + "/data/";
	if (filename.find(match) == 0) {
		return filename.substr(match.size());
	}
	return filename;
}

char path_sep(bool standard)
{
#ifdef _WIN32
	return standard? '\\': '/';
#else
	return standard? '/': '\\';
#endif
}

static bool is_path_sep(char c)
{
#ifdef _WIN32
	if (c == '/' || c == '\\') return true;
#else
	if (c == '/') return true;
#endif
	return false;
}

std::string get_hdpi_name(const std::string& name, int hdpi_scale)
{
	size_t pos = name.rfind('.');
	if (pos == std::string::npos) {
		return name;
	}
	char flag[4] = {'@', static_cast<char>(hdpi_scale + 0x30), 'x', '\0'};

	std::string ret = name;
	ret.insert(pos, flag);
	return ret;
}

bool walk_dir(const std::string& rootdir, bool subfolders, const twalk_dir_function& fn)
{
	bool ret = true;
	std::stringstream ss;
	SDL_DIR* dir = SDL_OpenDir(rootdir.c_str());
	if (!dir) {
		return false;
	}
	SDL_dirent2* dirent;
	
	while ((dirent = SDL_ReadDir(dir))) {
		if (SDL_DIRENT_DIR(dirent->mode)) {
			if (SDL_strcmp(dirent->name, ".") && SDL_strcmp(dirent->name, "..")) {
				if (fn) {
					// shallow->deep: first call fn, then walk it.
					if (!fn(rootdir, dirent)) {
						ret = false;
						break;
					}
				}
				ss.str("");
				ss << rootdir << "/" << dirent->name;
				if (subfolders) {
					walk_dir(ss.str(), true, fn);
				}
			}
		} else {
			// file
			if (fn) {
				if (!fn(rootdir, dirent)) {
					ret = false;
					break;
				}
			}
		}
	}
	SDL_CloseDir(dir);

	return ret;
}

bool copy_root_files(const std::string& src, const std::string& dst, std::set<std::string>* files)
{
	if (files) {
		files->clear();
	}

	if (src.empty() || dst.empty()) {
		return false;
	}

	SDL_bool ret = SDL_TRUE;
	std::stringstream src_ss, dst_ss;
	SDL_DIR* dir = SDL_OpenDir(src.c_str());
	if (!dir) {
		return false;
	}
	SDL_dirent2* dirent;
	
	while ((dirent = SDL_ReadDir(dir))) {
		if (!SDL_DIRENT_DIR(dirent->mode)) {
			src_ss.str("");
			dst_ss.str("");
			src_ss << src << '/' << dirent->name;
			dst_ss << dst << '/' << dirent->name;
			ret = SDL_CopyFiles(src_ss.str().c_str(), dst_ss.str().c_str());

			if (!ret) {
				break;
			}
			if (files) {
				files->insert(dirent->name);
			}
		}
	}
	SDL_CloseDir(dir);

	return ret? true: false;
}

class tcompare_dir_param
{
public:
	tcompare_dir_param(const std::string& dir1, const std::string& dir2, int& recursion_count)
		: current_path1_(dir1)
		, current_path2_(dir2)
		, recursion_count_(recursion_count)
	{
		const char c = current_path1_.at(current_path1_.size() - 1);
		if (c == '\\' || c == '/') {
			current_path1_.erase(current_path1_.size() - 1);
		}

		if (!dir2.empty()) {
			const char c = current_path2_.at(current_path2_.size() - 1);
			if (c == '\\' || c == '/') {
				current_path2_.erase(current_path2_.size() - 1);
			}
		}
	}

	bool cb_compare_dir_explorer(const std::string& dir, const SDL_dirent2* dirent);

private:
	std::string current_path1_;
	std::string current_path2_;
	int& recursion_count_;
};

bool tcompare_dir_param::cb_compare_dir_explorer(const std::string& dir, const SDL_dirent2* dirent)
{
	bool compair = !current_path2_.empty();
	recursion_count_ ++;
	std::string path2 = current_path2_;
	if (compair) {
		path2.append("/");
		path2.append(dirent->name);
	}
	
	// compair
	if (SDL_DIRENT_DIR(dirent->mode)) {
		if (compair) {
			if (!is_directory(path2.c_str())) {
				return false;
			}
		}

		tcompare_dir_param cdp2(current_path1_ + "/" + dirent->name, path2, recursion_count_);
		if (!walk_dir(cdp2.current_path1_, false, boost::bind(&tcompare_dir_param::cb_compare_dir_explorer, &cdp2, _1, _2))) {
			return false;
		}

	} else if (compair) {
		if (!file_exists(path2)) {
			return false;
		}

	}
	return true;
}

bool compare_directory(const std::string& dir1, const std::string& dir2)
{
	int recursion1_count = 0;
	tcompare_dir_param cdp1(dir1, dir2, recursion1_count);
	bool fok = walk_dir(dir1, false, boost::bind(&tcompare_dir_param::cb_compare_dir_explorer, &cdp1, _1, _2));
	if (!fok) {
		return false;
	}

	int recursion2_count = 0;
	tcompare_dir_param cdp2(dir2, "", recursion2_count);
	fok = walk_dir(dir2, false, boost::bind(&tcompare_dir_param::cb_compare_dir_explorer, &cdp2, _1, _2));

	if (recursion1_count != recursion2_count) {
		return false;
	}
	return true;
}

scoped_istream& scoped_istream::operator=(std::istream *s)
{
	delete stream;
	stream = s;
	return *this;
}

scoped_istream::~scoped_istream()
{
	delete stream;
}

scoped_ostream& scoped_ostream::operator=(std::ostream *s)
{
	delete stream;
	stream = s;
	return *this;
}

scoped_ostream::~scoped_ostream()
{
	delete stream;
}

#ifdef _WIN32
/**
 * conv_ansi_utf8()
 *   - Convert a string between ANSI encoding (for Windows filename) and UTF-8
 *  string &name
 *     - filename to be converted
 *  bool a2u
 *     - if true, convert the string from ANSI to UTF-8.
 *     - if false, reverse. (convert it from UTF-8 to ANSI)
 */
void conv_ansi_utf8(std::string &name, bool a2u) 
{
	int wlen = MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0,
								   name.c_str(), -1, NULL, 0);
	if (wlen == 0) return;
	WCHAR *wc = new WCHAR[wlen];
	if (wc == NULL) return;
	if (MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0, name.c_str(), -1,
							wc, wlen) == 0) {
		delete [] wc;
		return;
	}
	int alen = WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
								   NULL, 0, NULL, NULL);
	if (alen == 0) {
		delete [] wc;
		return;
	}
	CHAR *ac = new CHAR[alen];
	if (ac == NULL) {
		delete [] wc;
		return;
	}
	WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
						ac, alen, NULL, NULL);
	delete [] wc;
	if (ac == NULL) {
		return;
	}
	name = ac;
	delete [] ac;

	return;
}

std::string conv_ansi_utf8_2(const std::string &name, bool a2u) 
{
	int wlen = MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0,
								   name.c_str(), -1, NULL, 0);
	if (wlen == 0) return "";
	WCHAR *wc = new WCHAR[wlen];
	if (wc == NULL) return "";
	if (MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0, name.c_str(), -1,
							wc, wlen) == 0) {
		delete [] wc;
		return "";
	}
	int alen = WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
								   NULL, 0, NULL, NULL);
	if (alen == 0) {
		delete [] wc;
		return "";
	}
	CHAR *ac = new CHAR[alen];
	if (ac == NULL) {
		delete [] wc;
		return "";
	}
	WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
						ac, alen, NULL, NULL);
	delete [] wc;
	if (ac == NULL) {
		return "";
	}
	std::string result = ac;
	delete [] ac;

	return result;
}

const char* utf8_2_ansi(const std::string& str)
{
	static std::string ret;
	ret = conv_ansi_utf8_2(str, false);
	return ret.c_str();
}

const char* ansi_2_utf8(const std::string& str)
{
	static std::string ret;
	ret = conv_ansi_utf8_2(str, true);
	return ret.c_str();
}

#else

void conv_ansi_utf8(std::string &name, bool a2u)
{
}

std::string conv_ansi_utf8_2(const std::string &name, bool a2u)
{
	return name;
}

const char* utf8_2_ansi(const std::string& str)
{
	static std::string ret = str;
	return ret.c_str();
}

const char* ansi_2_utf8(const std::string& str)
{
	static const std::string ret = str;
	return ret.c_str();
}

#endif

std::string format_time_ymd(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf, sizeof(time_buf), _("%b %d %y"), tm_l);
		if (res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_time_ymd2(time_t t, const std::string& separator)
{
	std::stringstream ss;
	tm* tm_l = localtime(&t);

	ss << tm_l->tm_year + 1900;
	ss << separator;
	ss << tm_l->tm_mon + 1;
	ss << separator;
	ss << tm_l->tm_mday;

	return ss.str();
}

std::string format_time_hms(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf, sizeof(time_buf), _("%H:%M:%S"), tm_l);
		if (res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_time_hm(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf, sizeof(time_buf), _("%H:%M"), tm_l);
		if (res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_time_date(time_t t)
{
	time_t curtime = time(NULL);
	const struct tm* timeptr = localtime(&curtime);
	if(timeptr == NULL) {
		return "";
	}

	const struct tm current_time = *timeptr;

	timeptr = localtime(&t);
	if(timeptr == NULL) {
		return "";
	}

	const struct tm save_time = *timeptr;

	const char* format_string = _("%b %d %y");

	if (current_time.tm_year == save_time.tm_year) {
		const int days_apart = current_time.tm_yday - save_time.tm_yday;

		if (days_apart == 0) {
			// save is from today
			format_string = _("%H:%M:%S");
		} else if(days_apart > 0 && days_apart <= current_time.tm_wday) {
			// save is from this week. On chinese, %A cannot display. use %m%d instead.
			format_string = _("%A, %H:%M");
		} else {
			// save is from current year
			// format_string = _("%b %d");
			format_string = _("%A, %H:%M");
		}
	} else {
		// save is from a different year
		format_string = _("%b %d %y");
	}

	char buf[64];
	const size_t res = strftime(buf, sizeof(buf), format_string, &save_time);
	if(res == 0) {
		buf[0] = 0;
	}
	
	return buf;
}

std::string format_time_local(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf,sizeof(time_buf),_("%a %b %d %H:%M %Y"),tm_l);
		if(res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_time_ymdhms(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_l);
		if (res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_elapse_hms(time_t elapse, bool align)
{
	if (elapse < 0) {
		elapse = 0;
	}
	int sec = elapse % 60;
	int min = (elapse / 60) % 60;
	int hour = elapse / 3600;

	std::stringstream ss;
	if (align || hour) {
		if (align) {
			ss << std::setfill('0') << std::setw(2);
		}
		ss << hour << _("time^h");
	}
	if (align || hour || min) {
		if (align) {
			ss << std::setfill('0') << std::setw(2);
		}
		ss << min << _("time^m");
	}
	if (align) {
		ss << std::setfill('0') << std::setw(2);
	}
	ss << sec << _("time^s");
	
	return ss.str();
}

std::string format_elapse_hms2(time_t elapse, bool align)
{
	if (elapse < 0) {
		elapse = 0;
	}
	int sec = elapse % 60;
	int min = (elapse / 60) % 60;
	int hour = (elapse / 3600) % 24;
	int day = elapse / (3600 * 24);

	std::stringstream strstr;
	if (day) {
		utils::string_map symbols;
		symbols["d"] = str_cast(day);
		strstr << vgettext2("$d days", symbols) << " ";
	}
	strstr << std::setfill('0') << std::setw(2) << hour << ":";
	strstr << std::setfill('0') << std::setw(2) << min << ":";
	strstr << std::setfill('0') << std::setw(2) << sec;
	
	return strstr.str();
}

std::string format_elapse_hm(time_t elapse, bool align)
{
	if (elapse < 0) {
		elapse = 0;
	}
	int sec = elapse % 60;
	int min = (elapse / 60) % 60;
	int hour = (elapse / 3600) % 24;
	int day = elapse / (3600 * 24);

	std::stringstream ss;
	if (align) {
		ss << std::setfill('0') << std::setw(2);
	}
	ss << hour << _("time^h");
	ss << std::setfill('0') << std::setw(2) << min << _("time^m");
	
	return ss.str();
}

std::string format_elapse_hm2(time_t elapse, bool align)
{
	if (elapse < 0) {
		elapse = 0;
	}
	int sec = elapse % 60;
	int min = (elapse / 60) % 60;
	int hour = (elapse / 3600) % 24;
	int day = elapse / (3600 * 24);

	std::stringstream ss;
	if (align) {
		ss << std::setfill('0') << std::setw(2);
	}
	ss << hour << ":";
	ss << std::setfill('0') << std::setw(2) << min;
	
	return ss.str();
}

std::string format_elapse_ms(time_t elapse, bool align)
{
	if (elapse < 0) {
		elapse = 0;
	}
	int sec = elapse % 60;
	int min = elapse / 60;

	std::stringstream ss;
	if (min || align) {
		if (align) {
			ss << std::setfill('0') << std::setw(2);
		}
		ss << min << _("time^m");
	}
	if (align) {
		ss << std::setfill('0') << std::setw(2);
	}
	ss << sec << _("time^s");
	
	return ss.str();
}

// replace src_str in src_file, and generate to dst_file.
bool file_replace_string(const std::string& src_file, const std::string& dst_file, const std::vector<std::pair<std::string, std::string> >& replaces)
{
	tfile file(src_file,  GENERIC_READ, OPEN_EXISTING);
	int fsize = file.read_2_data();
	if (!fsize) {
		return false;
	}
	fsize = file.replace_string(fsize, replaces, NULL);
	
	// write data to new file
	tfile file2(dst_file,  GENERIC_WRITE, CREATE_ALWAYS);
	if (file2.valid()) {
		posix_fseek(file2.fp, 0);
		posix_fwrite(file2.fp, file.data, fsize);
	}

	return true;
}

bool file_replace_string(const std::string& src_file, const std::pair<std::string, std::string>& replace)
{
	std::vector<std::pair<std::string, std::string> > replaces;
	replaces.push_back(replace);
	return file_replace_string(src_file, replaces);
}

// replace src_str in src_file, and generate to dst_file.
bool file_replace_string(const std::string& src_file, const std::vector<std::pair<std::string, std::string> >& replaces)
{
	tfile file(src_file,  GENERIC_WRITE, OPEN_EXISTING);
	int fsize = file.read_2_data();
	if (!fsize) {
		return false;
	}

	bool dirty = false;
	fsize = file.replace_string(fsize, replaces, &dirty);

	if (dirty) {
		posix_fseek(file.fp, 0);
		posix_fwrite(file.fp, file.data, fsize);
		file.truncate(fsize);
	}
	return true;
}

//
// encrypt/decrypt
//
// if return isn't NULL, caller need free heap.
char* saes_encrypt_heap(const char* ptext, int size, unsigned char* key)
{
	if (size == 0) {
		return NULL;
	}
	char* ctext = (char*)malloc((size & 1)? size + 1: size);
	saes_encrypt_stream((const unsigned char*)ptext, size, key, ctext);
	return ctext;
}

// if return isn't NULL, caller need free heap.
char* saes_decrypt_heap(const char* ctext, int size, unsigned char* key)
{
	if (size == 0 || (size & 1)) {
		return NULL;
	}
	char* ptext = (char*)malloc(size);
	saes_decrypt_stream((const unsigned char*)ctext, size, key, ptext);
	return ptext;
}

tsaes_encrypt::tsaes_encrypt(const char* ctext, int s, unsigned char* key)
	: size(0)
{
	buf = saes_encrypt_heap(ctext, s, key);
	if (buf) {
		size = s & 1? s + 1: s;
	}
}

tsaes_decrypt::tsaes_decrypt(const char* ptext, int s, unsigned char* key)
	: size(s)
{
	buf = saes_decrypt_heap(ptext, size, key);
	if (buf && !buf[size - 1]) {
		size --;
	}
}

tfile::tfile(const std::string& file, uint32_t desired_access, uint32_t create_disposition)
	: fp(INVALID_FILE)
	, data(NULL)
	, data_size(0)
	, truncate_size_(0)
	, can_truncate_(create_disposition == CREATE_ALWAYS && (desired_access & GENERIC_WRITE))
{
	VALIDATE(create_disposition == OPEN_EXISTING || create_disposition == CREATE_ALWAYS, null_str);
	// CREATE_ALWAYS: create always. if file exist, it will truncate to 0 immediately.
	posix_fopen(file.c_str(), desired_access, create_disposition, fp);
}

void tfile::close()
{
	if (data) {
		free(data);
		data = NULL;
		data_size = 0;
	}
	if (fp == INVALID_FILE) {
		return;
	}

	if (!can_truncate_ && truncate_size_ > 0) {
		// when visual studio is using apps.sln, cannot delete it.
		// so must truncate apps.sln.
		// now SDL doesn't support truncate file. if I add, it necessary modify more.
		// in windows, use SetEndOfFile directly.
		// if linux-like, use ftruncate to truncate a file.
		int64_t fsize = posix_fsize(fp);
		if (fsize > truncate_size_) {
#ifdef _WIN32
			posix_fseek(fp, truncate_size_);
			SetEndOfFile(fp->hidden.windowsio.h);
#else
			// http://www.cnblogs.com/sky-heaven/p/4663630.html
			fflush(fp->hidden.stdio.fp);
			ftruncate(fileno(fp->hidden.stdio.fp), truncate_size_);
			rewind(fp->hidden.stdio.fp);
#endif
		}
	}

	posix_fclose(fp);
	fp = INVALID_FILE;
}

void tfile::resize_data(int size, int vsize)
{
	size = posix_align_ceil(size, 4096);
	VALIDATE(size >= 0, null_str);

	if (size > data_size) {
		char* tmp = (char*)malloc(size);
		if (data) {
			if (vsize) {
				memcpy(tmp, data, vsize);
			}
			free(data);
		}
		data = tmp;
		data_size = size;
	}
}

int tfile::replace_span(int start, int original_size, const char* new_data, int new_size, int fsize)
{
	int new_fsize = fsize + new_size - original_size;
	resize_data(new_fsize + 1, fsize); // extra 1 is for string terminate: '\0'.

	if (new_size <= original_size) {
		memcpy(data + start + new_size, data + start + original_size, fsize - start - original_size);
	} else {
		memmove(data + start + new_size, data + start + original_size, fsize - start - original_size);
	}
	if (new_data) {
		memcpy(data + start, new_data, new_size);
	}

	data[new_fsize] = '\0';
	return new_fsize; 
}

int64_t tfile::read_2_data()
{
	if (!valid()) {
		return 0;
	}
	int64_t fsize = posix_fsize(fp);
	if (!fsize) {
		return 0;
	}
	VALIDATE(fsize, null_str);

	posix_fseek(fp, 0);
	resize_data(fsize);
	posix_fread(fp, data, fsize);

	return fsize;
}

// caller require fill data before it.
// only modify data. doesn't touch file data.
int tfile::replace_string(int fsize, const std::vector<std::pair<std::string, std::string> >& replaces, bool* dirty)
{
	VALIDATE(data && data_size >= fsize, null_str);
	resize_data(fsize + 1, fsize);
	data[fsize] = '\0';

	const char* start2 = data; // search from 0.
	if (utils::bom_magic_started((const uint8_t*)data, fsize)) {
		start2 += BOM_LENGTH;
	}

	if (dirty) {
		*dirty = false;
	}
	for (std::vector<std::pair<std::string, std::string> >::const_iterator it = replaces.begin(); it != replaces.end(); ++ it) {
		const std::pair<std::string, std::string>& replace = *it;
		if (replace.first != replace.second) {
			const std::string& src = replace.first;
			const std::string& dst = replace.second;
			const char* ptr = strstr(start2, src.c_str());
			while (ptr) {
				// replace_span may remalloc data!
				int pos = ptr - data;
				fsize = replace_span(pos, src.size(), dst.c_str(), dst.size(), fsize);
				// replace next
				ptr = strstr(data + pos + dst.size(), src.c_str());
				if (dirty) {
					*dirty = true;
				}
			}
		}
	}

	return fsize;
}

int posix_align_ceil2(int dividend, int divisor)
{
	int remainer = dividend % divisor;
	return dividend + (remainer? divisor - remainer: 0);
}

#ifdef _WIN32
#define POSIX_DBG_BUFF_SIZE			1024
void posix_print(const char *formt, ...)
{
	va_list va_param;
	char szBuf[POSIX_DBG_BUFF_SIZE];

	// GetSystemTime(&SystemTime);
			
	va_start(va_param, formt);
	vsprintf(szBuf, formt, va_param);
	va_end(va_param);

	OutputDebugStringA((LPCSTR)szBuf);
}

void posix_print_mb(const char *formt, ...)
{
	va_list va_param;
	char szBuf[POSIX_DBG_BUFF_SIZE];
			
	va_start(va_param, formt);
	vsprintf(szBuf, formt, va_param);
	va_end(va_param);

	MessageBoxA(NULL, (LPCSTR)szBuf, game_config::app_title.c_str(), MB_OK);
}
#endif