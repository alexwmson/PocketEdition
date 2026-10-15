execute_process(
    COMMAND "${MCPE_CENSUS}" --help
    RESULT_VARIABLE help_result
    OUTPUT_VARIABLE help_text
    ERROR_VARIABLE help_error)

if(NOT help_result EQUAL 0)
    message(FATAL_ERROR "help failed: ${help_error}")
endif()

if(NOT help_text MATCHES "--range <start>-<end>")
    message(FATAL_ERROR "help does not document count --range: ${help_text}")
endif()

execute_process(
    COMMAND "${MCPE_CENSUS}" count --range 9-0
    RESULT_VARIABLE descending_result
    OUTPUT_VARIABLE descending_output
    ERROR_VARIABLE descending_error)

if(descending_result EQUAL 0)
    message(FATAL_ERROR "descending range should fail: ${descending_output}")
endif()

if(NOT descending_error MATCHES "range start must not exceed range end")
    message(FATAL_ERROR "unexpected descending-range error: ${descending_error}")
endif()

execute_process(
    COMMAND "${MCPE_CENSUS}" count --range
    RESULT_VARIABLE missing_result
    ERROR_VARIABLE missing_error)

if(missing_result EQUAL 0)
    message(FATAL_ERROR "missing range spec should fail")
endif()

if(NOT missing_error MATCHES "count --range expects")
    message(FATAL_ERROR "unexpected missing-range error: ${missing_error}")
endif()

execute_process(
    COMMAND "${MCPE_CENSUS}" count --range 0-999999999999
    RESULT_VARIABLE overflow_result
    ERROR_VARIABLE overflow_error)

if(overflow_result EQUAL 0)
    message(FATAL_ERROR "oversized range end should fail")
endif()

if(NOT overflow_error MATCHES "signed 32-bit range")
    message(FATAL_ERROR "unexpected oversized-range error: ${overflow_error}")
endif()

execute_process(
    COMMAND "${MCPE_CENSUS}" count 0 --range 1-2
    RESULT_VARIABLE both_result
    ERROR_VARIABLE both_error)

if(both_result EQUAL 0)
    message(FATAL_ERROR "seed and range together should fail")
endif()

if(NOT both_error MATCHES "more than one seed range")
    message(FATAL_ERROR "unexpected combined-seed error: ${both_error}")
endif()
