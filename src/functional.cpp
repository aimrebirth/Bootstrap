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

#include <codecvt>
#include <locale>

#include <boost/asio/io_service.hpp>

#include "functional.h"

#include <boost/thread.hpp>

//
// global data
//

extern wstring bootstrap_programs_prefix;

wstring git = L"git";
wstring cmake = L"cmake";
wstring msbuild = L"c:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MSBuild.exe";

//
// helper functions
//

std::string to_string(std::wstring s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(s.c_str());
}

std::wstring to_wstring(std::string s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(s.c_str());
}

//
// function definitions
//

void create_project_files(const wpath &dir)
{
    auto sln = dir / "Polygon4.sln";
    auto uproject = dir / "Polygon4.uproject";
    if (/*!exists(sln) && */exists(uproject))
    {
        PRINT("Creating project files");
        execute_command({ bootstrap_programs_prefix + uvs, L"/projectfiles", uproject.wstring() });
        SPACE();
    }
}

void build_project(const wpath &dir)
{
    auto sln = dir / "Polygon4.sln";
    PRINT("Building Polygon4 Unreal project");
    SPACE();
    execute_command({ msbuild, sln.wstring(), L"/p:Configuration=Development Editor", L"/p:Platform=Win64", L"/m" });
    SPACE();
}

void build_engine(const wpath &dir)
{
    auto bin_dir = dir / "ThirdParty" / "Engine" / "Win64";
    auto sln_file = bin_dir / "Engine.sln";
    if (!exists(sln_file))
        return;
    PRINT("Building Engine");
    SPACE();
    execute_command({ cmake, L"--build", bin_dir.wstring(), L"--config", L"RelWithDebInfo" });
    SPACE();
}

void run_cmake(const wpath &dir)
{
    auto third_party = dir / "ThirdParty";
    auto swig_dir = third_party / "swig";
    auto swig_exe = swig_dir / "swig";
    auto tools_dir = third_party / "tools";
    auto bison_exe = tools_dir / "bison";
    auto flex_exe = tools_dir / "flex";
    auto boost_dir = third_party / "boost";
    auto boost_lib_dir = boost_dir / "lib64-msvc-14.0";
    auto src_dir = third_party / "Engine";
    auto bin_dir = src_dir / "Win64";
    auto sln_file = bin_dir / "Engine.sln";
    if (exists(sln_file))
        return;
    if (cmake.empty())
        return;
    PRINT("Running CMake");
    execute_command({ cmake,
        L"-H" + src_dir.wstring(),
        L"-B" + bin_dir.wstring(),
        L"-DDATA_MANAGER_DIR=../DataManager",
        L"-DBOOST_ROOT=" + boost_dir.wstring(),
        L"-DBOOST_LIBRARYDIR=" + boost_lib_dir.wstring(),
        L"-DSWIG_DIR=" + swig_dir.wstring(),
        L"-DSWIG_EXECUTABLE=" + swig_exe.wstring(),
        L"-DBISON_EXECUTABLE=" + bison_exe.wstring(),
        L"-DFLEX_EXECUTABLE=" + flex_exe.wstring(),
        L"-G", L"Visual Studio 14 Win64" });
    if (!exists(sln_file))
        check_return_code(1);
    SPACE();
}

