#starts at 0x10000000

#starts at 0x10000010
#count_of_floating_pairs - tells the number of floating point pairs to be operated upon.
.data
.dword 11, 52
.dword 2
.dword 0x409fa0cf5c28f5c3, 0x4193aa8fc4f0a3d7
.dword 0x40e699d2003eea21, 0x420e674bcb5a1877

.text
#The following line initializes register x3 with 0x10000000 
#so that you can use x3 for referencing various memory locations. 
lui x3, 0x10000

addi x4, x3, 0
addi x4, x4, 0x200

addi x5, x3, 24
#your code starts here
ld x6, 16(x3)
while: beq x6, x0, exit
    addi x6, x6, -1
    ld x7, 0(x5)
    ld x8, 8(x5)
    addi x5, x5, 16
    
    addi x10, x0, 1
    slli x10, x10, 63
    
    and x21, x7, x10
    slli x22, x7, 1
    srli x22, x22, 53
    slli x22, x22, 52
    slli x23, x7, 12
    srli x23, x23, 12
    
    and x24, x8, x10
    slli x25, x8, 1
    srli x25, x25, 53
    slli x25, x25, 52
    slli x26, x8, 12
    srli x26, x26, 12
    
    addi sp, sp, -16
    sd x23, 0(sp)
    sd x26, 8(sp)
    
    #for checking infinities and NANs
    addi x30, x0, -1
    srli x30, x30, 53
    slli x30, x30, 52
    
    jal x1, fp64add
    jal x1, fp64mul
    beq x0, x0, while
    
fp64add:
    addi x27, x0, 0
    c1_add: bne x22, x30, c2_add
    addi x27, x0, 1
    beq x23, x0, c2_add
    nan1_add: addi x17, x21, 0
    add x17, x17, x22
    add x17, x17, x23
    beq x0, x0, store_add
    
    c2_add: beq x25, x30, c21_add
    beq x0, x0, if_1st_is_inf_add
    c21_add: beq x26, x0, inf_add
    
    nan2_add: addi x17, x24, 0
    add x17, x17, x25
    add x17, x17, x26
    beq x0, x0, store_add
    
    if_1st_is_inf_add: beq x27, x0, normal_add
    addi x17, x21, 0
    add x17, x17, x30
    beq x0, x0, store_add
    
    inf_add: addi x10, x0, 1
    beq x27, x10, both_inf_add
    if_2nd_is_inf_add: addi x17, x24, 0
    add x17, x17, x25
    add x17, x17, x26
    beq x0, x0, store_add
    
    both_inf_add: xor x10, x21, x24
    bne x10, x0, opp_signs_add
    addi x17, x21, 0
    add x17, x17, x30
    beq x0, x0, store_add
    opp_signs_add: addi x17, x0, 0
    beq x0, x0, store_add
    
    normal_add:
    addi x10, x0, 1
    slli x10, x10, 52
    add x23, x23, x10
    add x26, x26, x10
    #difference bw exponents and right shift
        sub x10, x22, x25
        srai x10, x10, 52
        
        bge x10, x0, else
        #if x25>x22
        addi x15, x24, 0
        addi x16, x25, 0
        addi x17, x26, 0
        addi x18, x23, 0
        beq x0, x0, out2
        else: beq x10, x0, same_exp
        addi x15, x21, 0
        addi x16, x22, 0
        addi x17, x23, 0
        addi x18, x26, 0
        out2:
            
        sub x10, x0, x10
        addi x8, x10, 0
        #shift smaller number and round
        srl x9, x18, x10
        andi x12, x9, 1
        sll x9, x9, x10
        addi x10, x10, -1
        addi x11, x0, 1
        sll x11, x11, x10
        sub x10, x18, x9
        
        srl x18, x18, x8
        
        beq x12, x0, even_case1
        odd_case1:bge x11, x10, round_down1
        addi x18, x18, 1
        beq x0, x0, round_down1
        even_case1: blt x10, x11, round_down1
        addi x18, x18, 1
        round_down1:
        
        #x15 is larger numbers f
        #check for sign bits
        xor x10, x21, x24
        #if signs equal add
        beq x10, x0, addf1
        subf1: sub x17, x17, x18
        beq x0, x0, outf1
        addf1: add x17, x17, x18
        outf1: beq x0, x0, return_add
        
        same_exp: 
        c1: bge x23, x26, c2
            #x26 is greater
            addi x15, x24, 0
            addi x16, x25, 0
            addi x17, x26, 0
            addi x18, x23, 0
        c2: addi x15, x21, 0
            addi x16, x22, 0
            addi x17, x23, 0
            addi x18, x26, 0
        
        #check sign
        xor x10, x21, x24
        beq x10, x0, addf2
        subf2: sub x17, x17, x18
        beq x0, x0, return_add
        addf2: add x17, x17, x18
            
        return_add:
            addi x11, x0, 1
            slli x11, x11, 52
            
            addi x10, x0, 1
            slli x10, x10, 53
            
            #to normalize and round in case of extra or fewer bits
            and x10, x10, x17
            bne x10, x0, shift_right
            and x10, x17, x11
            beq x10, x0, shift_left
            beq x0, x0, out
            shift_right: andi x20, x17, 1
            srli x17, x17, 1
            beq x20, x0, only_shift
            andi x20, x17, 1
            beq x20, x0, only_shift
            round_up: addi x17, x17, 1
            only_shift: 
            add x16, x16, x11
            bne x16, x30, out
            #if exponent overflow, inf
            add x17, x30, x15
            beq x0, x0, store_add
            shift_left: slli x17, x17, 1
            sub x16, x16, x11
            out: sub x17, x17, x11
            add x17, x17, x15
            add x17, x17, x16
            
            store_add: #store in memory
            sd x17, 0(x4)
            addi x4, x4, 8
            
            #reverting to original values
            ld x23, 0(sp)
            ld x26, 8(sp)
            addi sp, sp, 16
            
            jalr x0, x1, 0
        
