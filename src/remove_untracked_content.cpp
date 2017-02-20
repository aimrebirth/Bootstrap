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

#include <set>

#include "functional.h"

#include "logger.h"
DECLARE_STATIC_LOGGER(logger, "rm_untr_cont");

String bootstrap_programs_prefix = BOOTSTRAP_PREFIX;

int version()
{
    return UNTRACKED_CONTENT_DELETER_VERSION;
}

void print_version()
{
    LOG_INFO(logger, "Polygon-4 Developer Untracked Content Deleter Version " << version());
}

void check_version(int ver)
{
    if (ver == version())
        return;
    LOG_FATAL(logger, "FATAL ERROR:");
    LOG_FATAL(logger, "You have wrong version of the tool!");
    LOG_FATAL(logger, "Actual version: " << ver);
    LOG_FATAL(logger, "Your version: " << version());
    LOG_FATAL(logger, "Please, run BootstrapUpdater.exe to update the bootstrapper.");
    exit_program(1);
}

int bootstrap_module_main(int argc, char *argv[], const ptree &data)
{
    check_version(data.get<int>("tools.remove_untracked_content.version"));

    auto polygon4 = data.get<String>("name") + "Developer";
    auto base_dir = fs::current_path() / BOOTSTRAP_PREFIX;
    auto polygon4_dir = base_dir / polygon4;
    auto download_dir = base_dir / BOOTSTRAP_DOWNLOADS;

    fs::create_directory(polygon4_dir);

    printf("Do you REALLY want to DELETE ALL UNTRACKED FILES from developer content dir?");
    printf("This will remove any changes you did and any new files you created.");
    printf("Are you sure? (y/N)");
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty() || tolower(answer[0]) != 'y')
        return 0;

    remove_untracked(data.get_child("developer"), polygon4_dir, polygon4_dir / "Content");

    LOG_INFO(logger, "Removed untracked content developer files successfully");

    return 0;
}
