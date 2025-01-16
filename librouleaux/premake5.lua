--[[
    This file defines the build system for librouleaux.
    librouleaux can be built as either a shared library, or a static library.
]]--

-- Build option function definitions
function debug_options()
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
end

function release_options()
    defines
    {
        "ROULX_BUILD_RELEASE"
    }

    symbols  "on"
    optimize "on"
end

function distribution_options()
    defines
    {
        "ROULX_BUILD_DISTRIBUTION"
    }

    symbols  "off"
    optimize "on"
end




project "librouleaux"
    -- Build target is set in filters --
    language "C"
    cdialect "C17"
    staticruntime "on" -- TODO(Steven): We probably want to set the runtime to dynamic if we build as a sharedlib?

    targetdir ("%{wks.location}/bin/" .. output_folder_name)
    objdir ("!%{wks.location}/bin-int/" .. output_folder_name .. "/%{prj.name}")

    files
    {
        "src/**.c",
        "include/**h"
    }

    includedirs
    {
        "include/rouleaux"
    }

    -- Static Library Options
    filter "configurations:Debug-StaticLib"
        kind "StaticLib"
        debug_options()

    filter "configurations:Release-StaticLib"
        kind "StaticLib"
        release_options()

    filter "configurations:Distribution-StaticLib"
        kind "StaticLib"
        distribution_options()
    
    -- Shared Library Options
    filter "configurations:Debug-SharedLib"
        kind "SharedLib"
        debug_options()

        defines
        {
            "ROULX_BUILD_DLL"
        }

    filter "configurations:Release-SharedLib"
        kind "SharedLib"
        release_options()

        defines
        {
            "ROULX_BUILD_DLL"
        }

    filter "configurations:Distribution-SharedLib"
        kind "SharedLib"
        distribution_options()

        defines
        {
            "ROULX_BUILD_DLL"
        }
