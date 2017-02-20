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
#include "http.h"
#include "pack.h"

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <openssl/evp.h>

#include <atomic>
#include <codecvt>
#include <locale>
#include <mutex>
#include <unordered_map>

#include "logger.h"
DECLARE_STATIC_LOGGER(logger, "core");

//
// global data
//

extern String bootstrap_programs_prefix;

String git = "git";
String cmake = "cmake";
String msbuild = "c:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MSBuild.exe";

//
// helper functions
//

std::string to_string(const std::wstring &s)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(s.c_str());
}

std::wstring to_wstring(const std::string &s)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(s.c_str());
}

path temp_directory_path(const path &subdir)
{
    auto p = fs::temp_directory_path() / "polygon4" / subdir;
    fs::create_directories(p);
    return p;
}

path get_temp_filename(const path &subdir)
{
    return temp_directory_path(subdir) / fs::unique_path();
}

//
// function definitions
//

void create_project_files(const path &dir)
{
    auto uproject = dir / "Polygon4.uproject";
    if (exists(uproject))
    {
        LOG_INFO(logger, "Creating project files");
        execute_and_print({ bootstrap_programs_prefix + uvs, "/projectfiles", uproject.string() });
    }
}

void build_project(const path &dir)
{
    auto sln = dir / "Polygon4.sln";
    LOG_INFO(logger, "Building Polygon4 Unreal project");
    execute_and_print({ msbuild, sln.string(), "/p:Configuration=Development Editor", "/p:Platform=Win64", "/m" });
}

void build_engine(const path &dir)
{
    auto bin_dir = dir / "ThirdParty" / "Engine" / "Win64";
    auto sln_file = bin_dir / "Engine.sln";
    if (!exists(sln_file))
        return;
    LOG_INFO(logger, "Building Engine");
    execute_and_print({ cmake, "--build", bin_dir.string(), "--config", "RelWithDebInfo" });
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
    if (cmake.empty())
        return;
    LOG_INFO(logger, "Running CMake");
    execute_and_print({ cmake,
        "-H" + src_dir.string(),
        "-B" + bin_dir.string(),
        "-DDATA_MANAGER_DIR=../DataManager",
        "-DBOOST_ROOT=" + boost_dir.string(),
        "-DBOOST_LIBRARYDIR=" + boost_lib_dir.string(),
        "-DSWIG_DIR=" + swig_dir.string(),
        "-DSWIG_EXECUTABLE=" + swig_exe.string(),
        "-DBISON_EXECUTABLE=" + bison_exe.string(),
        "-DFLEX_EXECUTABLE=" + flex_exe.string(),
        "-G", "Visual Studio 14 Win64" });
    if (!exists(sln_file))
        check_return_code(1);
}

