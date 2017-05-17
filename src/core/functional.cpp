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

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

#include <atomic>
#include <codecvt>
#include <locale>
#include <mutex>
#include <unordered_map>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "core");

//
// global data
//

extern String bootstrap_programs_prefix;

String git = "git";

//
// helper functions
//

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
        for (int i = 0; i < 16; i++)
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
                            LOG_INFO(logger, "Calculating md5 for " << file);
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
                        LOG_INFO(logger, "Downloading " << file);
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
                    LOG_INFO(logger, "Downloading " << url);
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

void execute_and_print(Strings args, bool exit_on_error)
{
    command::Result ret;
    auto print = [](const String &s)
    {
        LOG_INFO(logger, s);
    };
    command::Options o;
    o.out.action = print;
    o.err.action = print;

    auto result = command::execute_and_capture(args, o);

    // print
    LOG_TRACE(logger, "OUT:\n" << result.out);
    LOG_TRACE(logger, "ERR:\n" << result.err);

    if (result.rc && exit_on_error)
    {
        // will die here
        // allowed to fail only after cleanup work
        check_return_code(result.rc);
        // should not hit
        abort();
    }
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