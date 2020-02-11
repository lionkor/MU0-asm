# modulo implementation
# a % b = result
JMP >init
d x = 0xA
d y = 0x6
d result = 0x0
d one = 0x1  # constant 1
d zero = 0x0 # constant 0

>init
    LDA x
    STO result
    # check for equality, a == b => a % b == 0
    LDA x
    SUB y
    JNE +4
    LDA zero
    STO result
    STP
    # not equal, so jump to the modulo operation
    JMP >modulo


    d f_a_a = 0x1 # a
    d f_a_b = 0x1 # b
    d f_a_r = 0x0 # result
>f_a
    LDA f_a_a
    JNE +2
    JMP >f_a_a_is_0
    LDA f_a_b
    JNE +2
    JMP >f_a_b_is_0
    # count down a by 1
    LDA f_a_a
    SUB one
    STO f_a_a
    JNE +2 # if not 0, ignore next jump
    JMP >f_a_a_is_0
    # count down b by 1
    LDA f_a_b
    SUB one
    STO f_a_b
    JNE +2 # if not 0, ignore next jump
    JMP >f_a_b_is_0
    # END, NO CONDITION HIT, LOOP TO start
    JMP >f_a
    # a reached 0
>f_a_a_is_0
    LDA one
    STO f_a_r
    JMP >f_a_ret
    # b reached 0
>f_a_b_is_0
    LDA zero
    STO f_a_r
>f_a_ret
    RET

>modulo
    LDA result
    SUB y
    STO result
    # call f_a (a < b)
    LDA y
    STO f_a_a # a = y
    LDA result
    STO f_a_b # b = result
    # call function f_a
    CALL >f_a
    LDA f_a_r
    JNE >modulo
    LDA result
    # result will be in ACC
    STP
