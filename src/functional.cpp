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

#include <atomic>
#include <codecvt>
#include <locale>
#include <mutex>
#include <unordered_map>

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

#include "functional.h"

DECLARE_STATIC_LOGGER(logger, "core");

//
// global data
//

extern String bootstrap_programs_prefix;

String git = L"git";
String cmake = L"cmake";
String msbuild = L"c:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MSBuild.exe";

//
// helper functions
//

std::string to_string(std::wstring s)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(s.c_str());
}

std::wstring to_wstring(std::string s)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(s.c_str());
}

//
// function definitions
//

void create_project_files(const path &dir)
{
    auto sln = dir / "Polygon4.sln";
    auto uproject = dir / "Polygon4.uproject";
    if (/*!exists(sln) && */exists(uproject))
    {
        LOG_INFO(logger, "Creating project files");
        execute_and_print({ bootstrap_programs_prefix + uvs, L"/projectfiles", uproject.wstring() });
    }
}

void build_project(const path &dir)
{
    auto sln = dir / "Polygon4.sln";
    LOG_INFO(logger, "Building Polygon4 Unreal project");
    execute_and_print({ msbuild, sln.wstring(), L"/p:Configuration=Development Editor", L"/p:Platform=Win64", L"/m" });
}

void build_engine(const path &dir)
{
    auto bin_dir = dir / "ThirdParty" / "Engine" / "Win64";
    auto sln_file = bin_dir / "Engine.sln";
    if (!exists(sln_file))
        return;
    LOG_INFO(logger, "Building Engine");
    execute_and_print({ cmake, L"--build", bin_dir.wstring(), L"--config", L"RelWithDebInfo" });
}

