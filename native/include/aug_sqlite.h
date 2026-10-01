#include "aug_native.h"
#ifdef __cplusplus
extern "C" {
#endif
AUG_EXPORT int32_t aug_sqlite_open_v1(const void *, uint64_t, void **, aug_native_error_v1 *) AUG_NOEXCEPT;
AUG_EXPORT int32_t aug_sqlite_execute_v1(void *, const void *, uint64_t, const void *const *, const uint64_t *, uint64_t, int64_t *, aug_native_error_v1 *) AUG_NOEXCEPT;
AUG_EXPORT int32_t aug_sqlite_scalar_v1(void *, const void *, uint64_t, const void *const *, const uint64_t *, uint64_t, void **, uint64_t *, aug_native_error_v1 *) AUG_NOEXCEPT;
AUG_EXPORT void aug_sqlite_text_release_v1(void *) AUG_NOEXCEPT;
AUG_EXPORT void aug_sqlite_release_v1(void *) AUG_NOEXCEPT;
AUG_EXPORT int64_t aug_sqlite_live_connections_v1(void) AUG_NOEXCEPT;
#ifdef __cplusplus
}
#endif
