# the following code to fetch sparsepp
# is inspired by and adapted after https://crascit.com/2015/07/25/cmake-gtest/
# download and unpack sparsepp at configure time

macro(fetch_sparsepp _download_module_path _download_root)
    set(SPARSEPP_DOWNLOAD_ROOT ${_download_root})
    configure_file(
        ${_download_module_path}/sparsepp-download.cmake
        ${_download_root}/CMakeLists.txt
        @ONLY
        )
    unset(SPARSEPP_DOWNLOAD_ROOT)

    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
        WORKING_DIRECTORY
            ${_download_root}
        )
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" --build .
        WORKING_DIRECTORY
            ${_download_root}
        )

    # adds the targers: gtest, gtest_main, gmock, gmock_main
    add_subdirectory(
        ${_download_root}/sparsepp-src
        ${_download_root}/sparsepp-build
    )
endmacro() 