include ../../src/macros.make

INSTALL= cp

GRIB_TABLES = \
	g2varsncep0.tbl \
	g2varsncep1.tbl \
	g2varswmo1.tbl \
	g2varswmo2.tbl \
	g2vcrdncep0.tbl \
	g2vcrdncep1.tbl \
	g2vcrdwmo1.tbl \
	g2vcrdwmo2.tbl \
        g2vcrdwmo3.tbl \
	grdnav.tbl \
	wmocenter.tbl


install:
	@for file in $(GRIB_TABLES) ""; do \
	   if test "$$file" ; then \
	      $(MAKE) GFILE=$$file $(ETCDIR)/$$file ; \
	   fi; \
	done;

$(ETCDIR)/$(GFILE): $(ETCDIR) $(GFILE)
	($(INSTALL) $(GFILE) $@ && echo '$@ installed') 

include ../../src/rules.make
