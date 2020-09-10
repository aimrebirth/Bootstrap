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

#include <boost/property_tree/json_parser.hpp>

#include <primitives/command.h>
#include <primitives/filesystem.h>

#include <iostream>
#include <set>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>
#include <unordered_set>

namespace pt = boost::property_tree;

#define BOOTSTRAP_JSON_FILE "Bootstrap.json"
#define BOOTSTRAP_JSON_DIR "https://raw.githubusercontent.com/aimrebirth/Bootstrap/master/"
#define BOOTSTRAP_JSON_URL BOOTSTRAP_JSON_DIR BOOTSTRAP_JSON_FILE

extern const int BOOTSTRAPPER_VERSION;
extern const int BOOTSTRAP_UPDATER_VERSION;
extern const int UNTRACKED_CONTENT_DELETER_VERSION;
extern const int BOOTSTRAPPER_TOOLS;

#define POLYGON4_NAME "Polygon4"
#define BOOTSTRAP_PREFIX path("..")
#define BOOTSTRAP_DOWNLOADS path("BootstrapDownloads")
#define BOOTSTRAP_PROGRAMS path("BootstrapPrograms")
#define BOOTSTRAP_COPY_UPDATER "CopyUpdater"

#define uvs BOOTSTRAP_PROGRAMS / "UnrealVersionSelector"

#define LAST_WRITE_TIME_DATA "lwt.json"

//
// types
//

using ptree = pt::ptree;

//
// global data
//

extern path git;
extern std::thread::id main_thread_id;

//
// function declarations
//

// helpers
path temp_directory_path(const path &subdir = path());
path get_temp_filename(const path &subdir = path());

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
void git_checkout(const path &dir, const String &url);
void download_sources(const String &url);
void download_submodules();
void update_sources();
void manual_download_sources(const path &dir, const ptree &data);
void download_files(const path &dir, const path &output_dir, const ptree &data);

void enumerate_files(const path &dir, std::set<path> &files);
void remove_untracked(const ptree &data, const path &dir, const path &content_dir);

void execute_and_print(primitives::Command &c, bool exit_on_error = true);
void execute_and_print(const primitives::command::Arguments &args, bool exit_on_error = true);
