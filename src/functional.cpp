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

#include <boost/asio/io_service.hpp>

#include "functional.h"

#include <boost/thread.hpp>

//
// global data
//

string git = "git";
string cmake = "cmake";
string msbuild = "C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\MSBuild.exe";

//
// function definitions
//

void create_project_files(path dir)
{
    auto sln = dir / "Polygon4.sln";
    auto uproject = dir / "Polygon4.uproject";
    if (!exists(sln) && exists(uproject))
    {
        PRINT("Creating project files");
        execute_command({ uvc, "/projectfiles", uproject.string() });
        SPACE();
    }
}

void build_project(path dir)
{
    auto sln = dir / "Polygon4.sln";
    PRINT("Building Polygon4 Unreal project");
    SPACE();
    execute_command({ msbuild, sln.string(), "/property:Configuration=Development Editor", "/property:Platform=Windows", "/m" });
    SPACE();
}

void build_engine(path dir)
{
    auto bin_dir = dir / "ThirdParty" / "Engine" / "Win64";
    auto sln_file = bin_dir / "Engine.sln";
    if (!exists(sln_file))
        return;
    PRINT("Building Engine");
    SPACE();
    execute_command({ cmake, "--build", bin_dir.string(), "--config", "RelWithDebInfo" });
    SPACE();
}

void run_cmake(path dir)
{
    auto third_party = dir / "ThirdParty";
    auto swig_dir = third_party / "swig";
    auto swig_exe = swig_dir / "swig";
    auto boost_dir = third_party / "boost";
    auto boost_lib_dir = boost_dir / "lib64-msvc-12.0";
    auto src_dir = third_party / "Engine";
    auto bin_dir = src_dir / "Win64";
    auto sln_file = bin_dir / "Engine.sln";
    if (exists(sln_file))
        return;
    if (cmake.empty())
        return;
    PRINT("Running CMake");
    execute_command({ cmake, "-H" + src_dir.string(),
        "-B" + bin_dir.string(),
        "-DBOOST_ROOT=" + boost_dir.string(),
        "-DBOOST_LIBRARYDIR=" + boost_lib_dir.string(),
        "-DSWIG_DIR=" + swig_dir.string(),
        "-DSWIG_EXECUTABLE=" + swig_exe.string(),
        "-G", "Visual Studio 12 Win64" });
    if (!exists(sln_file))
        check_return_code(1);
    SPACE();
}

