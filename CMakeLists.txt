# CMakeList.txt : CMake project for DiscordHookTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("DiscordHookTest")

option (BUILD_EXECUTABLE "Build as executable instead of lib" ON)

# Currently building dll version
set (BUILD_EXECUTABLE OFF)

file(GLOB_RECURSE EXEC_SOURCES
	"executable_ver/*.h"
	"executable_ver/*.cpp"
)

file(GLOB_RECURSE LIB_SOURCES
	"library_ver/*.h"
	"library_ver/*.cpp"
)

file(GLOB_RECURSE GLOBAL_FILES
	"global_files/*.h"
	"global_files/*.cpp"
)

# Don't forget about the global file:
# "Globals.h"
if(BUILD_EXECUTABLE)
    add_executable(${PROJECT_NAME} ${EXEC_SOURCES} ${GLOBAL_FILES})
    target_include_directories(${PROJECT_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/executable_ver)
else()
    add_library(${PROJECT_NAME} SHARED ${LIB_SOURCES} ${GLOBAL_FILES})
    target_include_directories(${PROJECT_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/library_ver)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DiscordHookTest PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
