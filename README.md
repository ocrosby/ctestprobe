# ctestprobe

A unit testing framework for C.

## Motivation

Whenever I end up writing C code after having worked in more modern languages, I find myself 
missing the convenience of unit testing frameworks. I wanted to write a simple unit testing 
framework for C that would allow me to write tests in a way that is similar to how I would 
write tests in a language like Python or JavaScript.

The difficulty in writing a unit testing framework for C is that C does not have reflection,
which means that you can't easily iterate over all of the functions in a program. This means
that you have to manually register each test function with the testing framework. This is
a bit of a pain, but it's not too bad.

## Structure

Designing a unit testing framework that can be easily utilized from other repositories involves creating a library that contains the testing framework, and then distributing this library so that it can be included in other projects. Here's a high-level overview of how you could design such a framework:

1. **Create a Static Library**: The core of the unit testing framework will be a static library. This library will contain all the functions and macros needed to define and run tests. In your case, you already have a Makefile that compiles your code into a static library (`libctestprobe.a`).

2. **Design the Testing API**: The library provides a simple API for defining tests. This could be as simple as a function that takes a test name and a function pointer to the test code. You might also provide setup and teardown functions that are run before and after each test.

3. **Provide a Test Runner**: You'll need a way to run all the tests that have been defined. This could be a function in your library that runs all registered tests and reports the results. Alternatively, you could provide a separate test runner executable that links against your library.

4. **Distribute Your Library**: Once your library is built, you'll need to distribute it so that other projects can use it. This could be as simple as providing a download link for the compiled library and headers, or you could create a package for a package manager like `apt` or `brew`.

5. **Provide Documentation and Examples**: To make it easy for other developers to use the testing framework, you should provide thorough documentation and examples. This could be in the form of a `README` file, a wiki, or a full-blown website.

6. **Automate the Build Process**: To make it easy for other developers to build and run tests, you should provide scripts that automate the build process. This could be a simple shell script, or you could use a build system like `CMake`.

Here's an example of how you might define a simple test in your library:

```c
#define TEST(name) void name(void)

typedef void (*TestFunc)(void);

void register_test(const char *name, TestFunc func);
void run_all_tests(void);
```

And here's how you might use this API to define and run a test:

```c
#include "ctestprobe.h"

TEST(my_test) {
    // Test code here
}

int main(void) {
    register_test("my_test", my_test);
    run_all_tests();
    return 0;
}
```

This is a very basic example, and a real testing framework would likely be more 
complex. However, it should give us a good starting point for designing the 
framework.


