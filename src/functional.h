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

#include <iostream>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "md5.h"
#include "Logger.h"

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

#define BOOTSTRAP_JSON_FILE L"Bootstrap.json"
#define BOOTSTRAP_JSON_DIR L"https://raw.githubusercontent.com/aimrebirth/Bootstrap/master/"
#define BOOTSTRAP_JSON_URL BOOTSTRAP_JSON_DIR BOOTSTRAP_JSON_FILE

#define BOOTSTRAPPER_VERSION                4
#define BOOTSTRAP_UPDATER_VERSION           1

#define UNTRACKED_CONTENT_DELETER_VERSION   2
#define BOOTSTRAPPER_TOOLS                  1

#define POLYGON4_NAME L"Polygon4"
#define BOOTSTRAP_DOWNLOADS L"BootstrapDownloads/"
#define BOOTSTRAP_PROGRAMS L"BootstrapPrograms/"
#define BOOTSTRAP_COPY_UPDATER L"CopyUpdater"

#define _7z BOOTSTRAP_PROGRAMS L"7za"
#define uvs BOOTSTRAP_PROGRAMS L"UnrealVersionSelector"
#define curl BOOTSTRAP_PROGRAMS L"curl"

#define ZIP_EXT L".zip"

#define LAST_WRITE_TIME_DATA L"lwt.json"

//
// types
//

using path = fs::wpath;
using ptree = pt::wptree;

using String = std::wstring;
using Strings = std::vector<String>;

using Byte = uint8_t;
using Bytes = std::vector<Byte>;

struct SubprocessAnswer
{
    int ret = 0;
    Bytes bytes;
};

enum DownloadFlags
{
    D_DEFAULT           =   0x0,
    D_SILENT            =   0x1,
    D_CURL_SILENT       =   0x2,
    D_NO_SPACE          =   0x4
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
std::string to_string(std::wstring s);
std::wstring to_wstring(std::string s);

// main
int bootstrap_module_main(int argc, char *argv[], const ptree &data);
void init();
int version();
void print_version();

// all other
ptree load_data(const String &url);
ptree load_data(const path &dir);
Bytes download(const String &url);
void download(const String &url, const String &file, int flags = D_DEFAULT);
void exit_program(int code);
void check_return_code(int code);
String to_string(const Bytes &b);
void check_version(int version);
bool has_program_in_path(String &prog);
void git_checkout(const path &dir, const String &url);
void download_sources(const String &url);
void download_submodules();
void update_sources();
void manual_download_sources(const path &dir, const ptree &data);
void unpack(const String &file, const String &output_dir, bool exit_on_error = true);
bool copy_dir(const path &source, const path &destination);
void download_files(const path &dir, const path &output_dir, const ptree &data);
void run_cmake(const path &dir);
void build_engine(const path &dir);
void create_project_files(const path &dir);
void build_project(const path &dir);
Bytes read_file(const String &file);

void execute_and_print(Strings args, bool exit_on_error = true);
SubprocessAnswer execute_command(Strings args, bool exit_on_error = true);