void download_files(const wpath &dir, const wpath &output_dir, const pt::wptree &data)
{
    wstring redirect = data.get(L"redirect", L"");
    if (!redirect.empty())
    {
        auto data2 = load_data(redirect);
        download_files(dir, output_dir, data2);
        return;
    }

    // parallel curl in case of noncompressed files
    boost::asio::io_service io_service;
    boost::thread_group threadpool;
    boost::asio::io_service::work work(io_service);
    for (int i = 0; i < 10; i++)
        threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));

    wstring file_prefix = data.get(L"file_prefix", L"");
    const pt::wptree &files = data.get_child(L"files");
    for (auto &repo : files)
    {
        auto url = repo.second.get<wstring>(L"url");
        auto name = repo.second.get<wstring>(L"name", L"");
        auto file = (dir / file_prefix).wstring() + name;
        auto hash = repo.second.get<wstring>(L"md5", L"");
        auto check_path = repo.second.get(L"check_path", L"");
        bool packed = repo.second.get<bool>(L"packed", false);
        if (!packed)
        {
            file = (output_dir / check_path).wstring();
            if (!exists(file) || md5(file) != hash)
            {
                create_directories(wpath(file).parent_path());
                // create empty files
                ofstream ofile(file);
                if (ofile)
                    ofile.close();
                io_service.post([file, url](){ download(url, file, D_NO_SPACE); });
            }
            continue;
        }
        if (!exists(file) || md5(file) != hash)
        {
            download(url, file);
            if (md5(file) != hash)
            {
                PRINT("Wrong file is located on server! Cannot proceed.");
                exit_program(1);
            }
            unpack(file, output_dir.wstring());
        }
        if (!check_path.empty() && !exists(output_dir / check_path))
        {
            unpack(file, output_dir.wstring());
        }
    }
    io_service.stop();
    threadpool.join_all();
}

void init()
{
    create_directory(BOOTSTRAP_DOWNLOADS);
    create_directory(BOOTSTRAP_PROGRAMS);

    has_program_in_path(git);
    has_program_in_path(cmake);
}

void unpack(const wstring &file, const wstring &output_dir, bool exit_on_error)
{
    PRINT("Unpacking file: " << file);
    execute_command({ bootstrap_programs_prefix + _7z, L"x", L"-y", L"-o" + output_dir, file}, exit_on_error);
    SPACE();
}

