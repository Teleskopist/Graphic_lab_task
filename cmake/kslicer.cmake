cmake_minimum_required(VERSION 3.20) # string(JSON)

include_guard(GLOBAL)

#
# Checking if this file is executed as script (cmake -P)
# Do not remove it from here because this check fails after first include()
#
if(CMAKE_SCRIPT_MODE_FILE AND NOT CMAKE_PARENT_LIST_FILE)
    set(KSLICER_SCRIPT TRUE)
else()
    set(KSLICER_SCRIPT FALSE)
    set(KSLICER_PATH "~/kernel_slicer/kslicer" CACHE STRING "Path to kernel slicer executable")
    set(KSLICER_DIR "~/kernel_slicer" CACHE STRING "Path to kernel slicer source directory")
    set(KSLICER_DEBUG FALSE CACHE BOOL "Enable extra logging")
endif()

function(_kslicer_read_config config_path prefix)
    # ------------------------------- Reading config file ------------------------------- #
    file(READ ${config_path} config)

    # ------------------------------- Parsing config ------------------------------- #
    string(JSON class GET ${config} mainClass)
    string(JSON base_dir GET ${config} baseDirectory)
    string(JSON source0 GET ${config} source 0)
    string(JSON shader_compiler GET ${config} options -shaderCC)
    string(JSON suffix GET ${config} options -suffix)

    if(KSLICER_DEBUG)
        message(STATUS "config.mainClass == \"${class}\"")
        message(STATUS "config.baseDirectory == \"${base_dir}\"")
        message(STATUS "config.source[0] ==  \"${source0}\"")
        message(STATUS "config.options[\"-shaderCC\"] == \"${shader_compiler}\"")
        message(STATUS "config.options[\"-suffix\"] == \"${suffix}\"")
    endif()

    # ----------------------------- Validating suffix ---------------------------- #
    if(NOT(suffix MATCHES "^_.*$"))
        message(WARNING "Suffix does not start with `_` at ${config_path} (config.options[\"-suffix\"] == \"${suffix}\").")
    endif()

    # -------------------------- Calculating paths -------------------------- #
    get_filename_component(config_dir ${config_path} DIRECTORY)
    set(base_dir ${config_dir}/${base_dir})
    get_filename_component(class_dir ${base_dir}/${source0} DIRECTORY)
    set(rawname ${class_dir}/${class}${suffix})
    set(shaders_dir ${class_dir}/shaders${suffix})

    if(KSLICER_DEBUG)
        message(STATUS "Config directory: ${config_dir}")
        message(STATUS "Base directory: ${base_dir}")
        message(STATUS "Class directory: ${class_dir}")
        message(STATUS "Raw name: ${rawname}")
        message(STATUS "Shaders directory: ${shaders_dir}")
    endif()

    # ------------------------------- Host sources ------------------------------- #
    set(host_sources
        ${rawname}.h
        ${rawname}.cpp
        ${rawname}_ds.cpp
        ${rawname}_init.cpp
    )

    if(KSLICER_DEBUG)
        foreach(i IN LISTS host_sources)
            message(STATUS "Host sources: ${i}")
        endforeach()
    endif()

    # ------------------------------- Build script ------------------------------- #
    if(shader_compiler STREQUAL "glsl")
        set(script build)
    elseif(shader_compiler STREQUAL "slang")
        set(script build_slang)
    elseIf(shader_compiler STREQUAL "wgpu")
        set(script build_slang)
    else()
        message(FATAL_ERROR "Unexpected value `${shader_compiler}` for config.options[\"-shaderCC\"] at ${config_path}")
    endif()

    if(WIN32)
        string(APPEND script .bat)
    else()
        string(APPEND script ".sh")
    endif()

    if(KSLICER_DEBUG)
        message(STATUS "Build script: ${script}")
    endif()

    # ----------------------------- Returning values ----------------------------- #
    set(${prefix}class ${class} PARENT_SCOPE)
    set(${prefix}suffix ${suffix} PARENT_SCOPE)
    set(${prefix}host_sources ${host_sources} PARENT_SCOPE)
    set(${prefix}shaders_dir ${shaders_dir} PARENT_SCOPE)
    set(${prefix}script ${script} PARENT_SCOPE)
