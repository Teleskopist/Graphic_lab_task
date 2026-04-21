include_guard()

if (STRICT_WARNINGS)
message("STRICT_WARNINGS")
set(LITERT_WARNINGS
    -Wall
    -Wshadow
    -Wno-attributes # slicer attributes like [[size("tid*channels")]] trigger this warning, but they are needed
    -Wno-sign-compare
    -Wno-missing-braces
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-result
    -Wno-deprecated-declarations
)
else()
set(LITERT_WARNINGS
    -Wall
    -Wno-unknown-warning-option
    -Wno-inconsistent-missing-override
    -Wno-attributes # slicer attributes like [[size("tid*channels")]] trigger this warning, but they are needed
    -Wno-sign-compare
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-missing-braces
    -Wno-unused-parameter
    -Wno-unused-result
    -Wno-unused-private-field
    -Wno-deprecated-declarations
)
endif()

set(LITERT_EXTRA_WARNINGS
    -Wshadow
)

add_compile_options(${LITERT_WARNINGS})

_litert_check_option(USE_EXTRA_WARNINGS)

if(USE_EXTRA_WARNINGS)
    add_compile_options(${LITERT_EXTRA_WARNINGS})
endif()

_litert_check_option(LITERT_WARNINGS_AS_ERRORS)
if (LITERT_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
endif()
