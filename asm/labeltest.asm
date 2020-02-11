JMP >start
d a = 0x3
d one = 0x1

>start
LDA a
ADD one
STO a
JMP >start

