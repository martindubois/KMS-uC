
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Common/Version.h

// This file is not compiled. Is is there to let KMS-Build know the version.

#pragma  once

// ===== Import/Includes ====================================================
#ifdef __cplusplus
    #include <KMS/Version.h>
#endif

// Constantes
/////////////////////////////////////////////////////////////////////////////

#define VERSION_RC   1,0,3,0
#define VERSION_STR  "1.0.3.0"

#ifdef __cplusplus
    KMS_VERSION("beta")
#endif
