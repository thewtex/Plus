# Find the ultrasonix SDK 
# This module defines
# ULTRASONIX_SDK_DIR - path to the Ultrasonix SDK 
#

IF ( NOT "${ULTRASONIX_SDK_VERSION}" STREQUAL "${PLUS_ULTRASONIX_SDK_MAJOR_VERSION}.${PLUS_ULTRASONIX_SDK_MINOR_VERSION}.${PLUS_ULTRASONIX_SDK_PATCH_VERSION}" )
  SET(ULTRASONIX_SDK_VERSION ${PLUS_ULTRASONIX_SDK_MAJOR_VERSION}.${PLUS_ULTRASONIX_SDK_MINOR_VERSION}.${PLUS_ULTRASONIX_SDK_PATCH_VERSION} CACHE INTERNAL "Actual SDK version used." )
  UNSET(ULTRASONIX_SDK_DIR CACHE)
ENDIF()

SET( ULTRASONIX_SDK_PATH_HINTS 
    ../Ultrasonix
    ../PLTools/Ultrasonix
    ../../PLTools/Ultrasonix
    ../PLTools/trunk/Ultrasonix
    ${CMAKE_CURRENT_BINARY_DIR}/PLTools/Ultrasonix
    )

FIND_PATH(ULTRASONIX_SDK_DIR bin/ulterius${CMAKE_SHARED_LIBRARY_SUFFIX}
  PATH_SUFFIXES 
    sdk-${ULTRASONIX_SDK_VERSION}
    Ulterius-${ULTRASONIX_SDK_VERSION}/
  PATHS ${ULTRASONIX_SDK_PATH_HINTS} 
  )

# handle the QUIETLY and REQUIRED arguments and set ULTRASONIX_SDK_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ULTRASONIX_SDK  DEFAULT_MSG  ULTRASONIX_SDK_DIR )

IF(ULTRASONIX_SDK_FOUND)
  SET( ULTRASONIX_SDK_DIRS ${ULTRASONIX_SDK_DIR} )
ENDIF()
