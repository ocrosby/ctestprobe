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

