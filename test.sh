#!/bin/bash
for test_case in test_cases/case*.sh; do
    echo `$test_case` > /dev/null
    if [ $? = "1" ]
        then
            exit 1
    fi
done

echo OK