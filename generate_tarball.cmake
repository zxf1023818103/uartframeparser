file(INSTALL DESTINATION . TYPE DIRECTORY FILES ${QT_QPA_PLATFORM_PLUGIN_PATH}/platforms ${QT_QPA_PLATFORM_PLUGIN_PATH}/styles)
file(REAL_PATH . CURRENT_DIR)
file(GLOB INSTALL_LIST RELATIVE ${CURRENT_DIR} *.exe *.dll platforms/*.dll styles/*.dll *.qm)

message("Creating zip file...")

file(ARCHIVE_CREATE OUTPUT ${NAME}.zip
	PATHS ${INSTALL_LIST}
	FORMAT zip
	VERBOSE)

file(SIZE ${NAME}.zip ARCHIVE_SIZE)

message("Done. ${NAME}.zip: ${ARCHIVE_SIZE} Bytes")
