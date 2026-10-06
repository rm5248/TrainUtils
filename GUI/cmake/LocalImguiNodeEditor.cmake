# Builds imgui-node-editor from a local source checkout, as an alternative to
# the unofficial-imgui-node-editor package installed by vcpkg. Included from
# GUI/CMakeLists.txt only when TRAINUTILS_LOCAL_IMGUI_NODE_EDITOR_DIR is set.
#
# TRAINUTILS_LOCAL_IMGUI_NODE_EDITOR_DIR must point at a checkout of
# https://github.com/thedmd/imgui-node-editor. Its own vendored external/imgui
# is not used -- it links against whichever imgui::imgui target is already
# defined (local or vcpkg), set up earlier in GUI/CMakeLists.txt.
#
# Defines unofficial::imgui-node-editor::imgui-node-editor, matching the
# target name find_package(unofficial-imgui-node-editor CONFIG) would produce
# from vcpkg, so the rest of GUI/CMakeLists.txt doesn't need to care which
# path was used.

set(_local_node_editor_dir "${TRAINUTILS_LOCAL_IMGUI_NODE_EDITOR_DIR}")

if(NOT EXISTS "${_local_node_editor_dir}/imgui_node_editor.h")
    message(FATAL_ERROR
        "TRAINUTILS_LOCAL_IMGUI_NODE_EDITOR_DIR is set to '${_local_node_editor_dir}' but "
        "'${_local_node_editor_dir}/imgui_node_editor.h' doesn't exist. It should point at "
        "a checkout of https://github.com/thedmd/imgui-node-editor.")
endif()

add_library(imgui-node-editor-local STATIC
    "${_local_node_editor_dir}/crude_json.cpp"
    "${_local_node_editor_dir}/crude_json.h"
    "${_local_node_editor_dir}/imgui_canvas.cpp"
    "${_local_node_editor_dir}/imgui_canvas.h"
    "${_local_node_editor_dir}/imgui_node_editor.cpp"
    "${_local_node_editor_dir}/imgui_node_editor.h"
    "${_local_node_editor_dir}/imgui_node_editor_api.cpp"
    "${_local_node_editor_dir}/imgui_node_editor_internal.h"
    "${_local_node_editor_dir}/imgui_node_editor_internal.inl"
    "${_local_node_editor_dir}/imgui_bezier_math.h"
    "${_local_node_editor_dir}/imgui_bezier_math.inl"
    "${_local_node_editor_dir}/imgui_extra_math.h"
    "${_local_node_editor_dir}/imgui_extra_math.inl"
)
target_link_libraries(imgui-node-editor-local PUBLIC imgui::imgui)
target_include_directories(imgui-node-editor-local PRIVATE "${_local_node_editor_dir}")

# Application code includes the node editor as <imgui-node-editor/imgui_node_editor.h>,
# matching the layout vcpkg installs it under. Generate a shim header with that path
# so the include resolves no matter what this checkout's directory is named.
set(_shim_dir "${CMAKE_CURRENT_BINARY_DIR}/local-imgui-node-editor-shim")
file(MAKE_DIRECTORY "${_shim_dir}/imgui-node-editor")
file(WRITE "${_shim_dir}/imgui-node-editor/imgui_node_editor.h"
    "#pragma once\n#include \"${_local_node_editor_dir}/imgui_node_editor.h\"\n")
target_include_directories(imgui-node-editor-local PUBLIC "${_shim_dir}")

add_library(unofficial::imgui-node-editor::imgui-node-editor ALIAS imgui-node-editor-local)

unset(_local_node_editor_dir)
unset(_shim_dir)
