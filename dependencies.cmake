include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if (${LANDSTALKER_BUILD_SHARED})
    set(LANDSTALKER_BUILD_STATIC OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    message(STATUS "Configuring dependencies for shared library build")
else()
    set(LANDSTALKER_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    message(STATUS "Configuring dependencies for static library build")
endif()

function(UpdateSubmodules)
    find_package(Git QUIET)
    if(GIT_FOUND)
        message(STATUS "Git found: initializing/updating submodules...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE _git_submodule_result
            OUTPUT_QUIET ERROR_QUIET
        )
        if(NOT _git_submodule_result EQUAL 0)
            message(WARNING "git submodule update --init --recursive failed with exit code ${_git_submodule_result}")
        endif()
    else()
        message(STATUS "Git not found; skipping automatic git submodule initialization")
    endif()
endfunction()

function(InstallZlib)
    message("Fetching ZLIB sources...")
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG "5a82f71ed1dfc0bec044d9702463dbdf84ea3b71" # "master"
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_SHARED ${LANDSTALKER_BUILD_SHARED} CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_STATIC ${LANDSTALKER_BUILD_STATIC} CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_MINIZIP OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    message("Configuring ZLIB...")
    FetchContent_MakeAvailable(zlib)
    add_library(ZLIB::ZLIB ALIAS zlib)
    set(ZLIB_ROOT "${zlib_SOURCE_DIR}" CACHE INTERNAL "Root for the zlib library, used by libpng")
    set(ZLIB_LIBRARY zlib CACHE INTERNAL "" FORCE)
    set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE INTERNAL "" FORCE)
endfunction()

function(InstallLibpng)
    message("Fetching LIBPNG sources...")
    FetchContent_Declare(
        png
        GIT_REPOSITORY https://github.com/pnggroup/libpng.git
        GIT_TAG "2b978915d82377df13fcbb1fb56660195ded868a" # "v1.6.50"
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    set(PNG_SHARED ${LANDSTALKER_BUILD_SHARED} CACHE BOOL "" FORCE)
    set(PNG_STATIC ${LANDSTALKER_BUILD_STATIC} CACHE BOOL "" FORCE)
    set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
    set(PNG_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
    set(PNG_TESTS OFF CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
    message("Configuring LIBPNG...")
    FetchContent_MakeAvailable(png)
endfunction()

function(InstallPugixml)
    message("Fetching Pugixml sources...")
    FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml
        GIT_TAG "ee86beb30e4973f5feffe3ce63bfa4fbadf72f38" # "v1.15"
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    message("Configuring Pugixml...")
    FetchContent_MakeAvailable(pugixml)
endfunction()

function(InstallYamlcpp)
    message("Fetching YAML-CPP sources...")
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG "a83cd31548b19d50f3f983b069dceb4f4d50756d" # "master"
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    set(YAML_CPP_BUILD_CONTRIB ON CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_BUILD_SHARED_LIBS ${LANDSTALKER_BUILD_SHARED} CACHE BOOL "" FORCE)
    set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
    set(YAML_USE_SYSTEM_GTEST OFF CACHE BOOL "" FORCE)
    set(YAML_ENABLE_PIC ${LANDSTALKER_BUILD_SHARED} CACHE BOOL "" FORCE)
    message("Configuring YAML-CPP...")
    FetchContent_MakeAvailable(yaml-cpp)
endfunction()

function(InstallBoost)
    message("Fetching Boost sources...")
    FetchContent_Declare(
        boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG "ef7fea34711a189472893b88205b1dd3c275677b" # "boost-1.89.0"
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    message("Configuring Boost...")
    FetchContent_MakeAvailable(boost)
endfunction()

function(InstallWxwidgets)
    message("Fetching wxWidgets sources...")
    FetchContent_Declare(
        wx
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG "49c6810948f40c457e3d0848b9111627b5b61de5" # "wxWidgets-3.3.1"
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    set(wxBUILD_SHARED ${LANDSTALKER_BUILD_SHARED} CACHE BOOL "" FORCE)
    set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    message("Configuring wxWidgets...")
    FetchContent_MakeAvailable(wx)
endfunction()

function(InstallDependencies)
    IF(${INSTALL_DEPS})
        InstallZlib()
        InstallLibpng()
        InstallPugixml()
        InstallYamlcpp()
        # InstallBoost()
        InstallWxwidgets()
    else()
        message(STATUS "Skipping dependency installation as per user request")
        find_package(wxWidgets REQUIRED core base adv xrc propgrid aui xml)
        find_package(ZLIB CONFIG REQUIRED)
        find_package(PNG CONFIG REQUIRED)
        find_package(yaml-cpp CONFIG REQUIRED)
        find_package(PUGI CONFIG REQUIRED)
        find_package(Boost CONFIG REQUIRED)
        include(${wxWidgets_USE_FILE})
    endif()
endfunction()
