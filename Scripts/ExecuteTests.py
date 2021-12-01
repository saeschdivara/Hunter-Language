import os
import subprocess

files_to_test = [
    ('./Examples/first-const-string.hunt', './Examples/first-const-string.hunt.txt'),
    ('./Examples/for-loop.hunt', './Examples/for-loop.hunt.txt'),
    ('./Examples/function-calls-with-parameters.hunt', './Examples/function-calls-with-parameters.hunt.txt'),
    ('./Examples/function-calls-without-parameters.hunt', './Examples/function-calls-without-parameters.hunt.txt'),
    ('./Examples/hello-world.hunt', './Examples/hello-world.hunt.txt'),
    ('./Examples/if-else-conditions.hunt', './Examples/if-else-conditions.hunt.txt'),
    ('./Examples/print-string-const.hunt', './Examples/print-string-const.hunt.txt'),
    ('./Examples/simple-if-check.hunt', './Examples/simple-if-check.hunt.txt'),
    ('./Examples/single-line-comments.hunt', './Examples/single-line-comments.hunt.txt'),
    ('./Examples/string-compare.hunt', './Examples/string-compare.hunt.txt'),
    ('./Examples/use-all-int-types.hunt', './Examples/use-all-int-types.hunt.txt'),
    ('./Examples/while-loop.hunt', './Examples/while-loop.hunt.txt'),
]

path_to_compiler = './cmake-build-debug/bin/Hunter_Compiler'

for hunt_file, hunt_test_output in files_to_test:
    if not os.path.exists(hunt_test_output):
        ret_code = subprocess.call(f'{path_to_compiler} {hunt_file} --output-ir {hunt_test_output}'.split(' '))
        if ret_code != 0:
            print('Compilation failed')
            exit(1)