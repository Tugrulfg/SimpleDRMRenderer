cmake_minimum_required(VERSION 3.22)
project(SimpleDRMRenderer C)

# Enable warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Find required packages
find_package(PkgConfig REQUIRED)

pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(GBM REQUIRED gbm)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLESv2 REQUIRED glesv2)

# Include dirs
include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

# Add your source files here
set(SOURCES
    example.c
    Helpers/Renderer_helpers.c
    Helpers/GL_helpers.c
)

# Build executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${DRM_LIBRARIES}
    ${GBM_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GLESv2_LIBRARIES}
)

# Make sure pkg-config libs are found at runtime
target_link_directories(${PROJECT_NAME} PRIVATE
    ${DRM_LIBRARY_DIRS}
    ${GBM_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${GLESv2_LIBRARY_DIRS}
)

