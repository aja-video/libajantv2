project(ut_ajantv2)

set(TARGET_INCLUDE_DIRS
	${APP_ROOT}/..
	${AJA_LIBRARIES_ROOT}
	${NON_AJA_ROOT}
	${NON_AJA_ROOT}/argparse
	${NON_AJA_ROOT}/doctest
	${NON_AJA_ROOT}/rapidcsv)

set(ARGPARSE_SOURCES
	${NON_AJA_ROOT}/argparse/argparse.h
	${NON_AJA_ROOT}/argparse/argparse.c)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
	find_library(CORE_SERVICES_FRAMEWORK CoreServices)
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(TARGET_LINK_LIBS
		${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
endif()

add_executable(ut_ajantv2
	ut_ajantv2.cpp)
add_dependencies(ut_ajantv2 ajantv2)
target_include_directories(ut_ajantv2 PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ajantv2 PUBLIC ajantv2 ${TARGET_LINK_LIBS})
install(TARGETS ut_ajantv2
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

find_package(GTest)
if (AJA_BUILD_QA AND GTest_FOUND)
	option(NTV2CARD_TEST_USE_PNG "Enable PNG screenshots of the framebuffer under test?" ON)
	option(NTV2CARD_TEST_USE_WFM "Enable reading Tektronix WFM8300 via SNMP++ library?" OFF)
	set(TARGET_DEPS ajaconv ajantv2 ntv2qa GTest::gtest)
	set(TARGET_LINK_LIBS ajaconv ajantv2 ntv2qa GTest::gtest)

	# PNG support
	if (NTV2CARD_TEST_USE_PNG)
		find_package(PNG)
		if (PNG_FOUND)
			set(TARGET_INCLUDE_DIRS ${TARGET_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
			set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} PNG::PNG)
			set(TARGET_COMPILE_DEFS ${TARGET_COMPILE_DEFS} -DUSE_PNG)
		endif()
	endif()

	# SNMP++ support
	if (NTV2CARD_TEST_USE_WFM)
		set(TARGET_COMPILE_DEFS ${TARGET_COMPILE_DEFS} -DUSE_WFM)
		set(TARGET_DEPS
			${TARGET_DEPS}
			snmp++)
		set(TARGET_INCLUDE_DIRS
			${TARGET_INCLUDE_DIRS}
			${NON_AJA_ROOT}/snmp++-3.4.10
			${NON_AJA_ROOT}/snmp++-3.4.10/include)
		set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} snmp++)
	endif()
	add_executable(ntv2card_test
		${AJA_LIB_NTV2_ROOT}/utilityfiles/public/ntv2pixelcomponent.h
		${AJA_LIB_NTV2_ROOT}/utilityfiles/public/ntv2pixelcomponent.cpp
		ntv2card_test.cpp
		test_support.hpp)
	add_dependencies(ntv2card_test ${TARGET_DEPS})
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(TARGET_COMPILE_DEFS ${TARGET_COMPILE_DEFS} -DAJACONV_USE_OPENMP)
		set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} dl pthread rt gomp)
	endif()
	target_compile_definitions(ntv2card_test PRIVATE ${TARGET_COMPILE_DEFS})
	target_compile_features(ntv2card_test PUBLIC cxx_std_11)
	set_target_properties(ntv2card_test PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS ON)
	target_include_directories(ntv2card_test PRIVATE
		${TARGET_INCLUDE_DIRS}
		${NON_AJA_ROOT}/gtest/googletest/include)
	target_link_libraries(ntv2card_test PRIVATE ${TARGET_LINK_LIBS})
	set(ntv2card_test_JSON
		json/sdi_validated.json
		json/sdi_not_validated.json)
	foreach(j IN LISTS ntv2card_test_JSON)
		post_build_copy_file(ntv2card_test
			${CMAKE_CURRENT_SOURCE_DIR}/${j}
			$<TARGET_FILE_DIR:${PROJECT_NAME}>/${j})
	endforeach()
	install(FILES ${ntv2card_test_JSON} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/json)
	install(TARGETS ntv2card_test
		BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

add_executable(ut_ntv2bitfile
	${ARGPARSE_SOURCES}
	ut_ntv2bitfile.cpp
	test_support.hpp)
add_dependencies(ut_ntv2bitfile ajantv2)
target_include_directories(ut_ntv2bitfile PRIVATE ${TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ntv2bitfile PRIVATE ajantv2 ${TARGET_LINK_LIBS})
post_build_copy_file(ut_ntv2bitfile
	${CMAKE_CURRENT_SOURCE_DIR}/json/ntv2_classic_firmware.json
	$<TARGET_FILE_DIR:${PROJECT_NAME}>/json/ntv2_classic_firmware.json)
install(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/json/ntv2_classic_firmware.json
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/json)
install(TARGETS ut_ntv2bitfile
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
