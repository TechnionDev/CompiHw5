import glob
import os
import subprocess

RED='\033[1;31m'
GREEN='\033[0;32m'
NC='\033[0m'

MAIN_TESTS_FOLDER = "./tests/"

def clear_row():
    move_one_row_up_proc = subprocess.Popen(["tput", "cuu1"])
    move_one_row_up_proc.communicate()
    clear_the_line_proc = subprocess.Popen(["tput", "el"])
    clear_the_line_proc.communicate()

# Build compiler
make_process = subprocess.Popen("make")
stdout, stderr = make_process.communicate()
if stderr is not None:
    print(f"{RED}BUILD FAILED!{NC}")
    exit(0)
print()

# Run tests
total_tests_count = 0
successful_tests_count = 0
failed_tests_count = 0
failed_tests = []

for root, _, _ in os.walk(f"{MAIN_TESTS_FOLDER}"):
    tests_in = glob.glob(f"{root}/*.in")
    for test_in in tests_in:

        test_in_full_filename = test_in
        test_in_filename = test_in.replace(MAIN_TESTS_FOLDER, '')
        test_in_basename = os.path.basename(test_in)

        total_tests_count += 1
        print(f"Running Test Number {total_tests_count} - {test_in_filename}")
        matched_test_out = glob.glob(f"{root}/{test_in_basename.replace('.in', '.out')}")
        if len(matched_test_out) == 0:
            print(f"{RED}{test_in_filename} doesn't have matched {test_in_filename.replace('.in', '.out')}{NC}")
            continue

        test_out = matched_test_out[0]

        # fix windows \r\n to unix \n
        with open(test_out, 'rb') as test_out_file:
            content = test_out_file.read()
        content = content.replace(b'\r\n', b'\n')
        with open(test_out, 'wb') as test_out_file:
            test_out_file.write(content)


        # generate compiled llvm file for test.in
        llvm_filename = f"{root}/{test_in_basename.replace('.in', '.our.ll')}"
        fanC_file = open(test_in)
        llvm_file = open(llvm_filename, 'w')

        compile_process = subprocess.Popen("./hw5", stdin=fanC_file, stdout=llvm_file, stderr=subprocess.PIPE)
        _, compile_stderr = compile_process.communicate()

        fanC_file.close()
        llvm_file.close()

        # run lli on generated file
        fanC_out_filename = f"{root}/{test_in_basename.replace('.in', '.our.out')}"
        llvm_file = open(llvm_filename)
        fanC_out_file = open(fanC_out_filename, 'w')

        llvm_process = subprocess.Popen("lli", stdin=llvm_file, stdout=fanC_out_file, stderr=subprocess.PIPE)
        _, llvm_stderr = llvm_process.communicate()

        llvm_file.close()
        fanC_out_file.close()

        # check if there are differences between the test.our.out to the test.out
        diff_process = subprocess.Popen(["diff", "--minimal", fanC_out_filename, test_out], stdout=subprocess.PIPE)
        diff_stdout, _ = diff_process.communicate()

        if len(diff_stdout) == 0:
            successful_tests_count += 1
            files_to_remove = glob.glob(f"{root}/{test_in_basename.replace('.in', '.our.*')}")
            for file in files_to_remove:
                os.remove(file)
            clear_row()
        else:
            failed_tests_count += 1
            failed_tests.append(test_in_filename)
            if len(compile_stderr):
                print(compile_stderr.decode())
            elif len(llvm_stderr):
                print(llvm_stderr.decode())
            else:
                print(diff_stdout.decode())



if failed_tests_count == 0:
    print(f"{GREEN}ALL TEST PASSED ({successful_tests_count}/{total_tests_count}){NC}")
else:
    print(f"{RED}TEST FAILED:{NC}")
    for test_filename in failed_tests:
        print(test_filename)
    print(f"{GREEN}PASSED ({successful_tests_count}/{total_tests_count}){NC}")
    print(f"{RED}FAILED ({failed_tests_count}/{total_tests_count}){NC}")
    print(f"You can see failed output at testname.our.*")
