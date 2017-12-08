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

String bootstrap_programs_prefix;
path cmake = "cmake";

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
    if (exists(uproject))
    {
        LOG_INFO(logger, "Creating project files");
        execute_and_print({ bootstrap_programs_prefix + uvs, "/projectfiles", uproject.string() });
    }
}

void run_cmake(const path &dir)
{
    auto third_party = dir / "ThirdParty";
    auto swig_dir = third_party / "swig";
    auto swig_exe = swig_dir / "swig";
    auto tools_dir = third_party / "tools";
    auto src_dir = third_party / "Engine";
    auto bin_dir = src_dir / "Win64";
    auto sln_file = bin_dir / "Engine.sln";

    LOG_INFO(logger, "Running CPPAN");
    execute_and_print({ BOOTSTRAP_PROGRAMS "cppan", "-d", src_dir.string() });

    if (fs::exists(bin_dir / "CMakeCache.txt"))
        return;

    LOG_INFO(logger, "Running CMake");
    execute_and_print({ cmake.string(),
        "-H" + src_dir.string(),
        "-B" + bin_dir.string(),
        "-DSWIG_DIR=" + swig_dir.string(),
        "-DSWIG_EXECUTABLE=" + swig_exe.string(),
        "-DCPPAN_COMMAND=" + fs::absolute(BOOTSTRAP_PROGRAMS "cppan").string(),
        "-G", "Visual Studio 15 2017 Win64" });
    if (!exists(sln_file))
        check_return_code(1);
}

void build_engine(const path &dir)
{
    auto bin_dir = dir / "ThirdParty" / "Engine" / "Win64";
    auto sln_file = bin_dir / "Engine.sln";
    if (!exists(sln_file))
        return;
    LOG_INFO(logger, "Building Engine");
    execute_and_print({ cmake.string(), "--build", bin_dir.string(), "--config", "RelWithDebInfo" });
}

void build_project(const path &dir)
{
    auto msbuild = primitives::resolve_executable({
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\MSBuild\\15.0\\Bin\\MSBuild.exe",
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\MSBuild\\15.0\\Bin\\MSBuild.exe",
        "c:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Enterprise\\MSBuild\\15.0\\Bin\\MSBuild.exe" });
    if (!msbuild.empty())
    {
        auto sln = dir / "Polygon4.sln";
        LOG_INFO(logger, "Building Polygon4 Unreal project");
        execute_and_print({ msbuild.string(), sln.string(), "/p:Configuration=Development Editor", "/p:Platform=Win64", "/m" });
    }
}

void download_submodules()
{
    execute_and_print({ git.string(), "submodule", "update", "--init", "--recursive" });
}

void download_sources(const String &url)
{
    LOG_INFO(logger, "Downloading latest sources from Github repositories");
    if (!fs::exists(".git"))
    {
        fs::remove(".gitignore");
        fs::remove(".gitmodules");
        execute_and_print({ git.string(), "init" });
        execute_and_print({ git.string(), "remote", "add", "origin", url });
        execute_and_print({ git.string(), "fetch" });
        execute_and_print({ git.string(), "reset", "origin/master", "--hard" });
        execute_and_print({ git.string(), "submodule", "deinit", "-f", "." });
        download_submodules();
    }
    else
        execute_and_print({ git.string(), "clone", url, "." });
    download_submodules();
}

void update_sources()
{
    LOG_INFO(logger, "Updating latest sources from Github repositories");
    execute_and_print({ git.string(), "pull", "origin", "master" });
    download_submodules();
}

void git_checkout(const path &dir, const String &url)
{
    auto old_path = fs::current_path();
    if (!exists(dir))
        create_directories(dir);
    fs::current_path(dir);

    if (!exists(dir / ".git"))
        download_sources(url);
    else
        update_sources();

    fs::current_path(old_path);
}

int bootstrap_module_main(int argc, char *argv[], const ptree &data)
{
    init();
    check_version(data.get<int>("bootstrap.version"));

    // still init
    auto has_cmake = primitives::resolve_executable({
        cmake,
        R"(c:\Program Files\CMake\bin\cmake.exe)",
        R"(c:\Program Files (x86)\CMake\bin\cmake.exe)" });
    if (!has_cmake.empty())
        cmake = has_cmake;

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

    LOG_INFO(logger, "Downloading Third Party files...");
    download_files(download_dir, polygon4 / "ThirdParty", data.get_child("data.ThirdParty"));
    download_file("https://cppan.org/client/cppan-master-Windows-client.zip", BOOTSTRAP_DOWNLOADS + "cppan.zip"s);
    if (boost::trim_copy(download_file("https://cppan.org/client/cppan-master-Windows-client.zip.md5")) != md5(path(BOOTSTRAP_DOWNLOADS + "cppan.zip"s)))
    {
        LOG_ERROR(logger, "Bad md5 for cppan binary");
        return 1;
    }
    unpack_file(BOOTSTRAP_DOWNLOADS + "cppan.zip"s, BOOTSTRAP_PROGRAMS);

    LOG_INFO(logger, "Downloading main developer files...");
    download_files(download_dir, polygon4, data.get_child("developer"));

    if (!cmake.empty())
    {
        run_cmake(polygon4_dir);
        build_engine(polygon4_dir);
        create_project_files(polygon4_dir);
        build_project(polygon4_dir);
    }

    LOG_INFO(logger, "Bootstraped Polygon-4 Developer successfully");

    return 0;
}
