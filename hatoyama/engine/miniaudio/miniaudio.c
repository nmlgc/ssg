/*
 *   miniaudio compilation
 *
 */

// miniaudio includes <stdio.h> for the printf() family, which we'd like to
// redirect to the one implementation we use across all platforms. Since we
// soft-alias `printf` to `printf_`, we need to:

// 1) Include our flags first, since miniaudio defines opt-in macros for the
//    supported backends in its header part
#include "engine/miniaudio/flags.h"

// 2) Override miniaudio's own inclusion of <stdio.h> by including the header
//    ourselves first. This allows us to override its declaration of the libc's
//    `printf()`
#include <stdio.h>

// 3) Include only miniaudio's declarations to avoid changing the `printf` in
//    `__attribute__((format(printf)))`, which would otherwise throw `-Wformat`
//    on GCC
#include <libs/miniaudio/miniaudio.h>

// 4) Activate the soft alias
#include <printf/printf.h>

// 5) Include the actual miniaudio implementation
#include <libs/miniaudio/miniaudio.c>
