cmake_minimum_required(VERSION 3.20)
project(Pepperoni VERSION 1.0 LANGUAGES CXX)

#################################### Options ####################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##################################### Paths #####################################
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

################################# Dependencies ##################################
include(FetchContent)

# - Fmt
FetchContent_Declare(fmt GIT_REPOSITORY "https://github.com/fmtlib/fmt")
FetchContent_MakeAvailable(fmt)

# - Ftxui
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY "https://github.com/ArthurSonzogni/FTXUI"
    GIT_TAG        "main"
)
FetchContent_MakeAvailable(ftxui)

# - Asio
FetchContent_Declare(asio
  GIT_REPOSITORY "git@github.com:chriskohlhoff/asio.git"
  GIT_TAG master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)

FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
  FetchContent_Populate(asio)
endif()

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)

# - Toml++
FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY "https://github.com/marzer/tomlplusplus"
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

################################## Main Target ##################################
file(GLOB PEPPERONI_SOURCES
    "src/main.cpp"
)
set(PEPPERONI_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_executable(PepperoniBin ${PEPPERONI_SOURCES})
target_link_libraries(PepperoniBin PUBLIC
    fmt
    asio
    tomlplusplus::tomlplusplus
    ftxui::dom
    ftxui::screen
    ftxui::component
)
include_directories(PepperoniBin PUBLIC ${PEPPERONI_INCLUDE_PATH})
target_compile_definitions(PepperoniBin
    PUBLIC
      $<$<CONFIG:Debug>:DEBUG>
      $<$<CONFIG:RelWithDebInfo>:DEBUG>
      $<$<CONFIG:Release>:RELEASE>
      $<$<CONFIG:MinSizeRel>:RELEASE>
)

################################## Mold Linker ##################################
find_program(MOLD_EXECUTABLE "mold")

if(MOLD_EXECUTABLE)
    message(STATUS "Found mold linker: ${MOLD_EXECUTABLE}")
    # TODO: Missing support for msvc
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
endif()

############################ Link-Time-Optimization #############################
include(CheckIPOSupported)

check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT USE_LTO_ERROR)

if(LTO_SUPPORTED)
    message(STATUS "Enabled Link Time Optimization (LTO)")
    set_property(TARGET PepperoniBin PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()