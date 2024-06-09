# Due to logger, we need to set below sources as the target of the test
set(OPENROAD_SOURCE
        ${CMAKE_SOURCE_DIR}/src/Design.cc
        ${CMAKE_SOURCE_DIR}/src/Timing.cc
        ${CMAKE_SOURCE_DIR}/src/Tech.cc
        ${CMAKE_SOURCE_DIR}/src/OpenRoad.cc
)

# Due to above sources, we need to set below libraries as the target of the test
set(TEST_LIBS
        ifp
        pad
        openroad_swig
        gpl
        dpl
        dpo
        fin
        rsz
        ppl
        stt
        dbSta
        OpenSTA
        odb
        odbtcl
        rcx
        rmp
        cts
        grt
        tap
        gui
        drt
        dst
        mpl
        psm
        ant
        upf
        utl
        pdn
        dft
        mdm
        ${ABC_LIBRARY}
        ${TCL_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        gtest
        gtest_main
)


add_executable(mdmTests testMDM.cpp ${OPENROAD_SOURCE} stubs.cpp)

target_compile_definitions(mdmTests PRIVATE ENABLE_READLINE)
target_include_directories(mdmTests PRIVATE ${TCL_READLINE_H})

target_link_libraries(mdmTests PRIVATE ${TEST_LIBS} ${TCL_READLINE_LIBRARY})
target_include_directories(mdmTests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/mdm/include
        ${CMAKE_SOURCE_DIR}/src/mdm/src
)
target_sources(mdmTests PRIVATE testMDM.cpp)