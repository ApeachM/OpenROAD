set(TEST_LIBS
        mdm_lib
        utl_lib
        odb
        ${TCL_LIBRARY}
        OpenSTA
        dbSta_lib
        gpl
        gui
        par_lib
        dpl_lib
        gtest
        gtest_main
)
add_executable(mdmTests)
target_link_libraries(mdmTests PRIVATE ${TEST_LIBS})
target_include_directories(mdmTests
        PRIVATE
        ${OPENROAD_HOME}/include
        ${CMAKE_SOURCE_DIR}/src/mdm/include
        ${CMAKE_SOURCE_DIR}/src/mdm/src
)
target_sources(mdmTests PRIVATE testMDM.cpp)