# modulo implementation
# a % b = result
JMP +6
d x = 0x3
d y = 0x2
d result = 0x0
d one = 0x1 # constant 1
d zero = 0x0 # constant 0

# init: result = x
LDA x
STO result
# check for equality, a == b => a % b == 0
LDA x
SUB y
JNE +4
LDA result 
STO zero
STP
# not equal, so jump to the modulo operation
JMP +23

# f_a: a <= b
JMP +3
d f_a_a = 0x1 # a
d f_a_b = 0x1 # b
d f_a_r = 0x0 # result
# BEGIN
LDA f_a_a
JNE +2
JMP +15
LDA f_a_b
JNE +2
JMP +15
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
JMP -16
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
JMP -31
LDA f_a_r
JNE -9
LDA result
# result will be in ACC
STP
