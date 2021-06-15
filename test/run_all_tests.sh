#!/usr/bin/env bash

tests=(
    run_errormsg_test
    run_switchdialog_test
    run_dmenu_test
    run_run_test
    run_script_test
    run_script_meta_test
    run_issue_256
    run_issue_275
    run_dmenu_empty
    run_dmenu_issue_292
    run_screenshot_test
    run_combi_test
    run_regex_test
    run_glob_test
    run_issue333_test
    help_output_test
    default_theme_test
    convert_old_theme_test
    run_dmenu_normal_window_test
    run_window_test
)

cd ${MESON_BUILD_ROOT}
mkdir -p test-x-logs
rm -f core
display=200
for test in "${tests[@]}"; do
    log_prefix=test-x-logs/${display}
    echo -n "Test ${test}: "
    ${MESON_SOURCE_ROOT}/test/run_test.sh ${display} ${log_prefix} ${MESON_SOURCE_ROOT}/test/${test}.sh ${MESON_BUILD_ROOT} ${MESON_SOURCE_ROOT} &> ${log_prefix}-wrapper.log
    ret=$?
    if [[ -f core ]]; then
        echo "COREDUMP"
        echo "bt" | gdb ./rofi core
        more ${log_prefix}*.log | cat
        exit ${ret}
    elif [[ ${ret} != 0 ]]; then
        echo "FAIL"
        more ${log_prefix}*.log | cat
        exit ${ret}
    fi
    echo "PASS"
    display=$(( display + 1 ))
done
