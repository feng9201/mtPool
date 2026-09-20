# Overlay / private-registry port for mtpool.
#
#   vcpkg install mtpool[delayed,shared] --overlay-ports=<this-repo>/ports

get_filename_component(SOURCE_PATH "${CURRENT_PORT_DIR}/../.." ABSOLUTE)
if(NOT EXISTS "${SOURCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "mtpool overlay port expects the repo root at ${SOURCE_PATH}")
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        delayed MTPOOL_ENABLE_DELAYED
)

if("shared" IN_LIST FEATURES)
    set(MTPOOL_BUILD_SHARED ON)
    set(VCPKG_LIBRARY_LINKAGE dynamic)
elseif(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    set(MTPOOL_BUILD_SHARED ON)
else()
    set(MTPOOL_BUILD_SHARED OFF)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DMTPOOL_BUILD_SHARED=${MTPOOL_BUILD_SHARED}
        -DMTPOOL_BUILD_DEMO=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME mtPool CONFIG_PATH lib/cmake/mtPool)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
