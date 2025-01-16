-- Solution Level premake

workspace "rouleauxc"
    architecture "x86_64"
    configurations
    {
        -- Build configurations for building librouleaux as a static library
        "Debug-StaticLib",
        "Release-StaticLib",
        "Distribution-StaticLib",

        -- Build configurations for building librouleaux as a shared library
        "Debug-SharedLib",
        "Release-SharedLib",
        "Distribution-SharedLib"
    }
    toolset "clang"

output_folder_name = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

startproject "roulx"

group ""
    include "librouleaux" -- the base language library project
    include "roulx" -- the interpeter project
    --include "roulxc" -- the compiler project
