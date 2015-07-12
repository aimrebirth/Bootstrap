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

wstring bootstrap_programs_prefix;

int version()
{
    return BOOTSTRAPPER_TOOLS;
}

void print_version()
{
    PRINT("Polygon-4 Tools Bootstrapper Version " << version());
    SPACE();
}

void check_version(int ver)
{
    if (ver == version())
        return;
    PRINT("FATAL ERROR:");
    PRINT("You have wrong version of bootstrapper!");
    PRINT("Actual version: " << ver);
    PRINT("Your version: " << version());
    PRINT("Please, run BootstrapUpdater.exe to update the bootstrapper.");
    exit_program(1);
}

int bootstrap_module_main(int argc, char *argv[], const pt::wptree &data)
{
    init();
    check_version(data.get<int>(L"bootstrap.tools.version"));

    wpath base_dir = current_path();
    wpath download_dir = base_dir / BOOTSTRAP_DOWNLOADS;

    download_files(download_dir, current_path(), data.get_child(L"tools"));

    PRINT("Bootstraped Polygon-4 Tools successfully");

    return 0;
}