/*
 * Polygon-4 Bootstrapper
 * Copyright (C) 2015 lzwdgc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/range.hpp>

#include <boost/process.hpp>

#include <boost/property_tree/json_parser.hpp>

#include <iostream>
#include <set>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>
#include <unordered_set>

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
namespace bp = boost::process;

#define BOOTSTRAP_JSON_FILE "Bootstrap.json"
#define BOOTSTRAP_JSON_DIR "https://raw.githubusercontent.com/aimrebirth/Bootstrap/master/"
#define BOOTSTRAP_JSON_URL BOOTSTRAP_JSON_DIR BOOTSTRAP_JSON_FILE

#define BOOTSTRAPPER_VERSION                7
#define BOOTSTRAP_UPDATER_VERSION           1

#define UNTRACKED_CONTENT_DELETER_VERSION   2
#define BOOTSTRAPPER_TOOLS                  1

#define POLYGON4_NAME "Polygon4"
#define BOOTSTRAP_PREFIX "../"
#define BOOTSTRAP_DOWNLOADS "BootstrapDownloads/"
#define BOOTSTRAP_PROGRAMS "BootstrapPrograms/"
#define BOOTSTRAP_COPY_UPDATER "CopyUpdater"

#define uvs BOOTSTRAP_PROGRAMS "UnrealVersionSelector"

#define LAST_WRITE_TIME_DATA "lwt.json"

//
// types
//

using path = fs::wpath;
using Files = std::unordered_set<path>;

namespace std
{
    template<> struct hash<path>
    {
        size_t operator()(const path& p) const
        {
            return fs::hash_value(p);
        }
    };
}

using ptree = pt::ptree;

using String = std::string;
using Strings = std::vector<String>;

using namespace std::literals;

struct SubprocessAnswer
{
    int ret = 0;
    String out;
    String err;
};

//
// global data
//

extern String git;
extern String cmake;
extern String msbuild;

extern std::thread::id main_thread_id;

//
// function declarations
//

// helpers
std::string to_string(const std::wstring &s);
std::wstring to_wstring(const std::string &s);

path temp_directory_path(const path &subdir = path());
path get_temp_filename(const path &subdir = path());

String read_file(const path &p, bool no_size_check = false);
void write_file(const path &p, const String &s);

String md5(const path &fn);
String md5(const String &data);

// main
int bootstrap_module_main(int argc, char *argv[], const ptree &data);
void init();
int version();
void print_version();

// all other
ptree load_data(const String &url);
ptree load_data(const path &dir);
void exit_program(int code);
void check_return_code(int code);
void check_version(int version);
bool has_program_in_path(String &prog);
void git_checkout(const path &dir, const String &url);
void download_sources(const String &url);
void download_submodules();
void update_sources();
void manual_download_sources(const path &dir, const ptree &data);
bool copy_dir(const path &source, const path &destination);
void download_files(const path &dir, const path &output_dir, const ptree &data);
void run_cmake(const path &dir);
void build_engine(const path &dir);
void create_project_files(const path &dir);
void build_project(const path &dir);

void enumerate_files(const path &dir, std::set<path> &files);
void remove_untracked(const ptree &data, const path &dir, const path &content_dir);

void execute_and_print(Strings args, bool exit_on_error = true);
SubprocessAnswer execute_command(Strings args, bool exit_on_error = true);
