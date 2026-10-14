execute_process(
    COMMAND "${MCPE_CENSUS}" chunk 0 2 0 --output "${OUTPUT_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE json
    ERROR_VARIABLE error)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "chunk export failed: ${error}")
endif()

file(SIZE "${OUTPUT_FILE}" output_size)
file(REMOVE "${OUTPUT_FILE}")

if(NOT output_size EQUAL 32768)
    message(FATAL_ERROR "chunk export was ${output_size} bytes, expected 32768")
endif()

if(NOT json MATCHES "\"generated\":true")
    message(FATAL_ERROR "chunk export did not report a generated chunk: ${json}")
endif()

if(NOT json MATCHES "\"diamonds\":10")
    message(FATAL_ERROR "seed 0 chunk 2,0 diamond count changed: ${json}")
endif()
