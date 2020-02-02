# modulo implementation
# a % b = result
JMP +6
d x = 0xB
d y = 0x6
d result = 0x0
d one = 0x1 # constant 1
d zero = 0x0 # constant 0

# init: result = x
LDA x
STO result
JMP +23

# f_a: a <= b
JMP +3
d f_a_a = 0x1 # a
d f_a_b = 0x1 # b
d f_a_r = 0x0 # result
# BEGIN
# count down a by 1
LDA f_a_a
SUB one
STO f_a_a
JNE +2 # if not 0, ignore next jump
JMP +7 # jump to "a reached 0"
# count down b by 1
LDA f_a_b
SUB one
STO f_a_b
JNE +2 # if not 0, ignore next jump
JMP +5 # jump to "b reached 0"
# END, NO CONDITION HIT, LOOP TO "BEGIN"
JMP -10
# a reached 0
LDA one
STO f_a_r
JMP +4
# b reached 0
LDA zero
STO f_a_r
JMP +1
# DONE, RETURN
JMP +9

# begin
# TODO check condition first as well
LDA result
SUB y
STO result
# call f_a (a < b)
LDA y
STO f_a_a # a = y
LDA result
STO f_a_b # b = result
# call function
JMP -25
LDA f_a_r
JNE -9
LDA result
STP
