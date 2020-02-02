JMP 0x4
d a = 0xA
d b = 0x5
d result = 0x0
LDA a
SUB b
STO result
LDA result
STP
