/*
 * 	National Semiconductor Corporation CR16 processor module for IDA Pro.
 *	Copyright (c) 2002-2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#include "cr16.hpp"
#include <diskio.hpp>
#include <entry.hpp>
#include <srarea.hpp>

//--------------------------------------------------------------------------
// ᯨ᮪ ॣ���஢
static const char *RegNames[] =
{
        // �㫥���
        "",
        // ����� ॣ�����
        "R0","R1","R2","R3","R4","R5","R6","R7",
        "R8","R9","R10","R11","R12","R13","RA","SP",
        // ����ॣ�����
        "PC","ISP","INTBASE","PSR","CFG","DSR","DCR","CARL","CARH",
        "INTBASEH","INTBASEL",

        // �ᥢ��-ᥣ����
        "cs","ds"
};

#if IDP_INTERFACE_VERSION > 37
static netnode helper;
char device[MAXSTR] = "";
static size_t numports;
static ioport_t *ports;

#include "../iocommon.cpp"

//----------------------------------------------------------------------
static int idaapi notify(processor_t::idp_notify msgid, ...)
{
  va_list va;
  va_start(va, msgid);
// A well behaving processor module should call invoke_callbacks()
// in his notify() function. If this function returns 0, then
// the processor module should process the notification itself
// Otherwise the code should be returned to the caller:

  int code = invoke_callbacks(HT_IDP, msgid, va);
  if ( code ) return code;
  switch(msgid){
    case processor_t::init:
      inf.mf = 0;
      inf.s_genflags |= INFFL_LZERO;
      helper.create("$ CR16");
    default:
      break;

    case processor_t::term:
      free_ioports(ports, numports);
      break;

    case processor_t::newfile:
      //�뢮��� ���. ���� �����஢, � �������� ����� �㦭�, ���뢠�� ��� ��࠭���
      //������ ���ଠ�� �� cfg. �� ��⠭�� ���ଠ樨 ������뢠�� ����� � ॣ����
      {
        char cfgfile[QMAXFILE];
        get_cfg_filename(cfgfile, sizeof(cfgfile));
		if ( choose_ioport_device(cfgfile, device, sizeof(device), parse_area_line0) )
          set_device_name(device, IORESP_ALL);
      }
      break;

    case processor_t::newprc:{
          char buf[MAXSTR];
          if ( helper.supval(-1, buf, sizeof(buf)) > 0 )
            set_device_name(buf, IORESP_PORT);
        }
        break;

    case processor_t::newseg:{
		segment_t *s = va_arg(va, segment_t *);
		// Set default value of DS register for all segments
		set_default_dataseg(s->sel);
		}
		break;
  }
  va_end(va);
  return(1);
}
#else

//----------------------------------------------------------------------
// �㭪�� �����饭��
static int notify(int msgnum,void *arg,...)
{ // Various messages:
  qnotused(arg);
  switch ( msgnum ) {
  // ���� 䠩�
  case IDP_NEWFILE:
      inf.mf = 0;                                       // MSB last
      inf.nametype = NM_SHORT;
      segment_t *sptr = get_first_seg();
      if ( sptr != NULL ) {
	if ( sptr->startEA-get_segm_base(sptr) == 0 ) {
	  inf.beginEA = sptr->startEA;
	  inf.startIP = 0;
	}
      }
      // �᭮���� ᥣ���� - ������
      set_segm_class(get_first_seg(),"CODE");
      break;
    // ᮧ����� ������ ᥣ����
    case IDP_NEWSEG:    {
                        segment_t *seg;
                        seg=((segment_t *)arg);
                        // ��⠭���� ॣ����� �� 㬮�砭��
                        seg->defsr[rVds-ph.regFirstSreg] = 0;
                        break;
                        }
  }
  return 1;
}
#endif
//-----------------------------------------------------------------------
//	Checkarg data. Common for all assemblers. Not good.
//-----------------------------------------------------------------------
static const char *operdim[15] = {  // ������ � ������ 15
     "(", ")", "!", "-", "+", "%",
     "\\", "/", "*", "&", "|", "^", "<<", ">>", NULL};
//-----------------------------------------------------------------------
//	PseudoSam
//-----------------------------------------------------------------------
static asm_t pseudosam = {
  AS_COLON | AS_UDATA | ASH_HEXF3 | ASD_DECF0,
  // ���짮��⥫�᪨� 䫠���
  0,
  "Generic CR16 assembler",             // �������� ��ᥬ����
  0,                                    // ����� � help'e
  NULL,                                 // ��⮧��������
  NULL,                                 // ���ᨢ �� �ᯮ������� ������権
  "org",                                // ��४⨢� ORG
  "end",                                // ��४⨢� end

  ";",                                  // ������਩
  '"',                                  // ࠧ����⥫� ��ப�
  '\'',                                 // ᨬ���쭠� ����⠭�
  "\\\"'",                              // ᯥ�ᨬ����

  "db",                                 // ascii string directive
  ".byte",                              // byte directive
  ".word",                              // word directive
  NULL,                                 // dword  (4 bytes)
  NULL,                                 // qword  (8 bytes)
#if IDP_INTERFACE_VERSION > 37
  NULL,									// oword  (16 bytes)
#endif
  NULL,                                 // float  (4 bytes)
  NULL,                                 // double (8 bytes)
  NULL,                                 // tbyte  (10/12 bytes)
  NULL,                                 // packed decimal real
  "#d dup(#v)",                         // arrays (#h,#d,#v,#s(...)
  "db ?",                               // uninited arrays
  ".equ",                               // equ
  NULL,                                 // seg prefix
  NULL,                                 // ����஫�
  NULL,                                 // atomprefix
  operdim,                              // ���ᨢ ����権
  NULL,                                 // ��४���஢�� � ASCII
  "$",                                  // ����騩 IP
  NULL,                                 // ��������� �㭪樨
  NULL,                                 // ����� �㭪樨
  NULL,                                 // ��४⨢� public
  NULL,                                 // ��४⨢� weak
  NULL,                                 // ��४⨢� extrn
  NULL,                                 // ��४⨢� comm
  NULL,                                 // ������� ��� ⨯�
  ".ALIGN"                              // ���� align
#if IDP_INTERFACE_VERSION > 37
  ,'(', ')',     // lbrace, rbrace
  NULL,    // mod
  NULL,    // and
  NULL,    // or
  NULL,    // xor
  NULL,    // not
  NULL,    // shl
  NULL,    // shr
  NULL,    // sizeof
#endif
};

// ���᮪ ��ᥬ���஢
static asm_t *asms[] = { &pseudosam, NULL };
//-----------------------------------------------------------------------
// ���⪨� ����� �����஢
static const char *shnames[] = { "CR16", NULL };
// ������ ����� �����஢
static const char *lnames[] = { "CR16", NULL };

//--------------------------------------------------------------------------
// ���� �����⮢ �� �/�
static uchar retcode_1[] = { 0x00, 0x0B };    // RTS
static bytes_t retcodes[] = {
 { sizeof(retcode_1), retcode_1 },
 { 0, NULL }
};

//-----------------------------------------------------------------------
//	Processor Definition
//-----------------------------------------------------------------------
processor_t LPH = {
  IDP_INTERFACE_VERSION,        // version
  PLFM_CR16,                    // id ������
#if IDP_INTERFACE_VERSION > 37
  PR_USE32|PR_BINMEM|PR_SEGTRANS,      // can use register names for byte names
  8,							// 8 bits in a byte for code segments
#else
  PR_USE32,         // can use register names for byte names
#endif
  8,                            // 8 bits in a byte

  shnames,                      // ���⪨� ����� �����஢ (�� 9 ᨬ�����)
  lnames,                       // ������ ����� �����஢

  asms,                         // ᯨ᮪ ��������஢

  notify,                       // �㭪�� �����饭��

  CR16_header,                   // ᮧ����� ��������� ⥪��
  CR16_footer,                   // ᮧ����� ���� ⥪��

  CR16_segstart,                 // ��砫� ᥣ����
  std_gen_segm_footer,          // ����� ᥣ���� - �⠭�����, ��� �����襭��

  NULL,                         // ��४⨢� ᬥ�� ᥣ���� - �� �ᯮ�������

  CR16_ana,                      // ����������
  CR16_emu,                      // ������ ������権

  CR16_out,                      // ⥪�⮣������
  CR16_outop,                    // ⥪⮣������ ���࠭���
  CR16_data,                     // ������� ���ᠭ�� ������
  NULL,                         // �ࠢ������� ���࠭���
  NULL,                         // can have type

  qnumber(RegNames),		// Number of registers
  RegNames,			// Regsiter names
  NULL,                         // ������� ���祭�� ॣ����

  0,                            // �᫮ ॣ���஢�� 䠩���
  NULL,                         // ����� ॣ���஢�� 䠩���
  NULL,                         // ���ᠭ�� ॣ���஢
  NULL,				// Pointer to CPU registers
  rVcs,rVds,
#if IDP_INTERFACE_VERSION > 37
  2,                            // size of a segment register
#endif
  rVcs,rVds,
  NULL,                         // ⨯��� ���� ��砫� �����
  retcodes,                     // ���� return'ov
#if IDP_INTERFACE_VERSION <= 37
  NULL,                         // �����頥� ����⭮��� ������� ��᫥����⥫쭮��
#endif
  0,CR16_last,                   // ��ࢠ� � ��᫥���� ������樨
  Instructions,                 // ���ᨢ �������� ������権
  NULL,                         // �஢�ઠ �� �������� ���쭥�� ���室�
#if IDP_INTERFACE_VERSION <= 37
  NULL,                         // ���஥��� �����稪
#endif
  NULL,                         // �࠭���� ᬥ饭��
  3,                            // ࠧ��� tbyte - 24 ���
  NULL,                         // �८�ࠧ���⥫� ������饩 �窨
  {0,0,0,0},                    // ����� ������ � ������饩 �窮�
  NULL,                         // ���� switch
  NULL,                         // ������� MAP-䠩��
  NULL,                         // ��ப� -> ����
  NULL,                         // �஢�ઠ �� ᬥ饭�� � �⥪�
  NULL,                         // ᮧ����� �३�� �㭪樨
#if IDP_INTERFACE_VERSION > 37
  NULL,							// Get size of function return address in bytes (2/4 by default)
#endif
  NULL,                         // ᮧ����� ��ப� ���ᠭ�� �⥪���� ��६�����
  NULL,                         // ������� ⥪�� ��� ....
  0,                            // Icode ��� ������� ������
  NULL,                         // ��।�� ��権 � IDP
  NULL,							// Is the instruction created only for alignment purposes?
  NULL                          // micro virtual mashine
#if IDP_INTERFACE_VERSION > 37
  ,0							// fixup bit's
#endif
};
