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

int version()
{
    return BOOTSTRAPPER_VERSION;
}

void print_version()
{
    PRINT("Polygon-4 Developer Bootstrapper Version " << version());
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
    check_version(data.get<int>(L"bootstrap.version"));

    auto polygon4 = data.get<wstring>(L"name") + L"Developer";
    path base_dir = current_path();
    path polygon4_dir = base_dir / polygon4;
    path download_dir = base_dir / BOOTSTRAP_DOWNLOADS;

    create_directory(polygon4_dir);

    if (!git.empty())
        git_checkout(polygon4_dir, data.get<wstring>(L"git.url"));
    else
        manual_download_sources(data);
        
    download_files(download_dir, polygon4, data.get_child(L"data"));
    download_files(download_dir, polygon4, data.get_child(L"developer"));
    run_cmake(polygon4_dir);
    build_engine(polygon4_dir);
    create_project_files(polygon4_dir);
    build_project(polygon4_dir);

    PRINT("Bootstraped Polygon-4 successfully");

    return 0;
}