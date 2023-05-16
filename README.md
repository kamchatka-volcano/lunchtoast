# 🍞lunchtoast

**lunchtoast** - is a command-line functional testing tool that provides a clean and customizable way to launch
processes or shell commands and check their exit codes, console output, and contents of result files.

<p align="center">
  <img width="374" height="204" src="doc/lunchtoast.svg"/>
</p>

Test cases are created with simple configuration files, and you can use a set of built-in actions to check the expected
result of tested programs:

```
-Name: test
-Launch: my_process --out test.res
-Assert files equal: test.res test.ref
```

or define your own actions to invoke any shell command you like. For example:

```
-Check boiler #42 temperature: +54C
```

can be configured to run

```
[ $(curl http://localhost/boiler/42/temperature) == "+54C" ]
```

---

### Table of Contents

* [Test case format](#test-case-format)
  * [File structure](#file-structure)
  * [Parameter sections](#parameter-sections)
  * [Action sections](#action-sections)
* [Configuration](#configuration)
  * [Variables](#variables)
  * [User defined actions](#user-defined-actions)
* [Command line options](#command-line-options)
* [Showcase](#showcase)
* [Build instructions](#build-instructions)
* [Running unit tests](#running-unit-tests)
* [Running functional tests](#running-functional-tests)
* [License](#license)

### Test case format

A test case that can be launched by `lunchtoast` is a directory containing a test file `test.toast`:

```
my_test/
  test.toast
```

The test can be launched by simply passing its path to the `lunchtoast` executable:

```shell
kamchatka-volcano@home:~$ lunchtoast my_test/
```

Multiple tests can be launched by passing its parent directory:

```
tests_collection/
  my_test_1/
    test.toast
  my_test_2/
    test.toast   
```

```shell
kamchatka-volcano@home:~$ lunchtoast tests_collection/
```

#### File structure

Test cases in lunchtoast are written in a simple configuration file format with the `.toast` extension. A test case file
consists of multiple sections that define test parameters and actions.  
Sections are created by starting the line with `-` followed by the section's name and parameters, and delimiting the
section's value with `:`.   
The section's value is everything between `:` and the end of the line. By default, sections consist of only one line.

```
-SectionName: SectionValue
```

`lunchtoast` also supports multiline sections, which have a value that spans over multiple lines. To create a multiline
section, start a new line with `-` followed by the section's name and parameters, place a `:` delimiter, then start the
section's value on the next line. The section's value must be closed with `---` sequence or the end of the file.

```
-MultilineSectionName: 
SectionValueLine1
SectionValueLine2
---
```

Empty section values can be created using empty multiline sections:

```
-EmptySection:
---
```

or by omitting the `:` delimiter in single-line sections:

```
-EmptySection
```

#### Parameter sections

The following sections are available to set up test parameters:

- **Name**  
  Sets the name of the current test. The default value is the name of the test case directory.
  ```
  -Name: Example Test
  ```

- **Description**   
  Provides information about the test that will be displayed in the report.
  ```
  -Description: 
    Detailed info about this test
  ---
  ```

- **Suite**  
  Sets the suite of the test. This currently only affects grouping in the report.
  ```
  -Suite: smoke tests
  ```

- **Enabled**  
  Enables or disables the test. The default value is "true". To disable the test, set this to any other value.
  ```
  -Enabled: false
  ```

- **Contents**  
  Specifies the files in the test directory that should be preserved. Files not specified in this parameter are deleted
  at the start of the test and on successful completion. By default, the `test.toast` and `lunchtoast.cfg` files are
  included and do not need to be specified. Files should be specified relative to the test directory and separated by
  whitespace. Regular expressions can be used and should be surrounded with braces.
  ```
  -Contents: config.ini {.*\.txt} 
  ```

  The `Contents` section can be automatically generated based on the current state of the test directory by using
  the `saveContents` command:
  ```shell
  kamchatka-volcano@home:~$ lunchtoast saveContents my_test/
  ```

- **Tags**  
  Sets the list of tags separated by whitespace. Tags can be used to select or exclude a subset of tests by using
  the `select` and `skip` command line parameters:
  ```
  -Tags: windows network
  ```

  ```shell
  kamchatka-volcano@home:~$ lunchtoast test_collection/ -skip=windows,network
  kamchatka-volcano@home:~$ lunchtoast test_collection/ -select=network
  ```

#### Action sections

The following sections are available to set up test actions:

- **Launch**  
  The `Launch` section launches a shell command, waits for it to complete, and checks its exit code. If the exit code is
  different from 0, the action fails, and the test execution stops.
  ```
  -Launch: readme_proc.sh > test.res
  ``` 

  The default shell command is `bash -ceo pipefail`, but you can change it through the command line:
  ```shell
  kamchatka-volcano@home:~$ lunchtoast my_test/ -shell="sh -c -e"
  ```

  To launch the process directly without invoking it through the system shell, use the `Launch process` format:
  ```
  -Launch process: my_proc --write-ouput test.res
  ```

- **Assert/Expect files equal**  
  The `Assert files equal` section checks that two files specified in the section's value are equal. If they differ, the
  action fails, and the test execution stops. To continue execution on assertion failure, use the `Expect files equal`
  action:
  ```
  -Expect files equal: lhs.txt rhs.txt
  -Assert files equal: 1.res 1.ref 
  ```
  By default, files are compared in text mode, which normalizes line separators (e.g., `\n` and `\r\n` are considered
  the same).
  To compare files in binary mode, use the `Assert data files equal` format:
  ```
  -Assert data files equal: lhs.txt rhs.txt
  ```

- **Assert/Expect `<filename>`**  
  The `Assert <filename>` section compares the content of the file to the section's value. If they differ, the action
  fails, and the test execution stops. To continue execution on assertion failure, use the `Expect <filename>` action:
  ```
  -Expect 1.res: Hello world
  -Assert ouput.txt:
  Hello world
  Hello moon
  ---
  ```

- **Assert/Expect exit code**  
  The `Assert exit code` section compares the exit code of the `Launch` action to the section's value. If they differ,
  the `Launch` action fails, and the test execution stops. To continue execution on assertion failure, use
  the `Expect exit code` action:
  ```
  -Launch: my_proc --write-ouput test.res
  -Assert exit code: 1
  ```
  If the exit code of the `Launch` action is not important, you can use `any` or `*` section value to allow any exit
  code:
  ```
  -Launch: my_proc --write-ouput test.res
  -Expect exit code: any
  ```

- **Assert/Expect output**  
  The `Assert output` section compares the output of the `Launch` action to stdout to the section's value. If they
  differ, the `Launch` action fails, and the test execution stops. To continue execution on assertion failure, use
  the `Expect output` action:
  ```
  -Launch: echo "Hello world"
  -Assert output: Hello world  
  ```

- **Assert/Expect error output**  
  The `Assert error output` section compares the output of the `Launch` action to stderr to the section's value. If they
  differ, the `Launch` action fails, and the test execution stops. To continue execution on assertion failure, use
  the `Expect error output` action:
  ```
  -Launch: echo "Hello world" >&2
  -Assert error output: Hello world  
  ```

  *Note that all three assertions can be used after the `Launch` action simultaneously. However, it is not possible to
  mix the `Expect` and `Assert` assertions:*
  ```
  -Launch: my_proc --write-ouput test.res
  -Assert exit code: 1
  -Assert error output: Can't open file test.res for writing     
  ```

- **Write `<filename>`**  
  The `Write <filename>` action is used to write the section value to the file specified in the first section's
  parameter:
  ```
  -Write input.txt: Hello world
  ```

### Configuration

An optional configuration file for `lunchtoast` uses the [`shoal`](https://shoal.eelnet.org) config format and can be
specified on the command line using the `config` parameter:

```shell
kamchatka-volcano@home:~$ lunchtoast my_test/ -config=default.cfg
```

If a config file named `lunchtoast.cfg` is placed in any directory that is part of the path passed to `lunchtoast`, it
is read automatically. For example:

```
  tests_collection/
    lunchtoast.cfg
    my_test_1/
      test.toast      
    my_test_2/
      lunchtoast.cfg
      test.toast  
```

```shell
kamchatka-volcano@home:~$ lunchtoast tests_collection/
```

In this example, `my_test_1` is launched with the configuration from tests_collection/lunchtoast.cfg, while `my_test_2`
uses a merged configuration from both `tests_collection/lunchtoast.cfg` and `tests_collection/my_test_2/lunchtoast.cfg`.

#### Variables

You can define text variables in config files and use them in any section value of the test case. To use the variable
value, use the format `${{ var }}`:

```
  -Launch: myprocess ${{ args }}  
```

In the configuration file, variables are placed in the `vars` node:

```
#vars:
  args = -o result.txt
---  
```

Tagged tests can use presets of variables defined for their tags. To do this, use a node list `tagVars`, whose elements
contain a parameter `tag` and a subnode `vars`:

```
#tagVars:
###
  tag = windows
  #vars:
    args = '-shell="msys2 -c"'    
###    
  tag = linux
  #vars:
    args = '-shell=sh -ce'
---
```

With this configuration, a test tagged with `windows` tag will get a `msys` shell argument when using `${{ args }}`
variable.

#### User defined actions

`lunchtoast` provides a way to register shell commands as user actions. This allows you to make verbose commands
clearer, add domain-specific information to your tests, or implement custom comparison logic for result files.  
To register the `Check boiler #<boiler number> temperature` action shown at the beginning of this document, use a node
list `actions`, whose elements contain parameters `format` and `command`:

```
#actions:
###
  format = Check boiler #%1 temperature
  command = `[ $(curl http://localhost/boiler/%1/temperature) == "%input" ]`
```

The `format` parameter is a template of the user-defined action and can contain string variables matching a sequence of
non-whitespace characters. Variables are identified by their indices starting with 1: `%1`, `%2`, etc.

The `command` parameter specifies the shell command invoked by this action. Variables captured in the `format` command
can be used here with the same syntax: `%1`, `%2`, etc. The `%input` variable contains the section's value of the
action.

By default, the result of the command is expected to be `0`. You can control the expected result
with `checkExitCode`, `checkOutput`, and `checkErrorOutput` parameters. With them, it's possible to rewrite the same
action in the configuration file like this:

```
#actions:
###
  format = Check boiler #%1 temperature
  command = curl http://localhost/boiler/%1/temperature
  checkOutput = "%input"
```

To simplify action names with multiple variables, it is possible to register them in the `%input` variable by splitting
the section value into multiple subsections. Let's add a JSON payload to the `Check boiler #<boiler number> temperature`
command to demonstrate this:

```
-Check boiler #%1 temperature":
#payload:
{
  "access_token": "ae92f9f1-825a-453a-8924-556663c5d4b9" 
}
#expected_temperature:
+42C
---
```

To implement this in the configuration file, we use the `%input.payload` and `%input.expected_temperature` variables:

```
#actions:
###
  format = Check boiler #%1 temperature
  command = curl -d '%input.payload' http://localhost/boiler/42/temperature
  checkOutput = "%input.expected_temperature"
---
```

Subsections are created by starting the line with `#` followed by the subsection's name and parameters, placing a `:`
delimiter, and starting the section's value on the next line. The subsection's value is closed with `---`, the start of
the next subsection or the end of the file.

### Command line options

|                              |                                                                                     |
|------------------------------|-------------------------------------------------------------------------------------|
| **Arguments:**               |                                                                                     |
| `<testPath>`                 | directory containing tests                                                          |
| **Parameters:**              |                                                                                     |
| `-config=<path>`             | config file for setting variables and actions (optional)                            |
| `-shell=<string>`            | shell command (optional, default: bash -ceo pipefail)                               |
| `-listFailedTests=<path>`    | write a list of failed tests to the specified file (optional)                       |
| `-collectFailedTests=<path>` | copy directories containing failed tests to the specified path (optional)           |
| `-reportWidth=<int>`         | set the test report's width as the number of characters (optional, default: 48)     |
| `-reportFile=<path>`         | write the test report to the specified file (optional)                              |
| `-searchDepth=<int>`         | the number of descents into child directories levels for tests searching (optional) |
| `-select=<string>`           | select tests by tag names (multi-value, optional)                                   | 
| `-skip=<string>`             | skip tests by tag names (multi-value, optional)                                     |
| **Flags:**                   |                                                                                     | 
| `--withoutCleanup`           | disable cleanup of test files                                                       |
| `--help`                     | show usage info and exit                                                            |
| **Commands:**                |                                                                                     |
| `saveContents [options]`     | save the current contents of the test directory                                     |

### Showcase

- [`lunchtoast/functional_tests`](https://github.com/kamchatka-volcano/lunchtoast/tree/master/functional_tests)
- [`figcone/functional_tests`](https://github.com/kamchatka-volcano/figcone/tree/master/functional_tests)

### Build instructions

To build `lunchtoast`, you will need a C++20-compliant compiler, CMake, and the Boost.Process library installed on your
system.

```
git clone https://github.com/kamchatka-volcano/lunchtoast.git
cd lunchtoast
cmake -S . -B build
cmake --build build
```

Boost dependencies can be resolved using [`vcpkg`](https://vcpkg.io/en/getting-started.html) by running the build with
this command:

```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg path>/scripts/buildsystems/vcpkg.cmake
```

### Running unit tests

```
cd lunchtoast
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build 
cd build/tests && ctest
```

### Running functional tests

`lunchtoast` is tested for regression using the `lunchtoast` itself. Once the development branch is built, functional
tests are executed using the latest release of `lunchtoast` in the following manner:

* Linux command:

```
<lunchtoast_release>/lunchtoast <dev_branch_dir>/functional_tests -skip=windows -config=linux_vars.shoal -searchDepth=1
```

* Windows command:

```
<lunchtoast_release>/lunchtoast.exe <dev_branch_dir>/functional_tests -skip=linux -config=windows_vars.shoal -searchDepth=1
```

To run functional tests on Windows, it's recommended to use the bash shell from
the  [`msys2`](https://www.msys2.org/#installation) project. After installing it, add the following script `msys2.cmd`
to your system `PATH`:

```bat
@echo off
setlocal
IF NOT DEFINED MSYS2_PATH_TYPE set MSYS2_PATH_TYPE=inherit
set CHERE_INVOKING=1
C:\\msys64\\usr\\bin\\bash.exe -leo pipefail %*
```

### License

**lunchtoast** is licensed under the [MS-PL license](/LICENSE.md)  