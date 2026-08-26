# Based on:
#   * godbolt riscv decompilation of an sbox lookup function
#   * https://riscv.epcc.ed.ac.uk/documentation/how-to/first_c_asm_vector_prog/

.global getSBoxValue
.global getSBoxInvert
.global getRcon

// Tabular data used by the library
//
// this .rodata section is linked into place just
// before the text section in the linker script.
.section .rodata
sboxdata:
        .balign 16
        .ascii "c|w{\362ko\3050\001g+\376\327\253v\312\202\311}\372YG\360\255"
        .ascii  "\324\242\257\234\244r\300\267\375\223&6?\367\3144\245\345\361q"
        .ascii  "\3301\025\004\307#\303\030\226\005\232\007\022\200\342\353'\262u"
        .ascii  "\t\203,\032\033nZ\240R;\326\263)\343/\204S\321\000"
        .ascii  "\355 \374\261[j\313\2769JLX\317\320\357\252\373CM3\205E\371\002"
        .ascii  "\177P<\237\250Q\243@\217\222\2358\365\274\266\332!\020\377\363"
        .ascii  "\322\315\f\023\354_\227D\027\304\247~=d]\031s`\201O\334\"*\220"
        .ascii  "\210F\356\270\024\336^\013\333\3402:\nI\006$\\\302\323\254b\221"
        .ascii  "\225\344y\347\3107m\215\325N\251lV\364\352ez\256\b\272x%.\034"
        .ascii  "\246\264\306\350\335t\037K\275\213\212p>\265fH\003\366\016a5"
        .ascii  "W\271\206\301\035\236\341\370\230\021i\331\216\224\233\036\207"
        .ascii  "\351\316U(\337\214\241\211\r\277\346BhA\231-\017\260T\273\026"
rsboxdata:
        .balign 16
        .ascii  "R\tj\32506\2458\277@\243\236\201\363\327\373|\3439\202\233/\377"
        .ascii  "\2074\216CD\304\336\351\313T{\2242\246\302#=\356L\225\013B\372"
        .ascii  "\303N\b.\241f(\331$\262v[\242Im\213\321%r\370\366d\206h\230\026"
        .ascii  "\324\244\\\314]e\266\222lpHP\375\355\271\332^\025FW\247\215\235"
        .ascii  "\204\220\330\253\000"
        .ascii  "\214\274\323\n\367\344X\005\270\263E\006\320,\036\217\312?\017"
        .ascii  "\002\301\257\275\003\001\023\212k:\221\021AOg\334\352\227\362"
        .ascii  "\317\316\360\264\346s\226\254t\"\347\2555\205\342\3717\350\034"
        .ascii  "u\337nG\361\032q\035)\305\211o\267b\016\252\030\276\033\374V"
        .ascii  ">K\306\322y \232\333\300\376x\315Z\364\037\335\2503\210\007\307"
        .ascii  "1\261\022\020Y'\200\354_`Q\177\251\031\265J\r-\345z\237\223\311"
        .ascii  "\234\357\240\340;M\256*\365\260\310\353\273<\203S\231a\027+\004"
        .ascii  "~\272w\326&\341i\024cU!\f}"
rcondata:
        .balign 16
        .ascii  "\215\001\002\004\b\020 @\200\0336"

// NOTE about the auipc instruction:
//
//  auipc a0, 0x10000
//
// The calculation performed is a0 = PC + (imm20 << 12)
// so if PC = 0x10000004 and 0x10000 for imm20.
// a0 = 0x2000004:
//
// >>> PC = 0x10000004
// >>> imm20 = 0x10000
// >>> hex(PC + (imm20 << 12))
// '0x20000004'

.section .text.sboxfunction
getSBoxValue:
        .balign 16
        auipc   ca1, 0
        # The start of the table is in the compartment
        # rodata section before this text section!
        #
        # Go back:
        # (rcon=16 + rsbox=256 + sbox=256) = 528 bytes
        caddi   ca1, ca1, -528
        cadd    ca1, ca1, a0   # Add to that the index we want
        lbu     a0, 0(ca1)     # return the byte at that index
        ret

.section .text.rsboxfunction
getSBoxInvert:
        .balign 16
        auipc   ca1, 0
        # The start of the table is in the compartment
        # rodata section before this text section!
        #
        # Go back:
        # (getSBoxValue=32 + rcon=16 + rsbox=256) = 304 bytes
        caddi   ca1, ca1, -304
        cadd    ca1, ca1, a0   # Add to that the index we want
        lbu     a0, 0(ca1)     # return the byte at that index
        ret

.section .text.rconfunction
getRcon:
        .balign 16
        auipc   ca1, 0
        # The start of the table is in the compartment
        # rodata section before this text section!
        #
        # Go back:
        # (getSBoxInverse=32 + getSBoxValue=32 + rcon=16) = 80 bytes
        caddi   ca1, ca1, -80
        cadd    ca1, ca1, a0   # Add to that the index we want
        lbu     a0, 0(ca1)     # return the byte at that index
        ret