fp64mul:
    c1_mul: bne x22, x30, c2_mul
    beq x23, x0, c2_mul
    nan1_mul: addi x15, x21, 0
    add x15, x15, x22
    add x15, x15, x23
    addi x15, x15, 1
    beq x0, x0, store_mul
    
    c2_mul: beq x25, x30, c21_mul
    beq x0, x0, if_1st_is_inf_mul
    c21_mul: beq x26, x0, if_2nd_is_inf_mul
    
    nan2_mul: addi x15, x24, 0
    add x15, x15, x25
    add x15, x15, x26
    addi x15, x15, 1
    beq x0, x0, store_mul
    
    if_1st_is_inf_mul: beq x27, x0, normal_mul
    #if other value is zero
    bne x24, x0, inf
    bne x25, x0, inf
    nan1: addi x17, x30, 1
    beq x0, x0, store_mul
    
    if_2nd_is_inf_mul:
    #if other value is zero
    bne x24, x0, inf
    bne x25, x0, inf
    nan2: addi x17, x30, 1
    beq x0, x0, store_mul
    
    inf: xor x10, x21, x24
    beq x10, x0, pos
    or x10, x21, x24
    addi x15, x10, 0
    add x15, x15, x30
    beq x0, x0, store_mul
    pos: addi x15, x30, 0
    beq x0, x0, store_mul
    
    normal_mul:
        #check if zero
        bne x22, x0, if_2nd_zero
        addi x15, x0, 0
        beq x0, x0, store_mul
        if_2nd_zero: bne x25, x0, not_zero
        addi x15, x0, 0
        beq x0, x0, store_mul
        
        not_zero: addi x10, x0, 1
        slli x10, x10, 52
        add x23, x23, x10
        add x26, x26, x10
    
        xor x15, x21, x24
        
        addi x14, x0, 1023
        slli x14, x14, 52
        sub x16, x25, x14
        add x16, x16, x22
        
        bne x16, x30, under
        #if exponent overflow, inf
        add x15, x30, x15
        beq x0, x0, store_mul
        
        #if exponent underflows
        under: bge x16, x0, out_mul
        add x15, x30, x15
        beq x0, x0, store_mul
        
        out_mul: add x11, x0, x23
        add x12, x0, x26
        addi sp, sp, -8
        sd x1, 0(sp)
        jal x1, dmul
        ld x1, 0(sp)
        addi sp, sp, 8
        add x15, x15, x14
        add x15, x15, x16
        
        store_mul:
        sd x15, 0(x4)
        addi x4, x4, 8
        jalr x0, x1, 0
        
dmul:
     addi x9, x0, 1
     slli x10, x9, 53
     addi x14, x0, 0
     addi x8, x0, 0
     start: beq x9, x10, done
     and x20, x11, x9
     beq x20, x0, next
     add x14, x14, x12
     next: slli x9, x9, 1
     beq x9, x10, done
     andi x13, x14, 1
     or x8, x8, x13
     srli x14, x14, 1
     beq x0, x0, start
     done: #normalize and round
        andi x10, x14, 1
        beq x13, x0, round_down
        beq x10, x0, even_case
        odd_case: addi x14, x14, 1
        beq x0, x0, round_down
        even_case: beq x8, x0, round_down
        addi x14, x14, 1
        beq x10, x0, round_down
        round_down:addi x10, x0, 1
        slli x10, x10, 53
        and x13, x10, x14
        beq x13, x0, return
        andi x13, x14, 1
        srli x14, x14, 1
        andi x10, x14, 1
        beq x13, x0, no_shift
        beq x10, x0, no_shift
        addi x14, x14, 1
        no_shift:
        addi x10, x0, 1
        slli x10, x10, 52
        add x16, x16, x10
        bne x16, x30, return
        #if exponent overflow, inf
        add x15, x30, x15
        beq x0, x0, store_add
        return: 
        addi x10, x0, 1
        slli x10, x10, 52
        sub x14, x14, x10
        jalr x0, x1, 0 
exit: 
    addi x0, x0, 0   


#The final result should be in memory starting from address 0x10000200
#The first dword location at 0x10000200 contains sum of input11, input12
#The second dword location at 0x10000200 contains product of input11, input12
#The third dword location from 0x10000200 contains sum of input21, input22,
#The fourth dword location from 0x10000200 contains product of input21, input22, and so on.

