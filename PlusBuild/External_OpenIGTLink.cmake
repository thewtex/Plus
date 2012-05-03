# --------------------------------------------------------------------------
# OpenIGTLink 
SET (PLUS_OPENIGTLINK_DIR ${CMAKE_BINARY_DIR}/OpenIGTLink CACHE INTERNAL "Path to store OpenIGTLink sources.")
ExternalProject_Add( OpenIGTLink
            SOURCE_DIR "${PLUS_OPENIGTLINK_DIR}" 
            BINARY_DIR "OpenIGTLink-bin"
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
SET(OpenIGTLink_DIR ${CMAKE_BINARY_DIR}/OpenIGTLink-bin CACHE PATH "The directory containing a CMake configuration file for OpenIGTLink" FORCE)
