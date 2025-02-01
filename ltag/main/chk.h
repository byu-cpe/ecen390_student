#ifndef CHK_H_
#define CHK_H_

#include "esp_err.h"

#define CHK_RET(x) ({                                            \
        esp_err_t err_rc_ = (x);                                 \
        if (unlikely(err_rc_ != ESP_OK)) {                       \
            _esp_error_check_failed_without_abort(               \
                err_rc_, __FILE__, __LINE__, __ASSERT_FUNC, #x); \
            return err_rc_;                                      \
        }                                                        \
        err_rc_;                                                 \
    })

#endif // CHK_H_
