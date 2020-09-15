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

#include "functional.h"

#include <primitives/http.h>
#include <primitives/pack.h>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "updater");

#include <process.h>

int version()
{
    return BOOTSTRAP_UPDATER_VERSION;
}

void print_version()
{
    LOG_INFO(logger, "Polygon-4 Bootstrap Updater Version " << version());
}

void check_version(int ver)
{
    if (ver == version())
        return;
    LOG_FATAL(logger, "You have wrong version of bootstrap updater!");
    LOG_FATAL(logger, "Actual version: " << ver);
    LOG_FATAL(logger, "Your version: " << version());
    LOG_FATAL(logger, "Please, run updater.exe to update the component.");
    exit_program(1);
}

int bootstrap_module_main(int argc, char *argv[], const ptree &data)
{
    init();
    check_version(data.get<int>("bootstrap.updater.version"));

    auto bootstrap_zip = data.get<String>("bootstrap.updater.archive_name");
    auto file = BOOTSTRAP_DOWNLOADS / bootstrap_zip;
    auto bak = file;
    bak += ".bak";
    if (fs::exists(file))
        fs::copy_file(file, bak, fs::copy_options::overwrite_existing);

    auto bootstrapper_new = BOOTSTRAP_DOWNLOADS / "bootstrapper.new";
    download_file(data.get<String>("bootstrap.url"), file);
    fs::remove_all(bootstrapper_new);
    unpack_file(file, bootstrapper_new);
    try
    {
		copy_dir(bootstrapper_new, ".");
    }
    catch (...)
    {
    }

    // self update
    auto updater = bootstrapper_new / path(argv[0]).filename();
    auto exe = normalize_string_copy(absolute(updater).wstring());
    auto arg0 = L"\"" + exe + L"\"";
    auto dst = L"\"" + to_wstring(argv[0]) + L"\"";
    if (_wexecl(exe.c_str(), arg0.c_str(), L"--copy", dst.c_str(), 0) == -1)
    {
        LOG_FATAL(logger, "errno = " << errno);
        LOG_FATAL(logger, "Cannot update myself. Close the program and replace this file with newer updater.");
        exit_program(1);
    }

    return 0;
}
