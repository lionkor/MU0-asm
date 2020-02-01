# `MU0-asm`

Simple assembler for MU0

See [lionkor/MU0](https://github.com/lionkor/MU0) for the emulator


## How to build

1. Clone the repo recursively, with `git clone [URL] --recursive` and enter the directory.

2. Go into the String folder and run `cmake . && make`.

3. Go back into the repo folder and run `cmake . && make`.

You now have an executable `mu0asm` that you can pass an asm file as an argument, like this:

`./mu0asm multiply.asm`

It will compile it into the file `a.out`. You can then run it with my MU0 emulator (not included) with `./MU0 a.out`.