bool copy_dir(const wpath &source, const wpath &destination)
{
    namespace fs = boost::filesystem;
    try
    {
        if (!fs::exists(source) || !fs::is_directory(source))
        {
            std::cerr << "Source directory " << source.string()
                << " does not exist or is not a directory." << '\n';
            return false;
        }
        fs::create_directory(destination);
    }
    catch (fs::filesystem_error const & e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
    for (fs::directory_iterator file(source); file != fs::directory_iterator(); ++file)
    {
        try
        {
            fs::wpath current(file->path());
            if (fs::is_directory(current))
            {
                if (!copy_dir( current, destination / current.filename()))
                    return false;
            }
            else
            {
                fs::copy_file(current, destination / current.filename(), fs::copy_option::overwrite_if_exists);
            }
        }
        catch (fs::filesystem_error const & e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    return true;
}

void manual_download_sources(const wpath &dir, const pt::wptree &data)
{
    auto url = data.get<wstring>(L"url_zip");
    auto suffix = data.get<wstring>(L"suffix");
    auto name = data.get<wstring>(L"name");
    auto file = wpath(BOOTSTRAP_DOWNLOADS) / (name + L"-" + suffix + ZIP_EXT);
    download(url, file.wstring());
    unpack(file.wstring(), BOOTSTRAP_DOWNLOADS);
    copy_dir(wpath(BOOTSTRAP_DOWNLOADS) / (name + L"-master"), dir);
}

void download_submodules()
{
    execute_command({git, L"submodule", L"update", L"--init", L"--recursive"});
}

void download_sources(const wstring &url)
{
    PRINT("Downloading latest sources from Github repositories");
    execute_command({git, L"clone", url, L"."});
    if (!exists(".git"))
    {
        execute_command({git, L"init"});
        execute_command({git, L"remote", L"add", L"origin", url});
        execute_command({git, L"fetch"});
        execute_command({git, L"reset", L"origin/master", L"--hard"});
    }
    download_submodules();
    SPACE();
}

void update_sources()
{
    PRINT("Updating latest sources from Github repositories");
    execute_command({git, L"pull", L"origin", L"master"});
    download_submodules();
    SPACE();
}

void git_checkout(const wpath &dir, const wstring &url)
{
    auto old_path = current_path();
    if (!exists(dir))
        create_directories(dir);
    current_path(dir);

    if (!exists(dir / ".git"))
        download_sources(url);
    else
        update_sources();

    current_path(old_path);
}

bool has_program_in_path(wstring &prog)
{
    bool ret = true;
    try
    {
        prog = to_wstring(find_executable_in_path(to_string(prog)));
    }
    catch (filesystem_error)
    {
        PRINT("Warning: \"" << prog << "\" is missing in your wpath environment variable");
        SPACE();
        ret = false;
    }
    return ret;
}

pt::wptree load_data(const wstring &url)
{
    auto s = download(url);
    pt::wptree pt;
    wstringstream ss(to_string(s));
    CATCH(
        pt::json_parser::read_json(ss, pt),
        pt::json_parser_error,
        "Json file: " << url << " has errors in its structure!" << "\n"
        "Please, report to the author."
        );
    return pt;
}

Bytes download(const wstring &url)
{
    PRINT("Downloading file: " << url);
    wstring file = (temp_directory_path() / "polygon4_bootstrap_temp_file").wstring();
    download(url, file, D_SILENT);
    return read_file(file);
}

Bytes read_file(const wstring &file)
{
    auto size = file_size(file);
    FILE *f = fopen(to_string(file).c_str(), "rb");
    if (!f)
    {
        PRINT("Cannot open file: " << file);
        check_return_code(1);
    }
    Bytes bytes(size, 0);
    fread(bytes.data(), 1, size, f);
    fclose(f);
    return bytes;
}

void download(const wstring &url, const wstring &file, int flags)
{
    if (file.empty())
    {
        PRINT("file is empty!");
        exit_program(1);
    }
    if (!(flags & D_SILENT))
        PRINT("Downloading file: " << file);
    execute_command({ bootstrap_programs_prefix + curl, L"-L", L"-k", (flags & D_CURL_SILENT) ? L"-s" : L"-#", L"-o" + file, url });
    if (!(flags & D_NO_SPACE))
        SPACE();
}

SubprocessAnswer execute_command(WStrings args, bool exit_on_error, stream_behavior stdout_behavior)
{
    if (args[0].rfind(L".exe") != args[0].size() - 4)
        args[0] += L".exe";
    boost::algorithm::replace_all(args[0], "/", "\\");

    std::wstring exec = absolute(args[0]).wstring();

    Strings args_s;
    for (auto &a : args)
        args_s.push_back(to_string(a));

    context ctx;
    ctx.stdout_behavior = stdout_behavior;
    ctx.stderr_behavior = inherit_stream();
    
    SubprocessAnswer answer;

    try
    {
        child c = boost::process::launch(to_string(exec), args_s, ctx);
        int ret = c.wait().exit_status();
        if (ret && exit_on_error)
            check_return_code(ret);

        answer.ret = ret;

        if (stdout_behavior.get_type() == capture_stream().get_type())
        {
            Bytes buf(1 << 20, 0);
            pistream &stream = c.get_stdout();
            while (stream)
            {
                stream.read((char *)buf.data(), buf.size());
                size_t read = stream.gcount();
                answer.bytes.insert(answer.bytes.end(), buf.begin(), buf.begin() + read);
            }
            stream.close();
        }
    }
    catch (...)
    {
        PRINT("Cannot run command: " << exec);
        throw;
    }

    return answer;
}

void check_return_code(int code)
{
    if (!code)
        return;
    SPACE();
    PRINT("Last bootstrap step failed");
    exit_program(code);
}

void exit_program(int code)
{
    SPACE();
    PRINT("Press Enter to continue...");
    getchar();
    exit(1);
}

wstring to_string(const Bytes &b)
{
    return wstring(b.begin(), b.end());
}

int main(int argc, char *argv[])
try
{
    // parse cmd
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            char *arg = argv[i];
            if (*arg != '-')
                continue;
            if (strcmp(arg, "--copy") == 0)
            {
                char *dst = argv[++i];
                copy_file(argv[0], dst, copy_option::overwrite_if_exists);
                PRINT("Update successful.");
                return 0;
            }
            else if (strcmp(arg, "--version") == 0)
            {
                print_version();
                return 0;
            }
            else
            {
                PRINT("Unknown option: " << arg);
                return 1;
            }
        }
    }

    print_version();

    auto data = load_data(BOOTSTRAP_JSON_URL);

    bootstrap_module_main(argc, argv, data);

    return 0;
}
catch (std::exception &e)
{
    PRINT("Error " << e.what());
    check_return_code(1);
}
catch (...)
{
    PRINT("FATAL ERROR: Unkown exception!");
    return 1;
}