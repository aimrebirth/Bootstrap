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

#include <primitives/command.h>
#include <primitives/hash.h>
#include <primitives/http.h>
#include <primitives/pack.h>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "developer");

path bootstrap_programs_prefix;

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

void create_project_files(const path &dir)
{
    auto uproject = dir / "Polygon4.uproject";
    if (fs::exists(uproject))
    {
        LOG_INFO(logger, "Creating project files");
        execute_and_print({ (bootstrap_programs_prefix / uvs).u8string(), "/projectfiles", uproject.string() });
    }
}

void run_sw(const path &dir)
{
    auto third_party = dir / "ThirdParty";
    auto src_dir = third_party / "Engine";

    LOG_INFO(logger, "Running SW");
    execute_and_print({ (BOOTSTRAP_PROGRAMS / "sw").u8string(), "-d", src_dir.string(), "build" });
}

void build_project(const path &dir)
{
    auto msbuild = primitives::resolve_executable({
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",

        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\MSBuild\\15.0\\Bin\\MSBuild.exe",
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\MSBuild\\15.0\\Bin\\MSBuild.exe",
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Enterprise\\MSBuild\\15.0\\Bin\\MSBuild.exe",
        });
    if (!msbuild.empty())
    {
        auto sln = dir / "Polygon4.sln";
        LOG_INFO(logger, "Building Polygon4 Unreal project");
        execute_and_print({ msbuild.string(), sln.string(), "/p:Configuration=Development Editor", "/p:Platform=Win64", "/m" });
    }
}

static Strings git_command(const path &dir, const Strings &args)
{
    Strings s;
    s.push_back(git.u8string());
    s.push_back("-C");
    s.push_back(dir.u8string());
    s.insert(s.end(), args.begin(), args.end());
    return s;
}

void download_submodules(const path &dir)
{
    execute_and_print(git_command(dir, { "submodule", "update", "--init", "--recursive" }));
}

void download_sources(const String &url, const path &dir)
{
    LOG_INFO(logger, "Downloading latest sources from Github repositories");
    if (!fs::exists(dir / ".git"))
    {
        fs::remove(dir / ".gitignore");
        fs::remove(dir / ".gitmodules");
        execute_and_print(git_command(dir, { "init" }));
        execute_and_print(git_command(dir, { "remote", "add", "origin", url }));
        execute_and_print(git_command(dir, { "fetch" }));
        execute_and_print(git_command(dir, { "reset", "origin/master", "--hard" }));
        execute_and_print(git_command(dir, { "submodule", "deinit", "-f", "." }));
        download_submodules(dir);
    }
    else
        execute_and_print(git_command(dir, { "clone", url, "." }));
    download_submodules(dir);
}

void update_sources(const path &dir)
{
    LOG_INFO(logger, "Updating latest sources from Github repositories");
    execute_and_print(git_command(dir, { "pull", "origin", "master" }));
    download_submodules(dir);
}

void git_checkout(const path &dir, const String &url)
{
    if (!fs::exists(dir))
        create_directories(dir);

    if (!fs::exists(dir / ".git"))
        download_sources(url, dir);
    else
        update_sources(dir);
}

int bootstrap_module_main(int argc, char *argv[], const ptree &data)
{
    init();
    check_version(data.get<int>("bootstrap.version"));

    //
    auto polygon4 = path(data.get<String>("name") + "Developer");
    path base_dir = fs::current_path();
    path polygon4_dir = base_dir / polygon4;
    path download_dir = base_dir / BOOTSTRAP_DOWNLOADS;

    fs::create_directory(polygon4_dir);

    git = primitives::resolve_executable(git);
    if (!git.empty())
    {
        for (const auto &repo : data.get_child("git"))
            git_checkout(polygon4_dir / repo.second.get<String>("dir"), repo.second.get<String>("url"));
    }
    else
    {
        LOG_WARN(logger, "Git was not found. Trying to download files manually.");
        for (const auto &repo : data.get_child("git"))
            manual_download_sources(polygon4_dir / repo.second.get<String>("dir"), repo.second);
    }

    static auto sw_url = "https://software-network.org/client/sw-master-windows-client.zip"s;
    auto get_remote_md5 = []()
    {
        static auto md5 = boost::trim_copy(download_file(sw_url + ".md5"));
        return md5;
    };

    auto unpack_exe = [&get_remote_md5]()
    {
        unpack_file(BOOTSTRAP_DOWNLOADS / "sw.zip"s, BOOTSTRAP_PROGRAMS);
        if (!fs::exists(BOOTSTRAP_DOWNLOADS / "sw.zip"s) || get_remote_md5() != md5(BOOTSTRAP_DOWNLOADS / "sw.zip"s))
        {
            throw SW_RUNTIME_ERROR("Bad md5 for sw binary");
        }
    };

    if (!fs::exists(BOOTSTRAP_DOWNLOADS / "sw.zip"s) || get_remote_md5() != md5(BOOTSTRAP_DOWNLOADS / "sw.zip"s))
    {
        download_file(sw_url, BOOTSTRAP_DOWNLOADS / "sw.zip"s);
        unpack_exe();
    }
    if (!fs::exists(BOOTSTRAP_PROGRAMS / "sw.exe"s))
        unpack_exe();

    LOG_INFO(logger, "Downloading main developer files...");
    download_files(download_dir, polygon4, data.get_child("developer"));

    run_sw(polygon4_dir);
    create_project_files(polygon4_dir);
    build_project(polygon4_dir);

    LOG_INFO(logger, "Bootstraped Polygon-4 Developer successfully");

    return 0;
}
