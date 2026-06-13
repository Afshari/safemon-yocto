# Redis
find_library(HIREDIS_LIB hiredis REQUIRED)
find_path(HIREDIS_INCLUDE hiredis/hiredis.h REQUIRED)

# gRPC / Protobuf
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl REQUIRED)

# DRM / EGL / GLES
find_library(DRM_LIB drm REQUIRED)
find_path(DRM_INCLUDE xf86drm.h REQUIRED)
find_library(EGL_LIB EGL REQUIRED)
find_library(GLES2_LIB GLESv2 REQUIRED)

set(ABSEIL_LIBS
    absl::log_internal_check_op
    absl::log_initialize
    absl::log_globals
    absl::log_entry
    absl::log_sink
    absl::log_internal_message
    absl::log_internal_format
    absl::log_internal_globals
    absl::log_internal_log_sink_set
    absl::log_internal_conditions
    absl::log_internal_proto
    absl::log_internal_nullguard
    absl::strings
    absl::base
    absl::raw_logging_internal
    absl::spinlock_wait
    absl::synchronization
    absl::status
    absl::statusor
    absl::time
    absl::time_zone
)