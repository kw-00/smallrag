macro(use_llama target)
    set(LLAMA_INSTALL_PATH "${CMAKE_SOURCE_DIR}/installations/llama.cpp")
    list(APPEND CMAKE_PREFIX_PATH "${LLAMA_INSTALL_PATH}")
    find_package(llama CONFIG REQUIRED PATHS ${LLAMA_INSTALL_PATH} NO_DEFAULT_PATH)
    set_property(TARGET "${target}" APPEND PROPERTY BUILD_RPATH "${LLAMA_INSTALL_PATH}/lib")
    target_link_libraries(${target} PRIVATE
        llama
    )
endmacro()
