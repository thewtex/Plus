IF(OpenIGTLink_DIR)

  # OpenIGTLink has been built already
  FIND_PACKAGE(OpenIGTLink REQUIRED PATHS ${OpenIGTLink_DIR} NO_DEFAULT_PATH)

  
  # Copy libraries to PLUS_EXECUTABLE_OUTPUT_PATH
  IF ( ${CMAKE_GENERATOR} MATCHES "Visual Studio" )
    FILE(COPY 
      ${OpenIGTLink_LIBRARY_DIRS}/Release/
      DESTINATION ${PLUS_EXECUTABLE_OUTPUT_PATH}/Release
      FILES_MATCHING REGEX .*${CMAKE_SHARED_LIBRARY_SUFFIX}
      )
    FILE(COPY 
      ${OpenIGTLink_LIBRARY_DIRS}/Debug/
      DESTINATION ${PLUS_EXECUTABLE_OUTPUT_PATH}/Debug
      FILES_MATCHING REGEX .*${CMAKE_SHARED_LIBRARY_SUFFIX}
      )    
  ELSE()
      FILE(COPY 
        ${OpenIGTLink_LIBRARY_DIRS}/
        DESTINATION ${PLUS_EXECUTABLE_OUTPUT_PATH}
        FILES_MATCHING REGEX .*${CMAKE_SHARED_LIBRARY_SUFFIX}
        )
  ENDIF()
  SET (PLUS_OpenIGTLink_DIR "${OpenIGTLink_DIR}" CACHE INTERNAL "Path to store OpenIGTLink binaries")
  
ELSE(OpenIGTLink_DIR)

  # OpenIGTLink has not been built yet, so download and build it as an external project
  
  SET (PLUS_OpenIGTLink_SRC_DIR "${CMAKE_BINARY_DIR}/OpenIGTLink")
  SET (PLUS_OpenIGTLink_DIR "${CMAKE_BINARY_DIR}/OpenIGTLink-bin" CACHE INTERNAL "Path to store OpenIGTLink binaries")
  ExternalProject_Add( OpenIGTLink
    SOURCE_DIR "${PLUS_OpenIGTLink_SRC_DIR}"
    BINARY_DIR "${PLUS_OpenIGTLink_DIR}"
    #--Download step--------------
    GIT_REPOSITORY "${GIT_PROTOCOL}://github.com/openigtlink/OpenIGTLink.git"
    #--Configure step-------------
    CMAKE_ARGS 
        -DLIBRARY_OUTPUT_PATH:STRING=${PLUS_EXECUTABLE_OUTPUT_PATH}
        -DBUILD_SHARED_LIBS:BOOL=${PLUSBUILD_BUILD_SHARED_LIBS}
        -DBUILD_TESTING:BOOL=OFF 
        -DBUILD_EXAMPLES:BOOL=OFF
        -DOpenIGTLink_PROTOCOL_VERSION_2:BOOL=ON
        -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
        -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    #--Build step-----------------
    #--Install step-----------------
    INSTALL_COMMAND ""
    DEPENDS ${OpenIGTLink_DEPENDENCIES}
    )  
    
ENDIF(OpenIGTLink_DIR)
