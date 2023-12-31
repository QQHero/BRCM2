#
# wlconf.h make file
#
# Create wlconf.h from either wltunable.h specified via variable WLTUNEFILE
# or brand make file.
#
# Default *CONF* in variable WLTUNECONFS to 0 if not present.
#

# when WLTUNEFILE is not specified the following variables are used to
# create a file wltunable.h in which these variables are converted to
# a list of #define <variable> <value>
WLTUNECONFS = D11CONF D11CONF2 D11CONF3 D11CONF4 D11CONF5
WLTUNECONFS += NCONF LCN20CONF ACCONF ACCONF2 ACCONF5 HECONF

WLTUNEPARMS = NTXD NRXD NRXBUFPOST RXBND
WLTUNEPARMS += MAXSCB

wlconf.h: TMPFILE=tmp.$@
wlconf.h: wltunable.h
ifneq ($(CHIP),43xx)
	@cp $< $(TMPFILE) && \
	( \
	echo ''; \
	echo '/* Makefile added tunables */'; \
	$(foreach CONF,$(WLTUNECONFS), \
		$(if $(shell grep -e "^#define\\b[^\\S]\\+\\b$(CONF)\\b" $<), \
			, \
			echo '#define $(CONF) 0'; \
		) \
	) \
	echo ''; \
	) >> $(TMPFILE) && \
	$(WLCFGDIR)/diffupdate.sh $(TMPFILE) $@
else
	@cp $< $(TMPFILE) && \
	$(WLCFGDIR)/diffupdate.sh $(TMPFILE) $@
endif

ifdef WLCFGDIR
wltunable.h: TMPFILE=tmp.$@
ifdef WLTUNEFILE
wltunable.h: FROMFILE=$(notdir $<)
wltunable.h: $(WLCFGDIR)/$(WLTUNEFILE)
	@( \
	echo '/* Makefile generated wltunable file (from $(notdir $<)). */'; \
	echo ''; \
	echo '#ifndef __$(FROMFILE:.h=_h)__'; \
	echo '#define __$(FROMFILE:.h=_h)__'; \
	echo ''; \
	cat $< ; \
	echo ''; \
	echo '#endif	/* __$(FROMFILE:.h=_h)__ */'; \
	) > $(TMPFILE) && \
	$(WLCFGDIR)/diffupdate.sh $(TMPFILE) $@
else	# !WLTUNEFILE
wltunable.h: FORCE
	@( \
	echo '/* Makefile generated wltunable file */'; \
	echo ''; \
	echo '#ifndef __$(@:.h=_h)__'; \
	echo '#define __$(@:.h=_h)__'; \
	echo ''; \
	echo '/* dot11 mac/phy config */'; \
	$(foreach CONF,$(WLTUNECONFS),echo '#define $(CONF) $(if $($(CONF)),$($(CONF)),0)';) \
	echo ''; \
	echo '/* wl driver config */'; \
	$(foreach PARM,$(WLTUNEPARMS),$(if $($(PARM)),echo '#define $(PARM) $($(PARM))';)) \
	echo ''; \
	echo '#endif	/* __$(@:.h=_h)__ */'; \
	) > $(TMPFILE) && \
	$(WLCFGDIR)/diffupdate.sh $(TMPFILE) $@
endif	# !WLTUNEFILE

endif	# WLCFGDIR

# Tell emacs to use Makefile mode since it does not know from the filename:
#       Local Variables:
#       mode: makefile
#       End:
