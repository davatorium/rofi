#!/usr/bin/env bash

tests=(
    run_errormsg_test
    run_switchdialog_test
    run_dmenu_test
    run_dmenu_custom_test
    run_run_test
    run_script_test
    run_issue_256
    run_issue_275
    run_dmenu_empty
    run_dmenu_issue_292
    run_screenshot_test
    xr_dump_test
    run_drun_test
    run_combi_test
    run_regex_test
    run_glob_test
    run_issue333_test
    help_output_test
    run_dmenu_normal_window_test
    run_window_test
)

cd ${MESON_BUILD_ROOT}
rm -f core

display=200
for test in "${tests[@]}"; do
    echo "Test ${test}"
    ${MESON_SOURCE_ROOT}/test/run_test.sh ${display} ${MESON_SOURCE_ROOT}/test/${test}.sh ${MESON_BUILD_ROOT} ${MESON_SOURCE_ROOT}
    ret=$?
    if [[ -f core ]]; then
        echo "bt" | gdb ./rofi core
        exit ${ret}
    elif [[ ${ret} != 0 ]]; then
        exit ${ret}
    fi
    display=$(( ${display} + 1 ))
done
