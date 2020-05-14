cmake_minimum_required(VERSION 3.5)

project(accounting)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt REQUIRED)
find_package(eosio REQUIRED)

include(${EOSIO_CDT_REPO}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake)

add_contract(accounting accounting accounting.cpp)

message(STATUS "EOSIO_CDT_ROOT = ${EOSIO_CDT_ROOT}")

target_include_directories(${PROJECT_NAME} 
	PUBLIC 
	${CMAKE_SOURCE_DIR}/include
	${EOSIO_REPO}/eosio.contracts/contracts/eosio.token/include
  ${EOSIO_REPO}/eos/libraries/fc/include
  /usr/local/include/
)

set_target_properties(accounting
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/accounting"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

#target_ricardian_directory(accounting ${CMAKE_SOURCE_DIR}/../ricardian)