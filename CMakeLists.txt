cmake_minimum_required (VERSION 2.6)
project (parking-server)

macro(get_WIN32_WINNT version)
    if (WIN32 AND CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REPLACE "." "" ver ${ver})
        string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})

        set(${version} "0x${ver}")
    endif()
endmacro()

macro(my_append_target_property target property str)
  get_target_property(current_property ${target} ${property})
  if(NOT current_property) # property non-existent or empty
      set_target_properties(${target} PROPERTIES ${property} ${str})
  else()
      set_target_properties(${target} PROPERTIES ${property} "${current_property} ${str}")
  endif()
endmacro(my_append_target_property)

macro(my_add_link_flags target flags)
  my_append_target_property(${target} LINK_FLAGS ${flags})
endmacro(my_add_link_flags)

if (${CMAKE_HOST_WIN32})
  get_WIN32_WINNT(ver)
  add_definitions(-D_WIN32_WINNT=${ver})
  set(CMAKE_DEBUG_POSTFIX "d")
endif()

if (${CMAKE_HOST_UNIX})
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -g")
endif()

if (${CMAKE_HOST_WIN32})
  add_definitions(-D_WEBSOCKETPP_CPP11_CHRONO_ -D_SCL_SECURE_NO_WARNINGS -DMYSQLPP_NO_DLL -DWIN32_LEAN_AND_MEAN)
endif()

# 版本号
set(MAJOR_VERSION_NUMBER 0)
set(MINOR_VERSION_NUMBER 8)
set(REVISION_NUMBER 4)

