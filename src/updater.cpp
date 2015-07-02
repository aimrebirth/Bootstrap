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

#include <process.h>

wstring bootstrap_programs_prefix;

int version()
{
    return BOOTSTRAP_UPDATER_VERSION;
}

void print_version()
{
    PRINT("Polygon-4 Bootstrap Updater Version " << version());
    SPACE();
}

void check_version(int ver)
{
    if (ver == version())
        return;
    PRINT("FATAL ERROR:");
    PRINT("You have wrong version of bootstrap updater!");
    PRINT("Actual version: " << ver);
    PRINT("Your version: " << version());
    PRINT("Please, run BootstrapUpdater.exe to update the component.");
    exit_program(1);
}

int bootstrap_module_main(int argc, char *argv[], const pt::wptree &data)
{
    init();
    check_version(data.get<int>(L"bootstrap.updater.version"));

    auto bootstrap_zip = data.get<wstring>(L"bootstrap.updater.archive_name");
    auto file = BOOTSTRAP_DOWNLOADS + bootstrap_zip;
    auto bak = file + L".bak";
    if (exists(file))
        copy_file(file, bak, copy_option::overwrite_if_exists);

    // remove old files
    remove("Bootstrap.exe");
    remove("BootstrapLog.ps1");
    remove("Bootstrap.log");

    auto bootstrapper_new = BOOTSTRAP_DOWNLOADS L"bootstrapper.new";
    download(data.get<wstring>(L"bootstrap.url"), file);
    remove_all(bootstrapper_new);
    unpack(file, bootstrapper_new, false);
    copy_dir(bootstrapper_new, ".");

    // self update
    wpath updater = bootstrapper_new / path(argv[0]).filename();
    auto exe = absolute(updater).normalize().wstring();
    auto arg0 = L"\"" + exe + L"\"";
    auto dst = L"\"" + to_wstring(argv[0]) + L"\"";
    if (_wexecl(exe.c_str(), arg0.c_str(), L"--copy", dst.c_str(), 0) == -1)
    {
        PRINT("FATAL ERROR: errno = " << errno);
        PRINT("Cannot update myself. Close the program and replace this file with newer BootstrapUpdater.");
        exit_program(1);
    }

    return 0;
}