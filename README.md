# 🍞lunchtoast


**lunchtoast** - is a command line functional testing automation tool, written in C++17.  
It's spartan in its nature and gets the job done by launching processes and shell commands and comparing results with reference files.  

Test cases are created with simple and readable config files like this one:
```
-Name: test
-Launch: my_process --out test.res
-Assert files equal: test.res test.ref
```
Test results are presented in beautiful reports like this:
```
########################## [ 1 / 1 ] ###########################
Name: test
                                              Result:     PASSED
 
##########################  SUMMARY  ###########################
Default:                             1 out of 1 passed, 0 failed
---
Total:                               1 out of 1 passed, 0 failed
```

### Usage

`lunchtoast test.toast` - running a single test case from the config file `test.toast`  
`lunchtoast testdir` - running all recursively found test cases from the directory `testdir`


Command line options:

* **--ext=`<file extension>`** the extension of searched test files, required when specified test path is a directory,
  default value: **.toast**
* **--report=`<file path>`**  save test report to file
* **--width=`<number>`**   set test report's width in a number of characters
* **--saveContents** generate a section with contents of the test directory
* **--help** show usage info


Test case configuration syntax in 3 minutes:
```
# Hello, this config file will show you how to create lunchtoast test cases! This is a comment, by the way.
# (lunchtoast only supports commenting out the whole lines, by starting them with '#')

# The lunchtoast test case file consists of multiple sections, used to set up test parameters and actions.
# Sections are created by starting the line with '-' following by section's names and parameters, and delimiting section's value with ':'(must be on the same line as section's name). 
# -Name: Hello World
# In this example name is the section's name, 'Hello World' - is a section's value.
# (section's value is everything between ':' and the start of the next section, or the end of the file)
#
# Raw sections are sections that can have lines starting with '#' and '-' in their values. 
# Raw sections are closed by the '---' sequence, or the end of the file.
# -Write test.txt:
# Hello world
# #Yes
# -No
# ---
# In this example, 'Write' is the section's name, 'test.txt' is the section's parameter, and 3 lines between ':' and '---' are the section's value.

##########################
### PARAMETER SECTIONS ###
##########################

# Name, sets the name of the current test. The default value is the name of the test case file.
-Name: Example test

# Description, sets the text information about the test that is shown in the report for failed tests. 
-Description: This test case file describes all available parameters and actions. It has a valid configuration and should pass if you run it from the examples directory.

# Suite, sets the name of the test suite. The default value is the name of the directory where the test case file resides.
-Suite: Example suite

# Macro variables
# Three parameters described above can be used with the following macro variables:  
# ${{ DIR }} - the name of the directory where the test test case file resides and 
# ${{ FILENAME }} - the name of the test case file without the extension
# For example, they can be used like this:
# -Decription: ${{ FILENAME }}
# or
# -Suite: suite_${{ DIR }}

# Directory, sets the test directory, all non-absolute paths in this config are relative to it. 
# The default value is the directory where the test case file resides. If the value is a non-absolute path, it's relative to the default value's path.
-Directory: readme_test

#Enabled, enables test. The default value is "true". Any other value, disables the test.
-Enabled: true

#Cleanup, on succefull test run, enables cleanup of all files and directories that weren't present in the test directory before the test was started.
# The default value is false; To enable, set it to "true", any other value is treated as false.
-Cleanup: true

#Test contents, if present and not empty, modifies behaviour of the cleanup parameter. All files present in the test directory, 
# but not specified in contents are deleted before the test is started and after it finished successfuly.
# The parameter's value should be a list of files, directories and file-matching regular expression (surround with braces).
# The contents contain the by default so it can be omitted.
-Contents: {.*\.ref} readme_proc.sh

#Shell, sets the shell command that is used for launching commands (will be described lately)
#The default value is "sh -c -e"
-Shell: sh -c -e

#########################
###  ACTION SECTIONS  ###
#########################

# Launch, launches the process, waits for its completion and checks the exit code, if it differs from 0, the action is considered a failure and the test execution stops.
# Launch parameters:
#   unchecked - doesn't check the return code, the test doesn't stop if launching fails
#   command - launchs the process in system shell
#   silently - drops the standard output from the test report
# Parameters can be specified in any order and have any strings in between them, for example:
# -Launch command unchecked and silently: echo "0" > test.txt
-Launch command: readme_proc.sh > test.res

# Write, writes the section value to the file specified in the first section's parameter. 
# (note this is the raw section, you must put '---' after the last line of the section's value text)
-Write test2.res:
Hello world2
---

#Assert content of, checks the content of file, specified in the first section's parameter, with the section's value. If they differ, the action is considered a failure and the test execution stops.
# (note that this is the raw section, you must put '---' after the last line of the section's value text)
-Assert content of test.res:
Hello world
---

#Expect content of, checks the content of file, specified in the first section's parameter, with the section's value. If they differ, the test is considered a failure, but it continues to execute the next available actions.
# (note that this is the raw section, you must put '---' after the last line of the section's value text)
-Expect content of test2.res:
Hello world2
---

# Assert files equal, checks that two files specified in section's value are equal. If they differ, the action is considered a failure and the test execution stops.
-Assert files equal: test.res test.ref
# It's also possible to compare two file lists by using file matching regular expressions.
-Assert files equal: {.*\.res} {.*\.ref}

# Expect files equal, checks that two files specified in section's value are equal. If they differ, the test is considered a failure, but it continues to execute the next available actions.
-Expect files equal: test2.res test2.ref
# It's also possible to compare two file lists by using file matching regular expressions.
-Expect files equal: {.*\.res} {.*\.ref}

# That's it!
```
You can find this config at `examples/readme.toast` 

### Installation
```
git clone https://github.com/kamchatka-volcano/lunchtoast.git
cd lunchtoast
cmake -S . -B build
cmake --build build
cmake --install build
```
Dependencies:
*  Boost 1.67

### Running tests
```
cd lunchtoast
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build 
cd build/tests && ctest
```

### Running functional tests

**lunchtoast** can be tested against regression by **lunchtoast** itself. After building the development branch, use the master build of **lunchtoast** to run functional tests like this:
```
<lunchtoast_master_branch_dir>/build/lunchtoast <lunchtoast_dev_branch_dir>/functional_tests
```
