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

## Syntax

Supported instructions:
```
LDA S   Load mem[S] into ACC
STO S   Store ACC in mem[S]
ADD S   Add mem[S] to ACC
SUB S   Subtract mem[S] from ACC
JMP S   Jump to mem[S] (pc := mem[S])
JGE S   Jump to mem[S] (pc := mem[S]) if ACC >= 0
JNE S   Jump to mem[S] (pc := mem[S]) if ACC != 0
STP     Stop
```

`S` in this case might be any **HEX** address in the space supported by MU0. In this case, `S` *must* be noted in hex format `0xN`.

We also support data, noted like this:
```
d variable_name=0x65
```
`variable_name` can then be used inplace of any `S`.
This is the only way to get values into the program. 
`d` declarations **must** appear before the place where they are needed. They should be at the top of the file, with a JMP as the only instruction before them to jump past the data (since MU0 just starts executing at mem offset 0). Example:

A simple adder, does a+b=result, in this case 5+10
```
JMP 0x4
d a = 0x5
d b = 0xA
d result = 0x0
LDA a
ADD b
STO result
STP
```
`JMP 0x4` jumps (absolute) to the 5th instruction (counting from 0, that's 4).
The `d` declarations declare some data locations that we will use.
Then we load a, add b, and store the result in `result`.
Then we STP (stop), which hangs the CPU. When using my MU0 emulator, it will continuosly display the value in ACC, and that will be the result. We stop so that the ACC doesn't change.
