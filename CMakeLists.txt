cmake_minimum_required(VERSION 3.10)

# Change obs-plugintemplate to your plugin's name in a machine-readable format
# (e.g.: obs-myawesomeplugin) and set 
project(obs-text-slideshow VERSION 1.5.2)

# Replace `Your Name Here` with the name (yours or your organization's) you want
# to see as the author of the plugin (in the plugin's metadata itself and in the installers)
set(PLUGIN_AUTHOR "Joshua Wong")

# Replace `com.example.obs-plugin-template` with a unique Bundle ID for macOS releases
# (used both in the installer and when submitting the installer for notarization)
set(MACOS_BUNDLEID "com.example.obs-text-slideshow")

# Replace `me@contoso.com` with the maintainer email address you want to put in Linux packages
set(LINUX_MAINTAINER_EMAIL "jbwong05@gmail.com")

# TAKE NOTE: No need to edit things past this point

set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# In case you need C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32 OR APPLE)
include(external/FindLibObs.cmake)
endif()

find_package(LibObs REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

configure_file(
    src/plugin-macros.h.in
    ../src/plugin-macros.generated.h
)
configure_file(
    installer/installer-macOS.pkgproj.in
    ../installer/installer-macOS.generated.pkgproj
)
configure_file(
    installer/installer-Windows.iss.in
    ../installer/installer-Windows.generated.iss
)

configure_file(
    ci/ci_includes.sh.in
    ../ci/ci_includes.generated.sh
)
configure_file(
    ci/ci_includes.cmd.in
    ../ci/ci_includes.generated.cmd
)

set(PLUGIN_SOURCES
	src/plugin-main.cpp
    src/obs-text-freetype2-slideshow.cpp
    src/obs-text-slideshow.cpp
    src/obs-text-slideshow-dock.cpp
    src/files.cpp)

if(WIN32)
    set(PLUGIN_SOURCES 
        ${PLUGIN_SOURCES} 
        src/obs-text-gdiplus-slideshow.cpp)
endif()

set(PLUGIN_HEADERS
	src/plugin-macros.generated.h
    src/obs-text-slideshow.h
    src/obs-text-slideshow-dock.h
    src/files.h)

qt5_wrap_ui(PLUGIN_UI_HEADERS src/obs-text-slideshow-dock.ui)

# --- Platform-independent build settings ---
add_library(${CMAKE_PROJECT_NAME} MODULE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS} ${PLUGIN_UI_HEADERS})

include_directories(
	"${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api"
	${Qt5Core_INCLUDES}
	${Qt5Widgets_INCLUDES}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
	libobs
	Qt5::Core
	Qt5::Widgets
)

# --- End of section ---

# --- Windows-specific build settings and tasks ---
if(WIN32)
	if(NOT DEFINED OBS_FRONTEND_LIB)
		set(OBS_FRONTEND_LIB "OBS_FRONTEND_LIB-NOTFOUND" CACHE FILEPATH "OBS frontend library")
		message(FATAL_ERROR "Could not find OBS Frontend API\'s library !")
	endif()

    # Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when building with VS2019)
    if (MSVC)
        add_definitions(/MP /d2FH4-)
    endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(ARCH_NAME "64bit")
		set(OBS_BUILDDIR_ARCH "build64")
        if(MSVC)
            # Hardcoded path to w32-pthreads atm because find_library not properly finding w32-pthreads.lib
            target_link_libraries(
                ${CMAKE_PROJECT_NAME} 
                ${CMAKE_CURRENT_SOURCE_DIR}/obs-studio/build64/deps/w32-pthreads/RelWithDebInfo/w32-pthreads.lib)
        endif()
	else()
		set(ARCH_NAME "32bit")
		set(OBS_BUILDDIR_ARCH "build32")
        if(MSVC)
            # Hardcoded path to w32-pthreads atm because find_library not properly finding w32-pthreads.lib
            target_link_libraries(
                ${CMAKE_PROJECT_NAME} 
                ${CMAKE_CURRENT_SOURCE_DIR}/obs-studio/build32/deps/w32-pthreads/RelWithDebInfo/w32-pthreads.lib)
        endif()
	endif()

	include_directories(
		"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/UI"
	)

	target_link_libraries(${CMAKE_PROJECT_NAME}
		"${OBS_FRONTEND_LIB}"
    )

	# --- Release package helper ---
	# The "release" folder has a structure similar OBS' one on Windows
	set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release")

    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        # If config is Release or RelWithDebInfo, package release files
        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E make_directory
                "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )

        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        # If config is RelWithDebInfo, copy the pdb file
        COMMAND if $<CONFIG:RelWithDebInfo>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        # Copy to obs-studio dev environment for immediate testing
        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E make_directory
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )
    )
	# --- End of sub-section ---

endif()
# --- End of section ---

# --- Linux-specific build settings and tasks ---
if(UNIX OR APPLE)
    if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    endif()
endif()

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
	target_link_libraries(${CMAKE_PROJECT_NAME} obs-frontend-api)

	file(GLOB locale_files data/locale/*.ini)

    if(${BUILD_UBUNTU})
        install(TARGETS ${CMAKE_PROJECT_NAME}
            LIBRARY DESTINATION "/usr/lib/obs-plugins")
        install(FILES ${locale_files}
            DESTINATION "/usr/share/obs/obs-plugins/${CMAKE_PROJECT_NAME}/locale")
    else()
        install(TARGETS ${CMAKE_PROJECT_NAME}
            LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/obs-plugins")

        install(FILES ${locale_files}
            DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}/locale")
    endif()
endif()
# --- End of section ---

# -- OS X specific build settings and tasks --
if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fvisibility=default")

	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
	target_link_libraries(${CMAKE_PROJECT_NAME} "${OBS_FRONTEND_LIB}")
endif()
# -- End of section --
