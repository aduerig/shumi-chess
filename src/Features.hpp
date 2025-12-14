
// (NOTE: these must match the values defined in Features.py)
#define _FEATURE_ENHANCED_DEPTH_TT2      0x1
#define _FEATURE_TT2            0x2
#define _FEATURE_KILLER         0x4    // requires _FEATURE_UNQUIET_SORT to be on.
#define _FEATURE_UNQUIET_SORT   0x8
#define _FEATURE_TT             0x10
#define _FEATURE_TT2_LOW_DEPTH  0x20
#define _DEFAULT_FEATURES_MASK  (_FEATURE_ENHANCED_DEPTH_TT2 | _FEATURE_TT2 | _FEATURE_KILLER | _FEATURE_UNQUIET_SORT)