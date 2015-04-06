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

#define BOOTSTRAP_JSON_URL "https://raw.githubusercontent.com/aimrebirth/Bootstrap/master/Bootstrap.json"

#define BOOTSTRAPPER_VERSION        1
#define BOOTSTRAP_UPDATER_VERSION   1

#define POLYGON4_NAME "Polygon4"
#define BOOTSTRAP_DOWNLOADS "BootstrapDownloads/"
#define BOOTSTRAP_PROGRAMS "BootstrapPrograms/"
#define BOOTSTRAP_COPY_UPDATER "CopyUpdater"

#define _7z BOOTSTRAP_PROGRAMS "7za"
#define uvc BOOTSTRAP_PROGRAMS "UnrealVersionSelector"
#define curl BOOTSTRAP_PROGRAMS "curl"

#define ZIP_EXT ".zip"

#define PRINT(x) {cout << x << "\n"; cout.flush();}
#define SPACE() PRINT("")
#define CATCH(expr, ex, msg) \
    try { expr; } catch (ex e) { PRINT(msg); exit(1); }

//
// types
//

typedef vector<string> Strings;

typedef uint8_t Byte;
typedef vector<Byte> Bytes;

struct SubprocessAnswer
{
    int ret = 0;
    Bytes bytes;
};

//
// global data
//

extern string git;
extern string cmake;
extern string msbuild;

//
// function declarations
//

// main
int bootstrap_module_main(int argc, char *argv[], const pt::ptree &data);
void init();
int version();
void print_version();

// all other
pt::ptree load_data();
Bytes download(string url);
void download(string url, string file);
void exit_program(int code);
SubprocessAnswer execute_command(Strings args, bool exit_on_error = true, stream_behavior stdout_behavior = inherit_stream());
void check_return_code(int code);
string to_string(const Bytes &b);
void check_version(int version);
bool has_program_in_path(string &prog);
void git_checkout(path dir, string url);
void download_sources(string url);
void download_submodules();
void update_sources();
void manual_download_sources(const pt::ptree &data);
void unpack(string file, string output_dir, bool exit_on_error = true);
bool copy_dir(const path &source, const path &destination);
void download_files(path dir, path output_dir, const pt::ptree &files, string file_prefix);
void run_cmake(path dir);
void build_engine(path dir);
void create_project_files(path dir);
void build_project(path dir);
