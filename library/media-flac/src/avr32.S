.macro LOOP_SETUP_SHORT
    mov     r4, 0           # sum = 0
    mov     r5, 0           # sum = 0
    ld.w    r2, r8[0]
    ld.w    r3, r9[-4]
    mulwh.d r4, r3, r2:t    # r5:r4 = r3 * (0x0000ffff & r2)
.endm

.macro LOOP_SETUP
    LOOP_SETUP_SHORT
    ld.w    r3, r9[-8]
    macwh.d r4, r3, r2:b
.endm

.macro ADJUST_LOOP_AND_RETURN_LEFT loop:req
    lsl     r5, 16
    or      r5, r5, r4 >> 16    # r4 now contains the 32 bit value

    lsl     r5, r5, r11         # --- LEFT ---

    ld.w    r3, r9[0]
    add     r3, r5
    st.w    r9++, r3
    st.w    r7++, r3            # Store to the slow array

    # End of the loop work
    add     r0, r1          # i++
    cp.w    r0, r12         # r0 < r12 (i < blocksize)
    brlt    \loop           # Yes - then loop
    popm    r0-r3,r4-r7,r8-r9,r10,r11,r12,pc
.endm

.macro ADJUST_LOOP_AND_RETURN_RIGHT loop:req
    lsl     r5, 16
    or      r5, r5, r4 >> 16    # r4 now contains the 32 bit value

    asr     r5, r5, r11         # --- RIGHT ---

    ld.w    r3, r9[0]
    add     r3, r5
    st.w    r9++, r3            # Store to the decode array
    st.w    r7++, r3            # Store to the slow array

    # End of the loop work
    add     r0, r1          # i++
    cp.w    r0, r12         # r0 < r12 (i < blocksize)
    brlt    \loop           # Yes - then loop
    popm    r0-r3,r4-r7,r8-r9,r10,r11,r12,pc
.endm

.macro MULTIPLY_AND_ADD_HALF coeff_index:req data_index:req
    ld.w    r2, r8[\coeff_index]
    ld.w    r3, r9[\data_index]
    macwh.d r4, r3, r2:t
.endm

.macro MULTIPLY_AND_ADD_FULL coeff_index:req data_index_1:req data_index_2:req
    ld.w    r2, r8[\coeff_index]
    ld.w    r3, r9[\data_index_1]
    macwh.d r4, r3, r2:t
    ld.w    r3, r9[\data_index_2]
    macwh.d r4, r3, r2:b
