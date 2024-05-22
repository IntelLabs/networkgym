check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/networkgym-examples-test-suite.cc
        )
endif()

find_package(ZeroMQ REQUIRED PATHS ./)

build_lib(
    LIBNAME networkgym
    SOURCE_FILES model/data-processor.cc
                 model/southbound-interface.cc
                 helper/networkgym-helper.cc
    HEADER_FILES model/data-processor.h
                 model/southbound-interface.h
                 helper/networkgym-helper.h
    LIBRARIES_TO_LINK ${libcore}
                      ${ZeroMQ_LIBRARY}
    TEST_SOURCES test/networkgym-test-suite.cc
                 ${examples_as_tests_sources}
)