endfunction()

function(kslicer_add_config config_path)
    # ----------------------------- Parsing arguments ---------------------------- #
    cmake_parse_arguments(PARSE_ARGV 1 args "TMP_DIFF_RENDER_FIX" "" "")

    if(args_UNPARSED_ARGUMENTS)
        foreach(i IN LISTS args_UNPARSED_ARGUMENTS)
            message(WARNING "Unexpected argument `${i}`")
        endforeach()
    endif()

    # ----------------------------- Full config path ----------------------------- #
    file(REAL_PATH ${config_path} config_path BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} EXPAND_TILDE)

    # ------------------------------ Reading config ------------------------------ #
    _kslicer_read_config(${config_path} config_)

    # ---------------------------- Kernel slicer path ---------------------------- #
    set(kslicer_dir ${KSLICER_DIR})
    set(kslicer ${KSLICER_PATH})

    file(REAL_PATH ${kslicer_dir} kslicer_dir EXPAND_TILDE)
    file(REAL_PATH ${kslicer} kslicer EXPAND_TILDE)

    # -------------- Creating empty stubs for configuration to pass -------------- #
    foreach(i IN LISTS config_host_sources)
        if(NOT(EXISTS ${i}))
            message(STATUS "Creating empty ${i}")
            file(WRITE ${i} "")
        endif()
    endforeach()

    # ------------------------------- Adding target ------------------------------ #
    message(STATUS "Adding custom target slice_${config_class}${config_suffix}")
    add_custom_target(
        slice_${config_class}${config_suffix}
        COMMAND ${CMAKE_COMMAND}
        -DKSLICER_DEBUG=${KSLICER_DEBUG}
        -DKSLICER_PATH=${kslicer}
        -DKSLICER_DIR=${kslicer_dir}
        -DKSLICER_CONFIG=${config_path}
        -DTMP_DIFF_RENDER_FIX=${args_TMP_DIFF_RENDER_FIX}
        -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Slicing ${config_class}${config_suffix}"
        VERBATIM
    )

    # ------------------- Adding proxy targets and dependencies ------------------ #
    if(NOT(TARGET slice_all))
        add_custom_target(slice_all)
    endif()

    if(NOT(TARGET slice_${config_class}))
        add_custom_target(slice_${config_class})
        add_dependencies(slice_all slice_${config_class})
    endif()

    add_dependencies(slice_${config_class} slice_${config_class}${config_suffix})
endfunction()

function(_kslicer_set_status status_variable value)
    if(DEFINED ${status_variable})
        set(status ${${status_variable}})
    else()
        set(status ok)
    endif()

    if(value STREQUAL ok)
    elseif(value STREQUAL warning)
        if(status STREQUAL ok)
            set(status warning)
        endif()
    elseif(value STREQUAL error)
        set(status error)
    else()
        message(FATAL_ERROR "Invalid value for status: `${value}`. Expected `ok`, `warning` or `error`.")
    endif()

    set(${status_variable} ${status} PARENT_SCOPE)
endfunction()

