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
#include "Logger.h"

DECLARE_STATIC_LOGGER(logger, "main");

std::thread::id main_thread_id;

int main(int argc, char *argv[])
try
{
    auto p = path(argv[0]);
    p = p.parent_path() / p.stem();

    auto log_level =
#ifdef NDEBUG
        "Info"
#else
        "Debug"
#endif
        ;
    LOGGER_CONFIGURE(log_level, p.string());

    main_thread_id = std::this_thread::get_id();

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
                copy_file(argv[0], dst, fs::copy_option::overwrite_if_exists);
                LOG_INFO(logger, "Update successful.");
                return 0;
            }
            else if (strcmp(arg, "--version") == 0)
            {
                print_version();
                return 0;
            }
            else
            {
                LOG_FATAL(logger, "Unknown option: " << arg);
                return 1;
            }
        }
    }

    print_version();

    auto data = load_data(std::wstring(BOOTSTRAP_JSON_URL));

    bootstrap_module_main(argc, argv, data);

    return 0;
}
catch (std::exception &e)
{
    LOG_ERROR(logger, e.what());
    check_return_code(1);
}
catch (...)
{
    LOG_FATAL(logger, "Unkown exception!");
    check_return_code(2);
}