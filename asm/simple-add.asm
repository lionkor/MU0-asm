jmp 4
d a = 16
d b = 0x20
d result = 0
lda $a
add $a
add $b
sto $result
lda $result
stp
