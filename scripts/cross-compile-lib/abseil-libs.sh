# Abseil library list required for gRPC/protobuf linking.

ABSEIL_LIBS="\
  -labsl_log_internal_check_op -labsl_log_initialize \
  -labsl_log_globals -labsl_log_entry -labsl_log_sink \
  -labsl_log_internal_message -labsl_log_internal_format \
  -labsl_log_internal_globals -labsl_log_internal_log_sink_set \
  -labsl_log_internal_conditions -labsl_log_internal_proto \
  -labsl_log_internal_nullguard -labsl_strings -labsl_base \
  -labsl_raw_logging_internal -labsl_spinlock_wait \
  -labsl_synchronization -labsl_status -labsl_statusor \
  -labsl_time -labsl_time_zone -labsl_cord -labsl_cord_internal \
  -labsl_cordz_functions -labsl_cordz_handle -labsl_cordz_info \
  -labsl_crc32c -labsl_crc_cord_state -labsl_crc_cpu_detect \
  -labsl_crc_internal -labsl_str_format_internal \
  -labsl_strings_internal -labsl_string_view \
  -labsl_int128 -labsl_hash -labsl_raw_hash_set \
  -labsl_hashtablez_sampler -labsl_exponential_biased \
  -labsl_bad_optional_access -labsl_bad_variant_access \
  -labsl_malloc_internal -labsl_debugging_internal \
  -labsl_demangle_internal -labsl_stacktrace -labsl_symbolize \
  -labsl_strerror -labsl_throw_delegate"