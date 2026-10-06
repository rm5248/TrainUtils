# Builds imgui from a local source checkout, as an alternative to the imgui
# package installed by vcpkg. Included from GUI/CMakeLists.txt only when
# TRAINUTILS_LOCAL_IMGUI_DIR is set.
#
# TRAINUTILS_LOCAL_IMGUI_DIR must point at a checkout of
# https://github.com/ocornut/imgui (any branch/tag). Upstream imgui ships no
# CMakeLists.txt of its own, so this compiles the core sources directly.
#
# Defines imgui::imgui, matching the target name find_package(imgui CONFIG)
# would produce from vcpkg, so the rest of GUI/CMakeLists.txt doesn't need to
# care which path was used.

set(_local_imgui_dir "${TRAINUTILS_LOCAL_IMGUI_DIR}")

if(NOT EXISTS "${_local_imgui_dir}/imgui.h")
    message(FATAL_ERROR
        "TRAINUTILS_LOCAL_IMGUI_DIR is set to '${_local_imgui_dir}' but "
        "'${_local_imgui_dir}/imgui.h' doesn't exist. It should point at a checkout "
        "of https://github.com/ocornut/imgui.")
endif()

add_library(imgui-local STATIC
    "${_local_imgui_dir}/imgui.cpp"
    "${_local_imgui_dir}/imgui.h"
    "${_local_imgui_dir}/imgui_demo.cpp"
    "${_local_imgui_dir}/imgui_draw.cpp"
    "${_local_imgui_dir}/imgui_internal.h"
    "${_local_imgui_dir}/imgui_tables.cpp"
    "${_local_imgui_dir}/imgui_widgets.cpp"
)
target_include_directories(imgui-local PUBLIC "${_local_imgui_dir}")

add_library(imgui::imgui ALIAS imgui-local)

unset(_local_imgui_dir)
