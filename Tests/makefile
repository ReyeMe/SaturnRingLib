# Builds every test campaign (Tests/campaigns/*). `make <campaign>` builds one.
CAMPAIGNS = math memory_simple memory_tlsf memory_cpus_simple memory_cpus_tlsf video system timer timer_pal event datetime cd media slave scene string endian vdp1 vdp2 input sound transform tilemap draw2d fld

# Campaigns share SRL/module object files, so build them one at a time.
.NOTPARALLEL:

all: $(CAMPAIGNS)

$(CAMPAIGNS):
	$(MAKE) -C campaigns/$@ all

clean:
	@for c in $(CAMPAIGNS); do $(MAKE) -C campaigns/$$c clean || exit 1; done

.PHONY: all clean $(CAMPAIGNS)
