void configure(Build &s)
{
#ifndef SW_CPP_DRIVER_API_VERSION
    auto ss = s.createSettings();
    ss.Native.LibrariesType = LibraryType::Static;
    s.addSettings(ss);
#endif
}

void build(Solution &s)
{
    auto &p = s.addProject("Polygon4.Bootstrap", "0.0.12");
    p += Git("https://github.com/aimrebirth/Bootstrap", "", "{v}");

    auto &core = p.addTarget<StaticLibrary>("core");

    core.PackageDefinitions = true;
    core += cpp20;

    core += "src/core/.*"_rr, "Bootstrap.json";
    core.Public += "src/core"_idir;
    core.Public += "_WIN32_WINNT=0x0601"_def;
    core.Public += "pub.egorpugin.primitives.http-master"_dep;
    core.Public += "pub.egorpugin.primitives.log-master"_dep;
    core.Public += "pub.egorpugin.primitives.pack-master"_dep;
    core.Public += "pub.egorpugin.primitives.hash-master"_dep;
    core.Public += "pub.egorpugin.primitives.command-master"_dep;
    core.Public += "pub.egorpugin.primitives.executor-master"_dep;
    core.Public += "pub.egorpugin.primitives.sw.main-master"_dep;

    {
        auto &t = p.addTarget<Executable>("developer");
        t += cpp20;
        t += "src/bootstrap_developer.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("release");
        t += cpp20;
        t += "src/bootstrap_release.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("tools");
        t += cpp20;
        t += "src/bootstrap_tools.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("updater", "0.0.1");
        t += cpp20;
        t += "src/updater.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("remove_untracked_content");
        t += cpp20;
        t += "src/remove_untracked_content.cpp";
        t += core;
    }
}

