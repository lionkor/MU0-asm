# `MU0-asm`

Simple assembler for MU0

See [lionkor/MU0](https://github.com/lionkor/MU0) for the emulator


## How to build

1. Clone the repo, with `git clone [URL]` and enter the directory. There is a submodule, but it's no longer a dependency and will be completely removed in the future, so don't worry about it.

3. Go into the repo folder and run `cmake . && make`.

You now have an executable `mu0asm` that you can pass an asm file as an argument, like this:

`./mu0asm multiply.asm`

It will compile it into the file `a.out`. You can then run it with my MU0 emulator (not included) with `./MU0 a.out`.

## Syntax (standard MU0 instruction set)

You can write comments starting with `#`. Example:
```
# this is a comment
lda 0x5 # this is also a comment
```

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

`S` in this case might be any address in the space supported by MU0. `S` can be noted as hex `0xN` or decimal `N`.

We also support data, noted like this:
```
d variable_name=0x65
```
`variable_name` can then be used inplace of any `S`.
This is the only way to get values into the program. 
`d` declarations **must** appear before the place where they are needed. They should be at the top of the file, with a JMP as the only instruction before them to jump past the data (since MU0 just starts executing at mem offset 0). Example:

A simple adder, does a+b=result, in this case 5+10
```asm
jmp 0x4
d a = 0x5
d b = 0xA
d result = 0x0
lda $a
add $b
sto $result
stp
```
`JMP 0x4` jumps (absolute) to the 5th instruction (counting from 0, that's 4).
The `d` declarations declare some data locations that we will use.
Then we load a, add b, and store the result in `result`.
Then we STP (stop), which hangs the CPU. When using my MU0 emulator, it will continuosly display the value in ACC, and that will be the result. We stop so that the ACC doesn't change.

## Abstract instruction set extensions

The following extensions are provided by this assembler but don't actually translate *directly* to MU0 instructions. Instead, these are "faked" with a little bit of magic!

All of them are enabled and cannot be disabled, but can simply be ignored if one wishes to write "pure" MU0.

* Labels & label jumping:
    
    You can create labels with this syntax:
    ```asm
    .my_label:
    ```
    They follow usual rules of naming, like no spaces, etc.
    Labels can then be jumped to (this is the usual use-case):
    ```asm
    jmp .my_label
    ```

    You can jump to labels *anywhere* in the program.
    
    They must start with `.` and end with `:`.
