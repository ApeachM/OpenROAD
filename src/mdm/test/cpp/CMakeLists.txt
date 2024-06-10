# Due to logger, we need to set below sources as the target of the test
set(OPENROAD_SOURCE
        ${CMAKE_SOURCE_DIR}/src/Design.cc
        ${CMAKE_SOURCE_DIR}/src/Timing.cc
        ${CMAKE_SOURCE_DIR}/src/Tech.cc
        ${CMAKE_SOURCE_DIR}/src/OpenRoad.cc
)

set(TOOL_HEADER_DIR
        ${CMAKE_SOURCE_DIR}/src/ifp/include
        ${CMAKE_SOURCE_DIR}/src/pad/include
        ${CMAKE_SOURCE_DIR}/src/openroad_swig/include
        ${CMAKE_SOURCE_DIR}/src/gpl/include
        ${CMAKE_SOURCE_DIR}/src/dpl/include
        ${CMAKE_SOURCE_DIR}/src/dpo/include
        ${CMAKE_SOURCE_DIR}/src/fin/include
        ${CMAKE_SOURCE_DIR}/src/rsz/include
        ${CMAKE_SOURCE_DIR}/src/ppl/include
        ${CMAKE_SOURCE_DIR}/src/stt/include
        ${CMAKE_SOURCE_DIR}/src/dbSta/include
        ${CMAKE_SOURCE_DIR}/src/OpenSTA/include
        ${CMAKE_SOURCE_DIR}/src/odb/include
        ${CMAKE_SOURCE_DIR}/src/odbtcl/include
        ${CMAKE_SOURCE_DIR}/src/rcx/include
        ${CMAKE_SOURCE_DIR}/src/rmp/include
        ${CMAKE_SOURCE_DIR}/src/cts/include
        ${CMAKE_SOURCE_DIR}/src/grt/include
        ${CMAKE_SOURCE_DIR}/src/tap/include
        ${CMAKE_SOURCE_DIR}/src/gui/include
        ${CMAKE_SOURCE_DIR}/src/drt/include
        ${CMAKE_SOURCE_DIR}/src/dst/include
        ${CMAKE_SOURCE_DIR}/src/mpl/include
        ${CMAKE_SOURCE_DIR}/src/psm/include
        ${CMAKE_SOURCE_DIR}/src/ant/include
        ${CMAKE_SOURCE_DIR}/src/upf/include
        ${CMAKE_SOURCE_DIR}/src/utl/include
        ${CMAKE_SOURCE_DIR}/src/pdn/include
        ${CMAKE_SOURCE_DIR}/src/dft/include

)

# Due to above sources, we need to set below libraries as the target of the test
set(TEST_LIBS
        #        ifp
        #        pad
        #        openroad_swig
        gpl
        dpl
        dpo
        #        fin
        #        rsz
        #        ppl
        #        stt
        dbSta
        OpenSTA
        odb
        #        odbtcl
        #        rcx
        #        rmp
        #        cts
        #        grt
        #        tap
        gui
        #        drt
        #        dst
        #        mpl
        #        psm
        #        ant
        #        upf
        #        utl
        #        pdn
        #        dft
        mdm
        #        ${ABC_LIBRARY}
        #        ${TCL_LIBRARY}
        #        ${CMAKE_THREAD_LIBS_INIT}
        gtest
        gtest_main
)


add_executable(mdmTests testMDM.cpp ${OPENROAD_SOURCE} stubs.cpp)

target_compile_definitions(mdmTests PRIVATE ENABLE_READLINE)
target_include_directories(mdmTests PRIVATE ${TCL_READLINE_H} ${TOOL_HEADER_DIR})

target_link_libraries(mdmTests PRIVATE ${TEST_LIBS} ${TCL_READLINE_LIBRARY})
target_include_directories(mdmTests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/mdm/include
        ${CMAKE_SOURCE_DIR}/src/mdm/src
)
target_sources(mdmTests PRIVATE testMDM.cpp)