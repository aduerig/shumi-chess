
# Duplicate of C++ feature flags
#  these must match the default defined in Features.hpp
_FEATURE_FULL_EVAL    = 0x1
_FEATURE_TT2          = 0x2
_FEATURE_KILLER       = 0x4    # requires _FEATURE_UNQUIET_SORT to be on.
_FEATURE_UNQUIET_SORT = 0x8
_FEATURE_TT           = 0x10
_DEFAULT_FEATURES_MASK = (_FEATURE_FULL_EVAL | _FEATURE_TT2 | _FEATURE_KILLER | _FEATURE_UNQUIET_SORT)