#copied from here, thank you
#https://github.com/sipwise/sems/blob/master/cmake/FindLame.cmake

find_path(LAME_INCLUDE_DIR lame/lame.h)
find_library(LAME_LIBRARIES lame NAMES mp3lame)

if(LAME_INCLUDE_DIR AND LAME_LIBRARIES)
	set(LAME_FOUND TRUE)
endif(LAME_INCLUDE_DIR AND LAME_LIBRARIES)

if(LAME_FOUND)
	add_library(LAME::LAME SHARED IMPORTED)
	set_target_properties(LAME::LAME PROPERTIES
		IMPORTED_LOCATION "${LAME_LIBRARIES}"
		INTERFACE_INCLUDE_DIRECTORIES "${LAME_INCLUDE_DIR}/lame"
		INTERFACE_LINK_LIBRARIES "${LAME_LIBRARIES}"
	)
	if (NOT Lame_FIND_QUIETLY)
		message(STATUS "Found lame includes:	${LAME_INCLUDE_DIR}/lame")
		message(STATUS "Found lame library:	${LAME_LIBRARIES}")
	endif (NOT Lame_FIND_QUIETLY)
else(LAME_FOUND)
	if (Lame_FIND_REQUIRED)
		message(FATAL_ERROR "Could NOT find lame development files")
	endif (Lame_FIND_REQUIRED)
endif(LAME_FOUND)
