add_library(botindex_test_support STATIC
    ../src/chain.c
    ../src/config.c
    ../src/log.c
    ../src/util.c
    ../third_party/tomlc99/toml.c
)

target_include_directories(botindex_test_support PUBLIC
    ../include
    ../third_party/tomlc99
)

function(add_botindex_test name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} PRIVATE botindex_test_support)
    target_compile_definitions(${name} PRIVATE BOTINDEX_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_botindex_test(test_chain)
add_botindex_test(test_config)
add_botindex_test(test_util)
