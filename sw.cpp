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

    auto cppstd = cpp23;

    auto &core = p.addTarget<StaticLibrary>("core");

    core.PackageDefinitions = true;
    core += cppstd;

    core += "src/core/.*"_rr, "Bootstrap.json";
    core.Public += "src/core"_idir;
    core.Public += "pub.egorpugin.primitives.http"_dep;
    core.Public += "pub.egorpugin.primitives.log"_dep;
    core.Public += "pub.egorpugin.primitives.pack"_dep;
    core.Public += "pub.egorpugin.primitives.hash"_dep;
    core.Public += "pub.egorpugin.primitives.command"_dep;
    core.Public += "pub.egorpugin.primitives.executor"_dep;
    core.Public += "pub.egorpugin.primitives.sw.main"_dep;

    {
        auto &t = p.addTarget<Executable>("developer");
        t += cppstd;
        t += "src/bootstrap_developer.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("release");
        t += cppstd;
        t += "src/bootstrap_release.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("tools");
        t += cppstd;
        t += "src/bootstrap_tools.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("updater", "0.0.1");
        t += cppstd;
        t += "src/updater.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("remove_untracked_content");
        t += cppstd;
        t += "src/remove_untracked_content.cpp";
        t += core;
    }
}

