
find_path(LIBMUNKRES_INCLUDE_DIR munkres.h 
          HINTS "${LIBMUNKRES_DIR}"
	  PATH_SUFFIXES include )

find_library(LIBMUNKRES_LIBRARY NAMES libmunkres.a
          HINTS "${LIBMUNKRES_DIR}"
	  PATH_SUFFIXES lib  )

set(LIBMUNKRES_LIBRARIES ${LIBMUNKRES_LIBRARY} )
set(LIBMUNKRES_INCLUDE_DIRS ${LIBMUNKRES_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBMUNKRES DEFAULT_MSG LIBMUNKRES_LIBRARY LIBMUNKRES_INCLUDE_DIR)
