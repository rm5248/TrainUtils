vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO rm5248/CSerial
	REF v0.3
	SHA512 0
	HEAD_REF master
)

vcpkg_cmake_configure(SOURCE_PATH ${SOURCE_PATH})
vcpkg_cmake_install()
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/rm5248-cserial" RENAME copyright)
