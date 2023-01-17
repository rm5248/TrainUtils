vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO rm5248/CSerial
	REF v0.3
	SHA512 27f0a655266473a2fc594faebdc29a1818ab0f8de01d6d146f8d8dbda8e95dbda40e95e0d2f829449277df6ae7140ee0df61f80456e7e903895c024740decaef
	HEAD_REF master
)

vcpkg_cmake_configure(SOURCE_PATH ${SOURCE_PATH})
vcpkg_cmake_install()
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/rm5248-cserial" RENAME copyright)
