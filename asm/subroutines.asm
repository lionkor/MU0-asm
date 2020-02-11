JMP >start
d a = 0x1

>fn
    LDA a
    STO a
    RET

>my_subroutine
    LDA a
    CALL >fn
    JMP +1
    LDA a
    ADD a
    ADD a
    ADD a
    JMP +1
    STO a
    RET

>start
    CALL >my_subroutine
    LDA a
    STP

