vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO githubuser0xFFFF/Qt-Advanced-Docking-System
    REF 8d30fc9c3c36ac66bf3d027aaad7adff973f628f #v3.8.4
    SHA512 d26bbb66621d469bfe10a0c46c21953542ee4b7abd53f1c5d24c1932708c0de3386c33ef859268502e9c0fe01ae7066c815523fa196a4982bbeb1dfffbc2681d
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS 
        -DBUILD_EXAMPLES=OFF
        -DADS_VERSION=3.8.4
)

vcpkg_cmake_install()

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
file(INSTALL ${SOURCE_PATH}/gnu-lgpl-v2.1.md DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/license)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)
vcpkg_cmake_config_fixup(PACKAGE_NAME qtadvanceddocking CONFIG_PATH lib/cmake/qtadvanceddocking)
