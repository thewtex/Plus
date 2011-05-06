# --------------------------------------------------------------------------
# PlusApp
SET (PLUS_PLUSAPP_DIR ${CMAKE_BINARY_DIR}/PlusApp CACHE INTERNAL "Path to store PlusApp contents.")
ExternalProject_Add(PlusApp
            SOURCE_DIR "${PLUS_PLUSAPP_DIR}" 
            BINARY_DIR "PlusApp-bin"
            #--Download step--------------
            SVN_USERNAME ${PLUSBUILD_ASSEMBLA_USERNAME}
            SVN_PASSWORD ${PLUSBUILD_ASSEMBLA_PASSWORD}
            SVN_REPOSITORY https://subversion.assembla.com/svn/plus/trunk/PlusApp
            #--Configure step-------------
            CMAKE_ARGS 
                -DPLUS_DIR:PATH=${PLUS_DIR}
                -DSubversion_SVN_EXECUTABLE:FILEPATH=${Subversion_SVN_EXECUTABLE}
                -DPLUSAPP_BUILD_4DUltrasound:BOOL=ON
                -DPLUSAPP_BUILD_VolumeReconstructor:BOOL=ON
                -DPLUSAPP_BUILD_DiagnosticTools:BOOL=ON
                -DPLUSAPP_BUILD_OpenIGTLinkBroadcaster:BOOL=${PLUSBUILD_USE_OpenIGTLink}
                -DPLUSAPP_BUILD_SequenceMetaFileConverter:BOOL=ON
                -DPLUSAPP_BUILD_TrackedUltrasoundCapturing:BOOL=ON
                -DPLUSAPP_BUILD_FreehandCalibration:BOOL=ON
                -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
                -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
                -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
            #--Build step-----------------
            #--Install step-----------------
            INSTALL_COMMAND ""
            DEPENDS ${PlusApp_DEPENDENCIES}
            )
SET(PLUSAPP_DIR ${CMAKE_BINARY_DIR}/PlusApp-bin CACHE PATH "The directory containing PlusApp binaries" FORCE)                
