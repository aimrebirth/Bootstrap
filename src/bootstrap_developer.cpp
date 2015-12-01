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

DECLARE_STATIC_LOGGER(logger, "developer");

String bootstrap_programs_prefix;

int version()
{
    return BOOTSTRAPPER_VERSION;
}

void print_version()
{
    LOG_INFO(logger, "Polygon-4 Developer Bootstrapper Version " << version());
}

void check_version(int ver)
{
    if (ver == version())
        return;
    LOG_FATAL(logger, "You have wrong version of bootstrapper!");
    LOG_FATAL(logger, "Actual version: " << ver);
    LOG_FATAL(logger, "Your version: " << version());
    LOG_FATAL(logger, "Please, run BootstrapUpdater.exe to update the bootstrapper.");
    exit_program(1);
}

int bootstrap_module_main(int argc, char *argv[], const ptree &data)
{
    init();
    check_version(data.get<int>(L"bootstrap.version"));

    auto polygon4 = path(data.get<String>(L"name") + L"Developer");
    path base_dir = fs::current_path();
    path polygon4_dir = base_dir / polygon4;
    path download_dir = base_dir / BOOTSTRAP_DOWNLOADS;

    fs::create_directory(polygon4_dir);

    if (!git.empty())
    {
        for (const auto &repo : data.get_child(L"git"))
            git_checkout(polygon4_dir / repo.second.get<String>(L"dir"), repo.second.get<String>(L"url"));
    }
    else
    {
        LOG_WARN(logger, "Git was not found. Trying to download files manually.");
        for (const auto &repo : data.get_child(L"git"))
            manual_download_sources(polygon4_dir / repo.second.get<String>(L"dir"), repo.second);
    }

    LOG_INFO(logger, "Downloading Third Party files...");
    download_files(download_dir, polygon4 / "ThirdParty", data.get_child(L"data"));
    LOG_INFO(logger, "Downloading main developer files...");
    download_files(download_dir, polygon4, data.get_child(L"developer"));
    run_cmake(polygon4_dir);
    build_engine(polygon4_dir);
    create_project_files(polygon4_dir);
    build_project(polygon4_dir);

    LOG_INFO(logger, "Bootstraped Polygon-4 Developer successfully");

    return 0;
}