void download_files(path dir, path output_dir, const pt::ptree &data)
{
    string redirect = data.get("redirect", "");
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

    string file_prefix = data.get("file_prefix", "");
    const pt::ptree &files = data.get_child("files");
    for (auto &repo : files)
    {
        string url = repo.second.get<string>("url");
        string name = repo.second.get<string>("name", "");
        string file = (dir / file_prefix).string() + name;
        string hash = repo.second.get<string>("md5", "");
        string check_path = repo.second.get("check_path", "");
        bool packed = repo.second.get<bool>("packed", false);
        if (!packed)
        {
            file = (output_dir / check_path).string();
            if (!exists(file) || md5(file) != hash)
            {
                create_directories(path(file).parent_path());
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
            unpack(file, output_dir.string());
        }
        if (!check_path.empty() && !exists(output_dir / check_path))
        {
            unpack(file, output_dir.string());
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

void unpack(string file, string output_dir, bool exit_on_error)
{
    PRINT("Unpacking file: " << file);
    execute_command({_7z, "x", "-y", "-o" + output_dir, file}, exit_on_error);
    SPACE();
}

bool copy_dir(const path &source, const path &destination)
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
            fs::path current(file->path());
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

void manual_download_sources(const pt::ptree &data)
{
    string format = data.get<string>("git.url_format");
    string master_suffix = data.get<string>("git.master_suffix");
    for (auto &repo : data.get_child("sources.repositories"))
    {
        string name = repo.second.get<string>("name");
        string url = (boost::format(format.c_str()) % name).str();
        string file = BOOTSTRAP_DOWNLOADS + name + master_suffix + ZIP_EXT;
        download(url, file);
    }
    for (auto &repo : data.get_child("sources.repositories"))
    {
        string name = repo.second.get<string>("name");
        string dir = BOOTSTRAP_DOWNLOADS + name + master_suffix;
        string file = dir + ZIP_EXT;
        unpack(file, BOOTSTRAP_DOWNLOADS);

        string src = dir;
        string dst = data.get<string>("name");
        if (name != dst)
            dst += "/" + repo.second.get<string>("unpack_dir") + "/" + name;
        copy_dir(src, dst);
    }
}

void download_submodules()
{
    execute_command({git, "submodule", "update", "--init", "--recursive"});
}

void download_sources(string url)
{
    PRINT("Downloading latest sources from Github repositories");
    execute_command({git, "clone", url, "."});
    if (!exists(".git"))
    {
        execute_command({git, "init"});
        execute_command({git, "remote", "add", "origin", url});
        execute_command({git, "fetch"});
        execute_command({git, "reset", "origin/master", "--hard"});
    }
    download_submodules();
    SPACE();
}

void update_sources()
{
    PRINT("Updating latest sources from Github repositories");
    execute_command({git, "pull", "origin", "master"});
    download_submodules();
    SPACE();
}

void git_checkout(path dir, string url)
{
    auto old_path = current_path();
    current_path(dir);

    if (!exists(dir / ".git"))
        download_sources(url);
    else
        update_sources();

    current_path(old_path);
}

bool has_program_in_path(string &prog)
{
    bool ret = true;
    try
    {
        prog = find_executable_in_path(prog);
    }
    catch (filesystem_error)
    {
        PRINT("Warning: \"" << prog << "\" is missing in your PATH environment variable");
        SPACE();
        ret = false;
    }
    return ret;
}

pt::ptree load_data(string url)
{
    auto s = download(url);
    pt::ptree pt;
    stringstream ss(to_string(s));
    CATCH(
        pt::json_parser::read_json(ss, pt),
        pt::json_parser_error,
        "Json file: " << url << " has errors in its structure!" << "\n"
        "Please, report to the author."
        );
    return pt;
}

Bytes download(string url)
{
    PRINT("Downloading file: " << url);
    string file = (temp_directory_path() / "polygon4_bootstrap_temp_file").string();
    download(url, file, D_SILENT);
    return read_file(file);
}

Bytes read_file(string file)
{
    auto size = file_size(file);
    FILE *f = fopen(file.c_str(), "rb");
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

void download(string url, string file, int flags)
{
    if (file.empty())
    {
        PRINT("file is empty!");
        exit_program(1);
    }
    if (!(flags & D_SILENT))
        PRINT("Downloading file: " << file);
    execute_command({ curl, "-L", "-k", (flags & D_CURL_SILENT) ? "-s" : "-#", "-o" + file, url });
    if (!(flags & D_NO_SPACE))
        SPACE();
}

SubprocessAnswer execute_command(Strings args, bool exit_on_error, stream_behavior stdout_behavior)
{
    if (args[0].rfind(".exe") != args[0].size() - 4)
        args[0] += ".exe";
    boost::algorithm::replace_all(args[0], "/", "\\");

    std::string exec = absolute(args[0]).string();

    context ctx;
    ctx.stdout_behavior = stdout_behavior;
    ctx.stderr_behavior = inherit_stream();
    
    SubprocessAnswer answer;

    try
    {
        child c = boost::process::launch(exec, args, ctx);
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

string to_string(const Bytes &b)
{
    return string(b.begin(), b.end());
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

    init();
    print_version();

    auto data = load_data(BOOTSTRAP_JSON_URL);

    bootstrap_module_main(argc, argv, data);

    return 0;
}
catch (std::exception e)
{
    PRINT("Error " << e.what());
    check_return_code(1);
}
catch (...)
{
    PRINT("FATAL ERROR: Unkown exception!");
    return 1;
}