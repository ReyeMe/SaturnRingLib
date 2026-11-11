# Builds every test campaign (Tests/campaigns/*). `make <campaign>` builds one.
CAMPAIGNS = math memory_simple memory_tlsf video system cd

# Campaigns share SRL/module object files, so build them one at a time.
.NOTPARALLEL:

all: $(CAMPAIGNS)

$(CAMPAIGNS):
	$(MAKE) -C campaigns/$@ all

clean:
	@for c in $(CAMPAIGNS); do $(MAKE) -C campaigns/$$c clean || exit 1; done

.PHONY: all clean $(CAMPAIGNS)