if (${CMAKE_HOST_UNIX})
  set(BUILD_NUMBER "0")
  EXECUTE_PROCESS(COMMAND sh build-version.sh
    OUTPUT_VARIABLE SH_OUTPUT
    RESULT_VARIABLE SH_RESULT
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(${SH_RESULT} STREQUAL 0)
       set(BUILD_NUMBER ${SH_OUTPUT})
    endif()

  configure_file(
  ${PROJECT_SOURCE_DIR}/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
  @ONLY)

endif()

# windows版本
if (${CMAKE_HOST_WIN32})
    set(BUILD_NUMBER "0")
    set(COMMIT_NUMBER "0")
    set(GIT_NUMBER "0")

    EXECUTE_PROCESS(COMMAND cmd /C build-version.bat
    OUTPUT_VARIABLE SH_OUTPUT
    RESULT_VARIABLE SH_RESULT
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(${SH_RESULT} STREQUAL 0)
       set(COMMIT_NUMBER ${SH_OUTPUT})
    endif()

    EXECUTE_PROCESS(COMMAND cmd /C git-version.bat
    OUTPUT_VARIABLE SH_OUTPUT
    RESULT_VARIABLE SH_RESULT
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(${SH_RESULT} STREQUAL 0)
       set(GIT_NUMBER ${SH_OUTPUT})
    endif()

    # VS添加版本信息
    configure_file(
      ${PROJECT_SOURCE_DIR}/version.rc.in
      ${CMAKE_CURRENT_BINARY_DIR}/version.rc
      @ONLY)
    set(SRCS ${SRCS} src/parking_server.rc)

    set(BUILD_NUMBER ${COMMIT_NUMBER} ${GIT_NUMBER})
    configure_file(
      ${PROJECT_SOURCE_DIR}/version.h.in
      ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

include_directories(${PROJECT_SOURCE_DIR}/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/websocketpp)
include_directories(${PROJECT_SOURCE_DIR}/jsoncpp/include)
include_directories(${PROJECT_SOURCE_DIR}/senselock/include)
include_directories(${PROJECT_SOURCE_DIR}/Boost.Application/include)
include_directories(${PROJECT_SOURCE_DIR}/Boost.Application/example)

find_package(Boost 1.56.0 COMPONENTS date_time chrono system thread regex filesystem program_options coroutine context atomic)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${PROJECT_SOURCE_DIR}/mysql++/include)
    aux_source_directory(./src SRCS)
    aux_source_directory(${PROJECT_SOURCE_DIR}/websocketpp/websocketpp SRCS)
    aux_source_directory(${PROJECT_SOURCE_DIR}/jsoncpp/dist SRCS)
    add_executable(parking-server ${SRCS})
if(${CMAKE_HOST_WIN32})
  set(MYSQL_PREFIX "C:/Program Files (x86)/MySQL/MySQL Connector C 6.1")
  set(MYSQL_LIB_DIR ${MYSQL_PREFIX}/lib)
  set(MYSQL_INCLUDE_DIR ${MYSQL_PREFIX}/include)

  include_directories(${MYSQL_INCLUDE_DIR})
  target_link_libraries(parking-server ${Boost_LIBRARIES}
    ${MYSQL_LIB_DIR}/libmysql.lib
    optimized ${PROJECT_SOURCE_DIR}/mysql++/lib/release/mysqlpp.lib
    ${PROJECT_SOURCE_DIR}/mysql++/lib/debug/mysqlpp_d.lib
	${PROJECT_SOURCE_DIR}/senselock/lib/EleCrypt.lib
	${PROJECT_SOURCE_DIR}/senselock/lib/elitee.lib)

  add_executable(parking-daemon daemon/main.cpp src/log.cpp)
  target_link_libraries(parking-daemon ${Boost_LIBRARIES})

  set(DTS_SRC test/dts_test.cpp src/device_tcp_server.cpp src/device_message.cpp
    src/log.h src/log.cpp)
  aux_source_directory(${PROJECT_SOURCE_DIR}/jsoncpp/dist DTS_SRC)

  add_executable(dts-test ${DTS_SRC})
  target_link_libraries(dts-test ${Boost_LIBRARIES})

  set(DB_SRC test/db_test.cpp src/database.cpp src/log.cpp src/options.cpp src/mysqlconnectionpool.cpp)
  aux_source_directory(${PROJECT_SOURCE_DIR}/jsoncpp/dist DB_SRC)

  add_executable(db-test ${DB_SRC})
  target_link_libraries(db-test ${Boost_LIBRARIES}
    ${MYSQL_LIB_DIR}/libmysql.lib
    optimized ${PROJECT_SOURCE_DIR}/mysql++/lib/release/mysqlpp.lib
    ${PROJECT_SOURCE_DIR}/mysql++/lib/debug/mysqlpp_d.lib)

  add_custom_target(MysqlppCopyBinaries
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/mysql++/lib/debug/mysqlpp_d.dll ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/mysql++/lib/release/mysqlpp.dll ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${MYSQL_LIB_DIR}/libmysql.dll ${CMAKE_CURRENT_BINARY_DIR})

   add_custom_target(SenselockBinaries
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/senselock/lib/EleCrypt.dll ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/senselock/lib/elitee.dll ${CMAKE_CURRENT_BINARY_DIR})

  add_dependencies(parking-server MysqlppCopyBinaries SenselockBinaries)

  my_add_link_flags(parking-server "/SAFESEH:NO")
endif()

if(${CMAKE_HOST_UNIX})
  include_directories("/usr/include/mysql")

  target_link_libraries(parking-server ${Boost_LIBRARIES} pthread mysqlclient mysqlpp usb ${PROJECT_SOURCE_DIR}/senselock/lib/libsenseELE.a ${PROJECT_SOURCE_DIR}/senselock/lib/libEleCrypt.a)

  set(CHARGERULE_SRC test/charge_test.cpp src/database.cpp src/chargerule.cpp src/bill.cpp src/log.cpp src/options.cpp src/mysqlconnectionpool.cpp src/util.cpp)
  aux_source_directory(${PROJECT_SOURCE_DIR}/jsoncpp/dist CHARGERULE_SRC)

  add_executable(chargerule-test ${CHARGERULE_SRC})
  target_link_libraries(chargerule-test ${Boost_LIBRARIES} mysqlclient mysqlpp)

endif()

endif()
