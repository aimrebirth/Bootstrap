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

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "tools");

path bootstrap_programs_prefix;

int version()
{
    return BOOTSTRAPPER_TOOLS;
}

void print_version()
{
    LOG_INFO(logger, "Polygon-4 Tools Bootstrapper Version " << version());
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
    check_version(data.get<int>("bootstrap.tools.version"));

    path base_dir = fs::current_path();
    path download_dir = base_dir / BOOTSTRAP_DOWNLOADS;

    download_files(download_dir, fs::current_path(), data.get_child("tools"));

    LOG_INFO(logger, "Bootstraped Polygon-4 Tools successfully");

    return 0;
}