function(_kslicer_execute_command)
    # ----------------------------- Parsing arguments ---------------------------- #
    cmake_parse_arguments(PARSE_ARGV 0 args "" "STATUS_VARIABLE;OUTPUT_VARIABLE;LOG_FILE;WORKING_DIRECTORY" "COMMAND")

    if(args_UNPARSED_ARGUMENTS)
        foreach(i IN LISTS args_UNPARSED_ARGUMENTS)
            message(WARNING "Unexpected argument `${i}`.")
        endforeach()
    endif()

    set(status_variable ${args_STATUS_VARIABLE})
    set(output_variable ${args_OUTPUT_VARIABLE})
    set(log_file ${args_LOG_FILE})
    set(working_dir ${args_WORKING_DIRECTORY})
    set(command ${args_COMMAND})

    if(KSLICER_DEBUG)
        message(STATUS STATUS_VARIABLE=${status_variable})
        message(STATUS OUTPUT_VARIABLE=${output_variable})
        message(STATUS LOG_FILE=${log_file})
        message(STATUS WORKING_DIRECTORY="${working_dir}")
        message(STATUS COMMAND="${command}")
    endif()

    # --------------------- Making WORKING_DIRECTORY optional -------------------- #
    if(working_dir)
        set(working_dir WORKING_DIRECTORY ${working_dir})
    endif()

    # ------------------------------ Logging command ----------------------------- #
    foreach(i IN LISTS command)
        file(APPEND ${log_file} "${i} ")
    endforeach()

    file(APPEND ${log_file} "\n")

    # ----------------------------- Executing command ---------------------------- #
    execute_process(
        COMMAND ${command}
        ${working_dir}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
    )

    # ------------------------------ Logging output ------------------------------ #
    file(APPEND ${log_file} "result=${result}\n${output}\n")
    string(TOLOWER "${output}" output)

    # ---------------------------- Calculating status ---------------------------- #
    _kslicer_set_status(status ok)

    if(result STREQUAL 0)
        if(output MATCHES "^.*warning:.*$")
            _kslicer_set_status(status warning)
        endif()

        if(output MATCHES "^.*error:.*$")
            _kslicer_set_status(status error)
        endif()
    else()
        set(output ${result})
        _kslicer_set_status(status error)
    endif()

    # ----------------------------- Returning values ----------------------------- #
    set(${status_variable} ${status} PARENT_SCOPE)
    set(${output_variable} ${output} PARENT_SCOPE)
endfunction()

if(KSLICER_SCRIPT)
    if(KSLICER_DEBUG)
        message(STATUS KSLICER_DEBUG=${KSLICER_DEBUG})
        message(STATUS KSLICER_PATH=${KSLICER_PATH})
        message(STATUS KSLICER_DIR=${KSLICER_DIR})
        message(STATUS KSLICER_CONFIG=${KSLICER_CONFIG})
        message(STATUS TMP_DIFF_RENDER_FIX=${TMP_DIFF_RENDER_FIX})
    endif()

    # ------------------------------ Reading config ------------------------------ #
    _kslicer_read_config(${KSLICER_CONFIG} config_)

    # ------------------------------ Clear log file ------------------------------ #
    set(log_file ${config_shaders_dir}/log.txt)
    file(WRITE ${log_file} "")

    # -------------------------- Executing kernel slicer ------------------------- #
    _kslicer_execute_command(
        COMMAND ${KSLICER_PATH}
        -selfdir ${KSLICER_DIR}
        -config ${KSLICER_CONFIG}
        -temp_suffix ${config_suffix}
        -new_rawname 1
        -fwdfundecl 1
        STATUS_VARIABLE clang_status
        OUTPUT_VARIABLE output
        LOG_FILE ${log_file}
    )

    set(slicer_status finished)

    if(NOT(output MATCHES "^.*finished!.*$"))
        set(slicer_status error)
    endif()

    # ------------------------- Temporary diff render fix ------------------------ #
    if(TMP_DIFF_RENDER_FIX)
        if(WIN32)
            execute_process(COMMAND python scripts/dr_tmp_slicer_fix.py)
        else()
            execute_process(COMMAND python3 scripts/dr_tmp_slicer_fix.py)
        endif()
    endif()

    # ----------------------------- Building shaders ----------------------------- #
    file(STRINGS ${config_shaders_dir}/${config_script} commands)
    _kslicer_set_status(build_status ok)

    foreach(command IN LISTS commands)
        if(NOT(command MATCHES "^ *#.*$")) # Skipping comments
            separate_arguments(command NATIVE_COMMAND ${command})

            _kslicer_execute_command(
                COMMAND ${command}
                STATUS_VARIABLE build_status
                OUTPUT_VARIABLE output
                LOG_FILE ${log_file}
                WORKING_DIRECTORY ${config_shaders_dir}
            )
        endif()
    endforeach()

    message(STATUS "clang: ${clang_status}, slicer: ${slicer_status}, build: ${build_status}. Log: ${log_file}")
endif()
