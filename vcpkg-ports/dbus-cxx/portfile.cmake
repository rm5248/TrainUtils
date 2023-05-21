vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO dbus-cxx/dbus-cxx
	REF 2.4.0
	SHA512 110de7a1bbc0a3cae4270a5772206c31b689ece07738cbf4541c9e05ceb1576658d5df9c22440c6967cce82deb44e1645983e5266d9b5f51745cc27b1f4d4a71
	HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DENABLE_QT_SUPPORT=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/dbus-cxx DO_NOT_DELETE_PARENT_CONFIG_PATH)
file(INSTALL "${SOURCE_PATH}/COPYING" DESTINATION "${CURRENT_PACKAGES_DIR}/share/dbus-cxx" RENAME copyright)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

