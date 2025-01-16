-- Project file for the rouleaux interpreter

project "roulx"
    kind "ConsoleApp"
    language "C"
    cdialect "C17"
    staticruntime "on"

    targetdir ("%{wks.location}/bin/" .. output_folder_name)
    objdir ("!%{wks.location}/bin-int/" .. output_folder_name .. "/%{prj.name}")

    buildoptions
    {
        "-Wno-microsoft-include" -- disables warning about non-portable include on windows
    }

    files
    {
        "src/**.c",
        "include/**h"
    }

    includedirs
    {
        "include",
        "../librouleaux/include/"
    }

    links
    {
        "librouleaux"
    }

    filter "configurations:Debug*"
        defines
        {
            "ROULX_BUILD_DEBUG"
        }

        buildoptions
        {
            "-gcodeview" -- needed for clang to produce windows debug info format
        }

        linkoptions
        {
            "-g" -- Needed to produce pdb info from linking
        }

        symbols  "on"
        optimize "off"

    filter "configurations:Release*"
        defines
        {
            "ROULX_BUILD_RELEASE"
        }

        symbols  "on"
        optimize "on"

    filter "configurations:Distribution*"
        defines
        {
            "ROULX_BUILD_DISTRIBUTION"
        }

        symbols  "off"
        optimize "on"
