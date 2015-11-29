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
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "md5.h"

using namespace std;
using namespace boost::filesystem;
using namespace boost::process;
namespace pt = boost::property_tree;

#define BOOTSTRAP_JSON_FILE L"Bootstrap.json"
#define BOOTSTRAP_JSON_DIR L"https://raw.githubusercontent.com/aimrebirth/Bootstrap/master/"
#define BOOTSTRAP_JSON_URL BOOTSTRAP_JSON_DIR BOOTSTRAP_JSON_FILE

#define BOOTSTRAPPER_VERSION                3
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

#define PRINT(x) {cerr << x << "\n"; cerr.flush();}
#define SPACE() PRINT("")
#define CATCH(expr, ex, msg) \
    try { expr; } catch (ex &e) { PRINT(msg << ": " << e.what()); exit(1); }

//
// types
//

typedef vector<string> Strings;
typedef vector<wstring> WStrings;

typedef uint8_t Byte;
typedef vector<Byte> Bytes;

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

extern wstring git;
extern wstring cmake;
extern wstring msbuild;

//
// function declarations
//

// helpers
std::string to_string(std::wstring s);
std::wstring to_wstring(std::string s);

// main
int bootstrap_module_main(int argc, char *argv[], const pt::wptree &data);
void init();
int version();
void print_version();

// all other
pt::wptree load_data(const wstring &url);
Bytes download(const wstring &url);
void download(const wstring &url, const wstring &file, int flags = D_DEFAULT);
void exit_program(int code);
SubprocessAnswer execute_command(WStrings args, bool exit_on_error = true, stream_behavior stdout_behavior = inherit_stream());
void check_return_code(int code);
wstring to_string(const Bytes &b);
void check_version(int version);
bool has_program_in_path(wstring &prog);
void git_checkout(const wpath &dir, const wstring &url);
void download_sources(const wstring &url);
void download_submodules();
void update_sources();
void manual_download_sources(const wpath &dir, const pt::wptree &data);
void unpack(const wstring &file, const wstring &output_dir, bool exit_on_error = true);
bool copy_dir(const wpath &source, const wpath &destination);
void download_files(const wpath &dir, const wpath &output_dir, const pt::wptree &data);
void run_cmake(const wpath &dir);
void build_engine(const wpath &dir);
void create_project_files(const wpath &dir);
void build_project(const wpath &dir);
Bytes read_file(const wstring &file);
