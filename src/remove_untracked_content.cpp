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

wstring bootstrap_programs_prefix = BOOTSTRAP_PREFIX;

int version()
{
    return UNTRACKED_CONTENT_DELETER_VERSION;
}

void print_version()
{
    PRINT("Polygon-4 Developer Untracked Content Deleter Version " << version());
    SPACE();
}

void check_version(int ver)
{
    if (ver == version())
        return;
    PRINT("FATAL ERROR:");
    PRINT("You have wrong version of the tool!");
    PRINT("Actual version: " << ver);
    PRINT("Your version: " << version());
    PRINT("Please, run BootstrapUpdater.exe to update the bootstrapper.");
    exit_program(1);
}

void enumerate_files(const wpath &dir, set<wpath> &files)
{
    namespace fs = boost::filesystem;
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

void remove_untracked(const pt::wptree &data, const wpath &dir, const wpath &content_dir)
{
    wstring redirect = data.get(L"redirect", L"");
    if (!redirect.empty())
    {
        auto data2 = load_data(redirect);
        remove_untracked(data2, dir, content_dir);
        return;
    }

    wstring file_prefix = data.get(L"file_prefix", L"");
    const pt::wptree &files = data.get_child(L"files");

    set<wpath> actual_files;
    enumerate_files(content_dir, actual_files);

    for (auto &repo : files)
    {
        auto name = repo.second.get<wstring>(L"name", L"");
        auto check_path = repo.second.get(L"check_path", L"");
        auto file = (dir / check_path).wstring();
        try
        {
            file = canonical(file).wstring();
        }
        catch (std::exception &e)
        {
            e.what();
        }
        auto hash = repo.second.get<wstring>(L"md5", L"");
        if (exists(file))
        {
            auto i = find(actual_files.begin(), actual_files.end(), file);
            if (i != actual_files.end())
            {
                actual_files.erase(i);
            }
        }
    }
    for (auto &f : actual_files)
    {
        printf("removing: %s\n", f.string().c_str());
        remove(f);
    }
}

int bootstrap_module_main(int argc, char *argv[], const pt::wptree &data)
{
    check_version(data.get<int>(L"tools.remove_untracked_content.version"));

    auto polygon4 = data.get<wstring>(L"name") + L"Developer";
    wpath base_dir = current_path() / BOOTSTRAP_PREFIX;
    wpath polygon4_dir = base_dir / polygon4;
    wpath download_dir = base_dir / BOOTSTRAP_DOWNLOADS;

    create_directory(polygon4_dir);
    
    remove_untracked(data.get_child(L"developer"), polygon4_dir, polygon4_dir / "Content");

    PRINT("Removed untracked contenr developer files successfully");

    return 0;
}