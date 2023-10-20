find_path(FLAC_INCLUDE_DIR FLAC/stream_decoder.h)
find_library(FLAC_LIBRARIES FLAC NAMES flac)

if(FLAC_INCLUDE_DIR AND FLAC_LIBRARIES)
	set(FLAC_FOUND TRUE)
endif()

if(FLAC_FOUND)
	add_library(FLAC::FLAC SHARED IMPORTED)
	set_target_properties(FLAC::FLAC PROPERTIES
		IMPORTED_LOCATION "${FLAC_LIBRARIES}"
		INTERFACE_INCLUDE_DIRECTORIES "${FLAC_INCLUDE_DIR}/FLAC"
		INTERFACE_LINK_LIBRARIES "${FLAC_LIBRARIES}"
	)
	if (NOT FLAC_FIND_QUIETLY)
		message(STATUS "Found FLAC includes:	${FLAC_INCLUDE_DIR}/FLAC")
		message(STATUS "Found FLAC library:	${FLAC_LIBRARIES}")
	endif ()
else()
	if (FLAC_FIND_REQUIRED)
		message(FATAL_ERROR "Could NOT find FLAC development files")
	endif ()
endif()
