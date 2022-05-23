# Big Number Arithmetic (and possibly algorithms) in C

This project is an implementation of *unsigned* big number arithmetic. Please
note that this implementation is only an exploration of big number arithmetic.
There are most likely more mature options for production use.

Big Numbers should support the following operations:

- [x] Add
- [x] Subtract (assuming positive result)
- [x] Multiply
- [x] Divide
- [x] Modulo
- [x] Power with Modulo (a^b mod c)
- [x] Comparison (greater than, less than, equal to)
- [x] Print as hex
- [x] Creation from uint32_t
- [x] Creation from hex string

## Usage

To use this project as a library, include the header in your project and
provide the object file to the linker. The header can be used in C and C++.

The project provides unit tests, a C example and a C++ example. Their binary
can be compiled by using GNU make with the targets `test`, `mainc`, `maincpp`
respectively. `make all` can be used to compile all binaries at once.

To run all tests, run `make test && ./test`.