void run_cmake(const path &dir)
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
    LOG_INFO(logger, "Running CMake");
    execute_and_print({ cmake,
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
}

void download_files(const path &dir, const path &output_dir, const ptree &data)
{
    String redirect = data.get(L"redirect", L"");
    if (!redirect.empty())
    {
        auto data2 = load_data(redirect);
        download_files(dir, output_dir, data2);
        return;
    }
    
    std::atomic_int errors;

    auto work = [&]()
    {
        boost::asio::io_service io_service;
        boost::thread_group threadpool;
        auto work = std::make_unique<boost::asio::io_service::work>(io_service);
        for (int i = 0; i < 8; i++)
            threadpool.create_thread([&io_service, &errors]()
        {
            while (1)
            {
                try
                {
                    io_service.run();
                }
                catch (...)
                {
                    errors++;
                    continue;
                }
                break;
            }
        });

        // create last write time file catalog
        auto lwt_file = path(BOOTSTRAP_DOWNLOADS) / LAST_WRITE_TIME_DATA;
        if (!std::ifstream(lwt_file.string()))
            std::ofstream(lwt_file.string()) << R"({})";
        auto lwt_data = load_data(lwt_file);
        std::mutex lwt_mutex;

        String file_prefix = data.get(L"file_prefix", L"");
        const ptree &files = data.get_child(L"files");
        for (auto &repo : files)
        {
            io_service.post([&repo, &dir, &file_prefix, &output_dir, &lwt_data, &lwt_mutex]()
            {
                auto url = repo.second.get<String>(L"url");
                auto name = repo.second.get<String>(L"name", L"");
                auto file = (dir / file_prefix).wstring() + name;
                auto new_hash_md5 = repo.second.get<String>(L"md5", L"");
                auto check_path = repo.second.get(L"check_path", L"");
                bool packed = repo.second.get<bool>(L"packed", false);
                String old_file_md5;
                if (!packed)
                {
                    file = (output_dir / check_path).wstring();
                    bool file_exists = fs::exists(file);
                    if (file_exists)
                    {
                        int file_lwt = fs::last_write_time(file);
                        if (lwt_data.count(file))
                        {
                            auto &file_lwt_data_main = lwt_data.find(file)->second;
                            auto &file_lwt_data = file_lwt_data_main.get_child(L"lwt");
                            old_file_md5 = file_lwt_data_main.get(L"md5", String());

                            int old_file_lwt = file_lwt_data.get_value<int>(0);
                            if (old_file_lwt == 0)
                            {
                                old_file_lwt = file_lwt;
                            }
                            else if (old_file_lwt == file_lwt)
                            {
                                if (old_file_md5 == new_hash_md5)
                                    return;
                                else if (old_file_md5.empty())
                                {
                                    // no md5 was calculated before
                                    old_file_md5 = to_wstring(md5(file));
                                }
                            }
                            file_lwt_data.put_value(file_lwt);
                            file_lwt_data_main.put(L"md5", new_hash_md5);
                        }
                        else
                        {
                            // no md5 was calculated before
                            old_file_md5 = to_wstring(md5(file));
                            if (old_file_md5 == new_hash_md5)
                            {
                                ptree value;
                                value.add(L"lwt", file_lwt);
                                value.add(L"md5", old_file_md5);

                                std::lock_guard<std::mutex> g(lwt_mutex);
                                lwt_data.add_child(ptree::path_type(file, '|'), value);
                            }
                        }
                    }
                    if (!file_exists || old_file_md5 != new_hash_md5)
                    {
                        create_directories(path(file).parent_path());
                        // create empty files
                        std::ofstream ofile(file);
                        if (ofile)
                            ofile.close();
                        download(url, file, D_NO_SPACE);

                        // file is changed in previous download, so md5 is of new file,
                        //  not same in condition above!
                        auto new_file_md5 = to_wstring(md5(file));

                        // recheck hash
                        if (new_file_md5 != new_hash_md5)
                        {
                            LOG_FATAL(logger, "Wrong file is located on server! Cannot proceed.");
                            exit_program(1);
                        }

                        int file_lwt = fs::last_write_time(file);
                        ptree value;
                        value.add(L"lwt", file_lwt);
                        value.add(L"md5", new_file_md5);

                        std::lock_guard<std::mutex> g(lwt_mutex);
                        lwt_data.add_child(ptree::path_type(file, '|'), value);
                    }
                    return;
                }
                if (!fs::exists(file) || md5(file) != to_string(new_hash_md5))
                {
                    download(url, file);
                    // file is changed in previous download, so md5 is of new file,
                    //  not same in condition above!
                    // recheck hash
                    if (md5(file) != to_string(new_hash_md5))
                    {
                        LOG_FATAL(logger, "Wrong file is located on server! Cannot proceed.");
                        exit_program(1);
                    }
                    unpack(file, output_dir.wstring());
                }
                if (!check_path.empty() && !exists(output_dir / check_path))
                {
                    unpack(file, output_dir.wstring());
                }
            });
        }

        // work
        work.reset();
        threadpool.join_all();

        pt::write_json(lwt_file.string(), lwt_data);
    };

    const int max_attempts = 3;
    int attempts = 0;
    while (1)
    {
        if (attempts == max_attempts)
            break;
        errors = 0;
        work();
        if (errors == 0)
            break;
        LOG_ERROR(logger, "Download files ended with " << errors << " error(s)");
        LOG_ERROR(logger, "Retrying (" << ++attempts << ") ...");
    }
}

void init()
{
    fs::create_directory(BOOTSTRAP_DOWNLOADS);
    fs::create_directory(BOOTSTRAP_PROGRAMS);

    has_program_in_path(git);
    has_program_in_path(cmake);
}

void unpack(const String &file, const String &output_dir, bool exit_on_error)
{
    LOG_DEBUG(logger, "Unpacking file: " << file);
    execute_and_print({ bootstrap_programs_prefix + _7z, L"x", L"-y", L"-o" + output_dir, file}, exit_on_error);
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
            path current(file->path());
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

void manual_download_sources(const path &dir, const ptree &data)
{
    auto url = data.get<String>(L"url_zip");
    auto suffix = data.get<String>(L"suffix");
    auto name = data.get<String>(L"name");
    auto file = path(BOOTSTRAP_DOWNLOADS) / (name + L"-" + suffix + ZIP_EXT);
    download(url, file.wstring());
    unpack(file.wstring(), BOOTSTRAP_DOWNLOADS);
    copy_dir(path(BOOTSTRAP_DOWNLOADS) / (name + L"-master"), dir);
}

void download_submodules()
{
    execute_and_print({git, L"submodule", L"update", L"--init", L"--recursive"});
}

void download_sources(const String &url)
{
    LOG_INFO(logger, "Downloading latest sources from Github repositories");
    execute_and_print({git, L"clone", url, L"."});
    if (!fs::exists(".git"))
    {
        execute_and_print({git, L"init"});
        execute_and_print({git, L"remote", L"add", L"origin", url});
        execute_and_print({git, L"fetch"});
        execute_and_print({git, L"reset", L"origin/master", L"--hard"});
    }
    download_submodules();
}

void update_sources()
{
    LOG_INFO(logger, "Updating latest sources from Github repositories");
    execute_and_print({git, L"pull", L"origin", L"master"});
    download_submodules();
}

void git_checkout(const path &dir, const String &url)
{
    auto old_path = fs::current_path();
    if (!exists(dir))
        create_directories(dir);
    current_path(dir);

    if (!exists(dir / ".git"))
        download_sources(url);
    else
        update_sources();

    current_path(old_path);
}

bool has_program_in_path(String &prog)
{
    bool ret = true;
    try
    {
        prog = to_wstring(boost::process::find_executable_in_path(to_string(prog)));
    }
    catch (fs::filesystem_error &e)
    {
        LOG_WARN(logger, "'" << prog << "' is missing in your wpath environment variable. Error: " << e.what());
        ret = false;
    }
    return ret;
}

ptree load_data(const String &url)
{
    static std::unordered_map<String, ptree> cached;
    if (cached.count(url))
        return cached[url];

    auto s = download(url);
    ptree pt;
    std::wstringstream ss(to_string(s));
    try
    {
        pt::json_parser::read_json(ss, pt);
    }
    catch (pt::json_parser_error &e)
    {
        LOG_ERROR(logger, "Json file: " << url << " has errors in its structure!");
        LOG_ERROR(logger, "Please, report to the author.");
        throw;
    }
    return cached[url] = pt;
}

ptree load_data(const path &dir)
{
    ptree pt;
    try
    {
        pt::json_parser::read_json(dir.string(), pt);
    }
    catch (pt::json_parser_error &e)
    {
        LOG_ERROR(logger, "Json file: " << dir.string() << " has errors in its structure!");
        LOG_ERROR(logger, "Please, report to the author.");
        throw;
    }
    return pt;
}

Bytes read_file(const String &file)
{
    auto size = fs::file_size(file);
    FILE *f = fopen(to_string(file).c_str(), "rb");
    if (!f)
    {
        LOG_FATAL(logger, "Cannot open file: " << file);
        check_return_code(1);
    }
    Bytes bytes(size, 0);
    fread(bytes.data(), 1, size, f);
    fclose(f);
    return bytes;
}

Bytes download(const String &url)
{
    LOG_INFO(logger, "Downloading file: " << url);
    auto file = (fs::temp_directory_path() / "polygon4_bootstrap_temp_file").wstring();
    download(url, file, D_SILENT);
    return read_file(file);
}

void download(const String &url, const String &file, int flags)
{
    if (file.empty())
    {
        LOG_FATAL(logger, "file is empty!");
        exit_program(1);
    }
    if (!(flags & D_SILENT))
    {
        LOG_INFO(logger, "Downloading file: " << file);
    }
    else
    {
        LOG_TRACE(logger, "Downloading file: '" << file << "', url: '" << url << "'");
    }
    execute_and_print(
    {
        bootstrap_programs_prefix + curl,
        L"-L", // follow redirects
        L"-k", // insecure
        L"--connect-timeout", L"30", // initial connection timeout
        //L"--keepalive-time", L"45", // does not work on windows
        //L"-m", L"600", // max time for download
        (flags & D_CURL_SILENT) ? L"-s" : L"-#", // progress bar or silent
        L"-o" + file, // output file
        url
    });
    LOG_TRACE(logger, "Download completed: " << file);
}

void execute_and_print(Strings args, bool exit_on_error)
{
    auto result = execute_command(args);

    // print
    LOG_TRACE(logger, to_string(result.bytes));

    if (result.ret && exit_on_error)
    {
        // will die here
        // allowed to fail only after cleanup work
        check_return_code(result.ret);
        // should not hit
        abort();
    }
}

std::istream &safe_getline(std::istream &is, std::string &s)
{
    s.clear();

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for (;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if (sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case EOF:
            if (s.empty())
                is.setstate(std::ios::eofbit | std::ios::failbit);
            return is;
        default:
            s += (char)c;
        }
    }
}

SubprocessAnswer execute_command(Strings args, bool exit_on_error)
{
    using namespace boost::process;

    if (args[0].rfind(L".exe") != args[0].size() - 4)
        args[0] += L".exe";
    boost::algorithm::replace_all(args[0], "/", "\\");

    auto exec = fs::absolute(args[0]).wstring();

    std::vector<std::string> args_s;
    for (auto &a : args)
        args_s.push_back(to_string(a));

    context ctx;
    ctx.stdin_behavior = inherit_stream();
    ctx.stdout_behavior = capture_stream();
    ctx.stderr_behavior = capture_stream();
    
    SubprocessAnswer answer;

    try
    {
        child c = boost::process::launch(to_string(exec), args_s, ctx);

        std::mutex m_answer;
        auto stream_reader = [&c, &answer, &m_answer](std::istream &in, std::ostream &out)
        {
            std::thread t([&]()
            {
                std::string s;
                while (safe_getline(in, s))
                {
                    s += "\n";
                    std::lock_guard<std::mutex> lock(m_answer);
                    answer.bytes.insert(answer.bytes.end(), s.begin(), s.end());
                    out << s;
                }
            });
            return std::move(t);
        };

        auto &out = c.get_stdout();
        auto &err = c.get_stderr();

        auto t1 = stream_reader(out, std::cout);
        auto t2 = stream_reader(err, std::cerr);

        int ret = c.wait().exit_status();

        t1.join();
        t2.join();

        out.close();
        err.close();

        answer.bytes.resize(answer.bytes.size() + 3, 0);
        answer.ret = ret;
    }
    catch (...)
    {
        LOG_FATAL(logger, "Command failed: " << exec);
        throw;
    }

    return answer;
}

void check_return_code(int code)
{
    if (!code)
        return;
    LOG_ERROR(logger, "Last bootstrap step failed");
    exit_program(code);
}

void exit_program(int code)
{
    if (main_thread_id != std::this_thread::get_id())
        throw std::runtime_error("Exit attempt in non main thread");
    printf("Press Enter to continue...");
    getchar();
    exit(1);
}

String to_string(const Bytes &b)
{
    return String(b.begin(), b.end());
}

void enumerate_files(const path &dir, std::set<path> &files)
{
    try
    {
        if (!fs::exists(dir) || !fs::is_directory(dir))
        {
            std::cerr << "Source directory " << dir.string()
                << " does not exist or is not a directory." << '\n';
            return;
        }
    }
    catch (fs::filesystem_error const & e)
    {
        std::cerr << e.what() << '\n';
        return;
    }
    for (fs::directory_iterator file(dir); file != fs::directory_iterator(); ++file)
    {
        try
        {
            fs::wpath current(file->path());
            if (fs::is_directory(current))
                enumerate_files(current, files);
            else
                files.insert(canonical(current));
        }
        catch (fs::filesystem_error const & e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}

void remove_untracked(const ptree &data, const path &dir, const path &content_dir)
{
    String redirect = data.get(L"redirect", L"");
    if (!redirect.empty())
    {
        auto data2 = load_data(redirect);
        remove_untracked(data2, dir, content_dir);
        return;
    }

    LOG_INFO(logger, "Removing untracked files from " << content_dir.string());

    String file_prefix = data.get(L"file_prefix", L"");
    const ptree &files = data.get_child(L"files");

    std::set<path> actual_files;
    enumerate_files(content_dir, actual_files);

    std::set<path> package_files;
    for (auto &file : files)
    {
        auto check_path = file.second.get(L"check_path", L"");
        auto path = dir / check_path;
        package_files.insert(path);
    }

    std::set<path> to_remove;
    for (auto &file : actual_files)
    {
        if (package_files.count(file.wstring()))
            continue;
        to_remove.insert(file);
    }
    for (auto &f : to_remove)
    {
        LOG_INFO(logger, "removing: " << f.wstring());
        remove(f);
        auto p = f;
        while (fs::is_empty(p = p.parent_path()))
        {
            LOG_INFO(logger, "removing empty dir: " << p.wstring());
            fs::remove_all(p);
        }
    }
}
