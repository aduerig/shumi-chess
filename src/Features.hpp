
// (NOTE: these must match the values defined in Features.py)
#define _FEATURE_ENHANCED_DEPTH_TT2      0x1
#define _FEATURE_TT2            0x2
#define _FEATURE_KILLER         0x4    // requires _FEATURE_UNQUIET_SORT to work.
#define _FEATURE_UNQUIET_SORT   0x8
#define _FEATURE_TT             0x10
#define _FEATURE_DELTA_PRUNE    0x20
#define _FEATURE_EVAL_TEST1     0x40
#define _DEFAULT_FEATURES_MASK  (_FEATURE_ENHANCED_DEPTH_TT2 | _FEATURE_TT2 | _FEATURE_KILLER | _FEATURE_UNQUIET_SORT)