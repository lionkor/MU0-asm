JMP +5
d a = 0x3
d b = 0x3
d result = 0x0
d one = 0x1
LDA result
ADD a
STO result
LDA b
SUB one
STO b
JNE -6 # loop back
LDA result
STP