.endm

        .file       "avr32.S"
        .section    .text.lpc_decode_avr32,"ax",@progbits
        .align      1
        .globl      lpc_decode_avr32
        .type       lpc_decode_avr32, @function
    lpc_decode_avr32:

        # sp[56] -- output location
        # sp[52] - r0  - i
        # sp[48] - r1  - 1
        # sp[44] - r2  - *coeff value
        # sp[40] - r3  - *data value
        # sp[28] - r4  - jump offset / sum
        # sp[24] - r5  - temp jump offset / sum overflow
        # sp[24] - r6  - 
        # sp[16] - r7  - int32_t *slow
        # sp[20] - r8  - int32_t *coeffs
        # sp[16] - r9  - int32_t *data
        # sp[12] - r10 - int32_t pred_order  [1 - 32]
        # sp[8]  - r11 - int32_t qlevel      [-16 - 15]
        # sp[4]  - r12 - int32_t blocksize   (greater than 0)
        # sp[0]  - lr  - Return address
        pushm   r0-r3,r4-r7,r8-r9,r10,r11,r12,lr

        mov     r1, 1
        mov     r0, 0       # i = 0
        ld.w    r7, sp[56]  # load the output address

        # determine if we are shifting left or right and the order
        mov     r4, r10
        subal   r4, 1
        lsl     r4, 1
        cp.w    r11, 0
        brlt    shift_left  # if( qlevel < 0) then shift left
        orl     r4, 1       # shift to the right
    shift_left:
        abs     r11
        lda.w   r5, jump_table_start
        add     r5, r5, r4 << 2
        mov     pc, r5

    jump_table_start:
        bral    pred_order_1_left
        bral    pred_order_1_right
        bral    pred_order_2_left
        bral    pred_order_2_right
        bral    pred_order_3_left
        bral    pred_order_3_right
        bral    pred_order_4_left
        bral    pred_order_4_right
        bral    pred_order_5_left
        bral    pred_order_5_right
        bral    pred_order_6_left
        bral    pred_order_6_right
        bral    pred_order_7_left
        bral    pred_order_7_right
        bral    pred_order_8_left
        bral    pred_order_8_right
        bral    pred_order_9_left
        bral    pred_order_9_right
        bral    pred_order_10_left
        bral    pred_order_10_right
        bral    pred_order_11_left
        bral    pred_order_11_right
        bral    pred_order_12_left
        bral    pred_order_12_right
        bral    pred_order_13_left
        bral    pred_order_13_right
        bral    pred_order_14_left
        bral    pred_order_14_right
        bral    pred_order_15_left
        bral    pred_order_15_right
        bral    pred_order_16_left
        bral    pred_order_16_right
        bral    pred_order_17_left
        bral    pred_order_17_right
        bral    pred_order_18_left
        bral    pred_order_18_right
        bral    pred_order_19_left
        bral    pred_order_19_right
        bral    pred_order_20_left
        bral    pred_order_20_right
        bral    pred_order_21_left
        bral    pred_order_21_right
        bral    pred_order_22_left
        bral    pred_order_22_right
        bral    pred_order_23_left
        bral    pred_order_23_right
        bral    pred_order_24_left
        bral    pred_order_24_right
        bral    pred_order_25_left
        bral    pred_order_25_right
        bral    pred_order_26_left
        bral    pred_order_26_right
        bral    pred_order_27_left
        bral    pred_order_27_right
        bral    pred_order_28_left
        bral    pred_order_28_right
        bral    pred_order_29_left
        bral    pred_order_29_right
        bral    pred_order_30_left
        bral    pred_order_30_right
        bral    pred_order_31_left
        bral    pred_order_31_right
        bral    pred_order_32_left
        bral    pred_order_32_right

    ## 1 ###########################################################################
    pred_order_1_left:
        LOOP_SETUP_SHORT
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_1_left

    pred_order_1_right:
        LOOP_SETUP_SHORT
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_1_right

    ## 2 ###########################################################################
    pred_order_2_left:
        LOOP_SETUP
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_2_left

    pred_order_2_right:
        LOOP_SETUP
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_2_right

    ## 3 ###########################################################################
    pred_order_3_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_HALF 4, -12
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_3_left

    pred_order_3_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_HALF 4, -12
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_3_right

    ## 4 ###########################################################################
    pred_order_4_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_4_left

    pred_order_4_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_4_right

    ## 5 ###########################################################################
    pred_order_5_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_HALF 8, -20
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_5_left

    pred_order_5_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_HALF 8, -20
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_5_right

    ## 6 ###########################################################################
    pred_order_6_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_6_left

    pred_order_6_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_6_right

    ## 7 ###########################################################################
    pred_order_7_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_HALF 12, -28
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_7_left

    pred_order_7_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_HALF 12, -28
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_7_right

    ## 8 ###########################################################################
    pred_order_8_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_8_left

    pred_order_8_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_8_right

    ## 9 ###########################################################################
    pred_order_9_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_HALF 16, -36
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_9_left

    pred_order_9_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_HALF 16, -36
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_9_right

    ## 10 ##########################################################################
    pred_order_10_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_10_left

    pred_order_10_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_10_right

    ## 11 ##########################################################################
    pred_order_11_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_HALF 20, -44
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_11_left

    pred_order_11_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_HALF 20, -44
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_11_right

    ## 12 ##########################################################################
    pred_order_12_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_12_left

    pred_order_12_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_12_right

    ## 13 ##########################################################################
    pred_order_13_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_HALF 24, -52
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_13_left

    pred_order_13_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_HALF 24, -52
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_13_right

    ## 14 ##########################################################################
    pred_order_14_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_14_left

    pred_order_14_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_14_right

    ## 15 ##########################################################################
    pred_order_15_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_HALF 28, -60
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_15_left

    pred_order_15_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_HALF 28, -60
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_15_right

    ## 16 ##########################################################################
    pred_order_16_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_16_left

    pred_order_16_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_16_right

    ## 17 ##########################################################################
    pred_order_17_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_HALF 32, -68
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_17_left

    pred_order_17_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_HALF 32, -68
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_17_right

    ## 18 ##########################################################################
    pred_order_18_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_18_left

    pred_order_18_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_18_right

    ## 19 ##########################################################################
    pred_order_19_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_HALF 36, -76
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_19_left

    pred_order_19_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_HALF 36, -76
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_19_right

    ## 20 ##########################################################################
    pred_order_20_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_20_left

    pred_order_20_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_20_right

    ## 21 ##########################################################################
    pred_order_21_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_HALF 40, -84
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_21_left

    pred_order_21_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_HALF 40, -84
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_21_right

    ## 22 ##########################################################################
    pred_order_22_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_22_left

    pred_order_22_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_22_right

    ## 23 ##########################################################################
    pred_order_23_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_HALF 44, -92
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_23_left

    pred_order_23_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_HALF 44, -92
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_23_right

    ## 24 ##########################################################################
    pred_order_24_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_24_left

    pred_order_24_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_24_right

    ## 25 ##########################################################################
    pred_order_25_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_HALF 48, -100
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_25_left

    pred_order_25_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_HALF 48, -100
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_25_right

    ## 26 ##########################################################################
    pred_order_26_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_26_left

    pred_order_26_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_26_right

    ## 27 ##########################################################################
    pred_order_27_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_HALF 52, -108
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_27_left

    pred_order_27_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_HALF 52, -108
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_27_right

    ## 28 ##########################################################################
    pred_order_28_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_28_left

    pred_order_28_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_28_right

    ## 29 ##########################################################################
    pred_order_29_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_HALF 56, -116
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_29_left

    pred_order_29_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_HALF 56, -116
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_29_right

    ## 30 ##########################################################################
    pred_order_30_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_FULL 56, -116, -120
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_30_left

    pred_order_30_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_FULL 56, -116, -120
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_30_right

    ## 31 ##########################################################################
    pred_order_31_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_FULL 56, -116, -120
        MULTIPLY_AND_ADD_HALF 60, -124
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_31_left

    pred_order_31_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_FULL 56, -116, -120
        MULTIPLY_AND_ADD_HALF 60, -124
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_31_right

    ## 32 ##########################################################################
    pred_order_32_left:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_FULL 56, -116, -120
        MULTIPLY_AND_ADD_FULL 60, -124, -128
        ADJUST_LOOP_AND_RETURN_LEFT pred_order_32_left

    pred_order_32_right:
        LOOP_SETUP
        MULTIPLY_AND_ADD_FULL 4, -12, -16
        MULTIPLY_AND_ADD_FULL 8, -20, -24
        MULTIPLY_AND_ADD_FULL 12, -28, -32
        MULTIPLY_AND_ADD_FULL 16, -36, -40
        MULTIPLY_AND_ADD_FULL 20, -44, -48
        MULTIPLY_AND_ADD_FULL 24, -52, -56
        MULTIPLY_AND_ADD_FULL 28, -60, -64
        MULTIPLY_AND_ADD_FULL 32, -68, -72
        MULTIPLY_AND_ADD_FULL 36, -76, -80
        MULTIPLY_AND_ADD_FULL 40, -84, -88
        MULTIPLY_AND_ADD_FULL 44, -92, -96
        MULTIPLY_AND_ADD_FULL 48, -100, -104
        MULTIPLY_AND_ADD_FULL 52, -108, -112
        MULTIPLY_AND_ADD_FULL 56, -116, -120
        MULTIPLY_AND_ADD_FULL 60, -124, -128
        ADJUST_LOOP_AND_RETURN_RIGHT pred_order_32_right

        .size lpc_decode_avr32, .-lpc_decode_avr32
