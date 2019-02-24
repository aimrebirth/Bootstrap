void build(Solution &s)
{
    auto &p = s.addProject("Polygon4.Bootstrap");
    p += Git("enter your url here", "enter tag here", "or branch here");

    auto &core = p.addTarget<StaticLibrary>("core");
    core.CPPVersion = CPPLanguageStandard::CPP17;
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
        t.CPPVersion = CPPLanguageStandard::CPP17;
        t += "src/bootstrap_developer.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("release");
        t.CPPVersion = CPPLanguageStandard::CPP17;
        t += "src/bootstrap_release.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("tools");
        t.CPPVersion = CPPLanguageStandard::CPP17;
        t += "src/bootstrap_tools.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("updater");
        t.CPPVersion = CPPLanguageStandard::CPP17;
        t += "src/updater.cpp";
        t += core;
    }

    {
        auto &t = p.addTarget<Executable>("remove_untracked_content");
        t.CPPVersion = CPPLanguageStandard::CPP17;
        t += "src/remove_untracked_content.cpp";
        t += core;
    }
}