void download_files(const path &dir, const path &output_dir, const ptree &data)
{
    String redirect = data.get("redirect", "");
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

        String file_prefix = data.get("file_prefix", "");
        const ptree &files = data.get_child("files");
        for (auto &repo : files)
        {
            io_service.post([&repo, &dir, &file_prefix, &output_dir, &lwt_data, &lwt_mutex]()
            {
                auto url = repo.second.get<String>("url");
                auto name = repo.second.get<String>("name", "");
                auto file = dir / (file_prefix + name);
                auto new_hash_md5 = repo.second.get<String>("md5", "");
                auto check_path = repo.second.get("check_path", "");
                bool packed = repo.second.get<bool>("packed", false);
                String old_file_md5;
                if (!packed)
                {
                    file = (output_dir / check_path).string();
                    bool file_exists = fs::exists(file);
                    if (file_exists)
                    {
                        auto file_lwt = fs::last_write_time(file);
                        if (lwt_data.count(file.string()))
                        {
                            auto &file_lwt_data_main = lwt_data.find(file.string())->second;
                            auto &file_lwt_data = file_lwt_data_main.get_child("lwt");
                            old_file_md5 = file_lwt_data_main.get("md5", String());

                            auto old_file_lwt = file_lwt_data.get_value<time_t>(0);
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
                                    old_file_md5 = md5(file);
                                }
                            }
                            file_lwt_data.put_value(file_lwt);
                            file_lwt_data_main.put("md5", new_hash_md5);
                        }
                        else
                        {
                            // no md5 was calculated before
                            old_file_md5 = md5(file);
                            if (old_file_md5 == new_hash_md5)
                            {
                                ptree value;
                                value.add("lwt", file_lwt);
                                value.add("md5", old_file_md5);

                                std::lock_guard<std::mutex> g(lwt_mutex);
                                lwt_data.add_child(ptree::path_type(file.string(), '|'), value);
                            }
                        }
                    }
                    if (!file_exists || old_file_md5 != new_hash_md5)
                    {
                        download_file(url, file);

                        // file is changed in previous download, so md5 is of new file,
                        //  not same in condition above!
                        auto new_file_md5 = md5(file);

                        // recheck hash
                        if (new_file_md5 != new_hash_md5)
                        {
                            LOG_FATAL(logger, "Wrong file is located on server! Cannot proceed.");
                            exit_program(1);
                        }

                        auto file_lwt = fs::last_write_time(file);
                        ptree value;
                        value.add("lwt", file_lwt);
                        value.add("md5", new_file_md5);

                        std::lock_guard<std::mutex> g(lwt_mutex);
                        lwt_data.add_child(ptree::path_type(file.string(), '|'), value);
                    }
                    return;
                }
                if (!fs::exists(file) || md5(file) != new_hash_md5)
                {
                    download_file(url, file);
                    // file is changed in previous download, so md5 is of new file,
                    //  not same in condition above!
                    // recheck hash
                    if (md5(file) != new_hash_md5)
                    {
                        LOG_FATAL(logger, "Wrong file is located on server! Cannot proceed.");
                        exit_program(1);
                    }
                    unpack_file(file, output_dir);
                }
                if (!check_path.empty() && !exists(output_dir / check_path))
                {
                    unpack_file(file, output_dir);
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
    auto url = data.get<String>("url_zip");
    auto suffix = data.get<String>("suffix");
    auto name = data.get<String>("name");
    auto file = path(BOOTSTRAP_DOWNLOADS) / (name + "-" + suffix + ".zip");
    download_file(url, file);
    unpack_file(file, BOOTSTRAP_DOWNLOADS);
    copy_dir(path(BOOTSTRAP_DOWNLOADS) / (name + "-master"), dir);
}

void download_submodules()
{
    execute_and_print({git, "submodule", "update", "--init", "--recursive"});
}

void download_sources(const String &url)
{
    LOG_INFO(logger, "Downloading latest sources from Github repositories");
    execute_and_print({git, "clone", url, "."});
    if (!fs::exists(".git"))
    {
        execute_and_print({git, "init"});
        execute_and_print({git, "remote", "add", "origin", url});
        execute_and_print({git, "fetch"});
        execute_and_print({git, "reset", "origin/master", "--hard"});
    }
    download_submodules();
}

void update_sources()
{
    LOG_INFO(logger, "Updating latest sources from Github repositories");
    execute_and_print({git, "pull", "origin", "master"});
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
        prog = boost::process::search_path(prog).string();
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

    auto s = download_file(url);
    ptree pt;
    std::stringstream ss(s);
    try
    {
        pt::json_parser::read_json(ss, pt);
    }
    catch (pt::json_parser_error &e)
    {
        LOG_ERROR(logger, "Json file: " << url << " has errors in its structure!");
        LOG_ERROR(logger, e.what());
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
        LOG_ERROR(logger, e.what());
        LOG_ERROR(logger, "Please, report to the author.");
        throw;
    }
    return pt;
}

String read_file(const path &p, bool no_size_check)
{
    if (!fs::exists(p))
        throw std::runtime_error("File '" + p.string() + "' does not exist");

    auto fn = p.string();
    std::ifstream ifile(fn, std::ios::in | std::ios::binary);
    if (!ifile)
        throw std::runtime_error("Cannot open file '" + fn + "' for reading");

    size_t sz = (size_t)fs::file_size(p);
    if (!no_size_check && sz > 10'000'000)
        throw std::runtime_error("File " + fn + " is very big (> ~10 MB)");

    String f;
    f.resize(sz);
    ifile.read(&f[0], sz);
    return f;
}

void write_file(const path &p, const String &s)
{
    auto pp = p.parent_path();
    if (!pp.empty())
        fs::create_directories(pp);

    std::ofstream ofile(p.string(), std::ios::out | std::ios::binary);
    if (!ofile)
        throw std::runtime_error("Cannot open file '" + p.string() + "' for writing");
    ofile << s;
}

void execute_and_print(Strings args, bool exit_on_error)
{
    auto result = execute_command(args);

    // print
    LOG_TRACE(logger, "OUT:\n" << result.out);
    LOG_TRACE(logger, "ERR:\n" << result.err);

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
    if (args[0].rfind(".exe") != args[0].size() - 4)
        args[0] += ".exe";
    boost::algorithm::replace_all(args[0], "/", "\\");

    auto exec = fs::absolute(args[0]).string();

    SubprocessAnswer answer;

    bp::ipstream all, out, err;

    //boost::asio::io_service io_service;
    //bp::async_pipe p(io_service);

    bp::child c(
        bp::exe = exec,
        bp::args = Strings{args.begin() + 1, args.end()},

        (bp::std_out & bp::std_err) > all

        /*io_service,
        bp::on_exit([&](int exit, const std::error_code& ec_in)
        {
            p.async_close();
        })*/

        //bp::std_out > out,
        //bp::std_err > err
    );

    /*std::vector<char> in_buf;
    boost::asio::async_read(p, boost::asio::buffer(in_buf), [&answer, &in_buf](const boost::system::error_code &, std::size_t read)
    {
        if (!read)
            return;
        answer.out += String(in_buf.begin(), in_buf.end()) + "\n";
    });*/

    //io_service.run();


    String s;
    while (std::getline(all, s))
    {
        answer.out += s + "\n";
        std::cout << s << "\n";
    }

    c.wait();

    answer.ret = c.exit_code();

    /*while (std::getline(err, s))
        answer.err += s + "\n";*/

    /*try
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
    }*/

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
    String redirect = data.get("redirect", "");
    if (!redirect.empty())
    {
        auto data2 = load_data(redirect);
        remove_untracked(data2, dir, content_dir);
        return;
    }

    LOG_INFO(logger, "Removing untracked files from " << content_dir.string());

    const ptree &files = data.get_child("files");

    std::set<path> actual_files;
    enumerate_files(content_dir, actual_files);

    std::set<path> package_files;
    for (auto &file : files)
    {
        auto check_path = file.second.get("check_path", "");
        auto path = dir / check_path;
        package_files.insert(path);
    }

    std::set<path> to_remove;
    for (auto &file : actual_files)
    {
        if (package_files.count(file.string()))
            continue;
        to_remove.insert(file);
    }
    for (auto &f : to_remove)
    {
        LOG_INFO(logger, "removing: " << f.string());
        remove(f);
        auto p = f;
        while (fs::is_empty(p = p.parent_path()))
        {
            LOG_INFO(logger, "removing empty dir: " << p.string());
            fs::remove_all(p);
        }
    }
}

// keep always digits,lowercase,uppercase
static const char alnum[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

String hash_to_string(const uint8_t *hash, size_t hash_size)
{
    String s;
    for (uint32_t i = 0; i < hash_size; i++)
    {
        s += alnum[(hash[i] & 0xF0) >> 4];
        s += alnum[(hash[i] & 0x0F) >> 0];
    }
    return s;
}

String hash_to_string(const String &hash)
{
    return hash_to_string((uint8_t *)hash.c_str(), hash.size());
}

String md5(const String &data)
{
    uint8_t hash[EVP_MAX_MD_SIZE];
    uint32_t hash_size;
    EVP_Digest(data.data(), data.size(), hash, &hash_size, EVP_md5(), nullptr);
    return hash_to_string(hash, hash_size);
}

String md5(const path &fn)
{
    return md5(read_file(fn, true));
}
