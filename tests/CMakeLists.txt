project(accounting_test)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)
message(STATUS "${CMAKE_BUILD_TYPE}")

set(EOSIO_ROOT "$ENV{EOSIO_ROOT}")
find_package(eosio REQUIRED)
string(FIND "${EOSIO_VERSION}" "${EOSIO_DEPENDENCY}" output)
if (NOT "${output}" EQUAL 0)
   message(FATAL_ERROR "Incorrect EOSIO version, please use version ${EOSIO_DEPENDENCY}.x")
else()
   message(STATUS "EOSIO_ROOT = ${EOSIO_ROOT}")
endif()

enable_testing()

configure_file(${CMAKE_SOURCE_DIR}/tests/contracts.hpp.in ${CMAKE_SOURCE_DIR}/tests/contracts.hpp)

file(GLOB UNIT_TESTS_FILES "*.cpp" "*.hpp")

add_eosio_test(accounting_test ${UNIT_TESTS_FILES})

target_compile_options(accounting_test PUBLIC -ftemplate-backtrace-limit=0)

target_include_directories(${PROJECT_NAME} 
	PUBLIC 
	${CMAKE_SOURCE_DIR}/../include
	${EOSIO_ROOT}/eosio.contracts/contracts/eosio.token/include 
)

set_target_properties(${PROJECT_NAME} 
  PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)