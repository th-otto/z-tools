 .globl slb_header
 .text
 .globl _start
_start:
slb_header:
 .dc.l 0x70004afc
 .dc.l _slbname
 .dc.l 0x0200
 .dc.l 0
 .dc.l slb_init
 .dc.l slb_exit
 .dc.l slb_open
 .dc.l slb_close
 .dc.l _slh_names
 .dc.l 0
 .dc.l 0,0,0,0,0,0,0
 .dc.l (slh_fend-slh_functions)/4
slh_functions:
          .dc.l _wrap_slb_control
          .dc.l _wrap_reader_init
          .dc.l _wrap_reader_read
          .dc.l _wrap_reader_get_txt
          .dc.l _wrap_reader_quit
          .dc.l 0
          .dc.l 0
          .dc.l 0
          .dc.l _wrap_get_option
          .dc.l 0
slh_fend:
          name_slb_control_str: .ascii "slb_control"
  .dc.b 0
          name_reader_init_str: .ascii "reader_init"
  .dc.b 0
          name_reader_read_str: .ascii "reader_read"
  .dc.b 0
          name_reader_get_txt_str: .ascii "reader_get_txt"
  .dc.b 0
          name_reader_quit_str: .ascii "reader_quit"
  .dc.b 0
         
         
         
          name_get_option_str: .ascii "get_option"
  .dc.b 0
         
 .even
_slh_names:
          .dc.l name_slb_control_str
          .dc.l name_reader_init_str
          .dc.l name_reader_read_str
          .dc.l name_reader_get_txt_str
          .dc.l name_reader_quit_str
          .dc.l 0
          .dc.l 0
          .dc.l 0
          .dc.l name_get_option_str
          .dc.l 0
          _wrap_slb_control: movem.l 16(a7),d0-d1
  movem.l d0-d1,4(a7)
  bra slb_control
          _wrap_reader_init: movem.l 16(a7),d0-d1
  movem.l d0-d1,4(a7)
  bra reader_init
          _wrap_reader_read: movem.l 16(a7),d0-d1
  movem.l d0-d1,4(a7)
  bra reader_read
          _wrap_reader_get_txt: movem.l 16(a7),d0-d1
  movem.l d0-d1,4(a7)
  bra reader_get_txt
          _wrap_reader_quit: move.l 16(a7),d0
  move.l d0,4(a7)
  bra reader_quit
         
         
         
          _wrap_get_option: move.l 16(a7),d0
  move.l d0,4(a7)
  bra get_option
         
_slbname: .ascii "zvcanvas.slb"
 .dc.b 0
 .even
