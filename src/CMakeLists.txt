project(blender)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( clashdomestg clashdomestg clashdomestg.cpp )
target_include_directories( clashdomestg PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( clashdomestg ${CMAKE_SOURCE_DIR}/../ricardian )