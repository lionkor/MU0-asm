# `MU0-asm`

Simple assembler for MU0

**currently undergoing a complete rewrite, on the `rewrite` branch**

See [lionkor/MU0](https://github.com/lionkor/MU0) for the emulator


## How to build

1. Clone the repo recursively, with `git clone [URL] --recursive` and enter the directory.

2. Go into the String folder and run `cmake . && make`.

3. Go back into the repo folder and run `cmake . && make`.

You now have an executable `mu0asm` that you can pass an asm file as an argument, like this:

`./mu0asm multiply.asm`

It will compile it into the file `a.out`. You can then run it with my MU0 emulator (not included) with `./MU0 a.out`.

## Syntax (standard MU0 instruction set)

You can write comments starting with `#`. Example:
```
# this is a comment
LDA 0x5 # this is also a comment
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

`S` in this case might be any **HEX** address in the space supported by MU0. In this case, `S` *must* be noted in hex format `0xN`.

We also support data, noted like this:
```
d variable_name=0x65
```
`variable_name` can then be used inplace of any `S`.
This is the only way to get values into the program. 
`d` declarations **must** appear before the place where they are needed. They should be at the top of the file, with a JMP as the only instruction before them to jump past the data (since MU0 just starts executing at mem offset 0). Example:

A simple adder, does a+b=result, in this case 5+10
```asm
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

## Abstract instruction set extensions

The following extensions are provided by this assembler but don't actually translate *directly* to MU0 instructions. Instead, these are "faked" with a little bit of magic!

All of them are enabled and cannot be disabled, but can simply be ignored if one wishes to write "pure" MU0.

* Relative jumping:
    All jump instructions usually require the jump location to be encoded in hex, such as `0x56`. Instead, you can also perform a relative jump by providing the argument as `+N` or `-N` instead. For example, you can jump "back" two instructions with `JMP -2`. 

    Note that you *cannot* use hex values here, although this will probably change later. This extension works for all jump instructions.

    Behind the scenes this is realized by simply calculating the absolute address and replacing the argument with it.

* Labels & label jumping:
    You can create labels with this syntax:
    ```asm
    >my_label
    ```
    They follow usual rules of naming, like no spaces, etc.
    Labels can then be jumped to (this is the usual use-case):
    ```asm
    JMP >my_label
    ```

    You can jump to labels *anywhere* in the program.

* Subroutines
    You can call labels (see above) as subroutines with the following syntax:
    ```asm
    CALL >my_label
    ```
    There is a *hard requirement* for the label to be followed *at some point* by a `RET` statement.
    
    The `CALL` will jump to the label, and then the RET will jump back to the point of calling, to the next instruction.
    Example:
    ```asm
    JMP >start
    # some data

    >start
        # some code
        CALL >my_function
        # more code

    >my_function
        # some code
        RET
    ```
    
    **Keep in mind** that currently there can only be **ONE** `RET` statement per subroutine. This is currently since the assembler doesn't follow the flow of the program, and will just quit looking after the first `RET`, leaving any potential other `RET` statements in the same subroutine completely unhandled, which is undefined behaviour. This *also applies* if both `RET` statements are mutually exclusive in the control flow. This is currently **not** supported.

    Internally, subroutines are implemented by replacing the `CALL` with a jump to the label, then going to said label and looking for the next `RET` and replacing it with a jump back to the place where it was `CALL`-ed from. 
