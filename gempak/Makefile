ALIB = gemgrib.a

COBJS = \
	$(ALIB)(gb22gem.o) \
	$(ALIB)(gb2gtcntr.o) \
	$(ALIB)(gb2ftim.o) \
	$(ALIB)(gb2param.o) \
	$(ALIB)(gb2proc.o) \
	$(ALIB)(gb2vcrd.o) \
	$(ALIB)(gb2fcsm.o) \
	$(ALIB)(gb2gdsnav.o) \
	$(ALIB)(gb2gtlcllvltbl.o) \
	$(ALIB)(gb2gtlclvartbl.o) \
	$(ALIB)(gb2gtwmolvltbl.o) \
	$(ALIB)(gb2gtwmovartbl.o) \
	$(ALIB)(gb2vlev.o) \
	$(ALIB)(gb2skvar.o) \
	$(ALIB)(gb2ctim.o) \
	$(ALIB)(gb2prob.o) \
	$(ALIB)(gb2sklvl.o) \
	$(ALIB)(cflclos.o) \
	$(ALIB)(cflinqr.o) \
	$(ALIB)(cfliret.o) \
	$(ALIB)(cflpath.o) \
	$(ALIB)(cflropn.o) \
	$(ALIB)(cfltbnr.o) \
	$(ALIB)(cfltbop.o) \
	$(ALIB)(cfltinq.o) \
	$(ALIB)(cfltrln.o) \
	$(ALIB)(cstctoi.o) \
	$(ALIB)(cstinch.o) \
	$(ALIB)(cstitoc.o) \
	$(ALIB)(cstlcuc.o) \
	$(ALIB)(cstldsp.o) \
	$(ALIB)(cstlstr.o) \
	$(ALIB)(cstncat.o) \
	$(ALIB)(cstncpy.o) \
	$(ALIB)(cstnocc.o) \
	$(ALIB)(cstrmbl.o) \
	$(ALIB)(cstrmst.o) \
	$(ALIB)(cstrpst.o) \
	$(ALIB)(cstrxbl.o) \
	$(ALIB)(cstsplit.o) \
	$(ALIB)(cstuclc.o) \
	$(ALIB)(cssenvr.o) \
	$(ALIB)(ctbg2gnam.o) \
	$(ALIB)(ctbg2gnum.o) \
	$(ALIB)(ctbg2rdcntr.o) \
	$(ALIB)(ctbg2rdlvl.o) \
	$(ALIB)(ctbg2rdvar.o) \
	$(ALIB)(ctbg2read.o) \
	$(ALIB)(erwmsg.o) \
	$(ALIB)(lvcord.o) \
	$(ALIB)(tgitoc.o) \
	$(ALIB)(tiaddm.o) \
	$(ALIB)(tidaym.o) \
	$(ALIB)(tiitoj.o) \
	$(ALIB)(timdif.o) \
	$(ALIB)(decode_g2gnum.o)


all: $(ALIB)

$(ALIB): $(COBJS)
	-rm -f *.o

clean:
	-rm -f *.o $(ALIB)
