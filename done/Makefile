## ======================================================================
## partial Makefile provided to students
##

LIBMONGOOSEDIR = libmongoose/
## don't forget to export LD_LIBRARY_PATH pointing to it
## export LD_LIBRARY_PATH="${PWD}/libmongoose"
LD_LIBRARY_PATH="${PWD}"/libmongoose


VIPS_CFLAGS += $$(pkg-config vips --cflags)
VIPS_LIBS   += $$(pkg-config vips --libs)

LSSLCFLAGS += $$(pkg-config libssl --cflags)
LSSLLIBS   += $$(pkg-config libssl --libs)

LCRYPTOCFLAGS += $$(pkg-config libcrypto --cflags)
LCRYPTOLIBS   += $$(pkg-config libcrypto --libs)

.PHONY: clean new newlibs style \
feedback feedback-VM-CO clone-ssh clean-fake-ssh \
submit1 submit2 submit

CFLAGS += -std=c11 -Wall -pedantic

LDLIBS += $(VIPS_LIBS) $(LSSLLIBS) $(LCRYPTOLIBS) -ljson-c -lm



# a bit more checks if you'd like to (uncomment)
# CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
# -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
# -Wconversion -Wunreachable-code

# ----------------------------------------------------------------------
# feel free to update/modifiy this part as you wish

#used in make clean
UTILITIES := util.o tools.o error.o

TARGETS := imgStore_server
CHECK_TARGETS := tests/test-imgStore-implementation
OBJS := imgst_list.o imgst_create.o imgst_delete.o image_content.o dedup.o imgst_insert.o imgst_read.o  $(UTILITIES)
RUBS = $(OBJS) core
#core is file that contains program's state when it crashed (useful to debug)

all:: clean $(TARGETS)

lib: $(LIBMONGOOSEDIR)/libmongoose.so

# ----------------------------------------------------------------------

imgStoreMgr: $(OBJS) imgStoreMgr.o imgst_gbcollect.o

imgStore_server: lib $(OBJS) imgStore_server.o
	gcc  $(OBJS) imgStore_server.o -L $(LD_LIBRARY_PATH) -lmongoose $(LDLIBS) -o imgStore_server

imgStore_server.o: imgStore_server.c imgStore.h
	gcc $(VIPS_CFLAGS) -c -I libmongoose $<

imgStoreMgr.o: imgStoreMgr.c util.h imgStore.h error.h imgst_gbcollect.c
	gcc $(VIPS_CFLAGS) -c $<

imgst_list.o: imgst_list.c imgStore.h error.h

imgst_create.o: imgst_create.c imgStore.h error.h

imgst_delete.o: imgst_delete.c imgStore.h error.h

image_content.o: image_content.c image_content.h imgStore.h error.h tools.c
	gcc $(VIPS_CFLAGS) -c $<

dedup.o: dedup.c dedup.h

imgst_read.o: imgst_read.c image_content.h imgStore.h

imgst_insert.o: imgst_insert.c image_content.h imgStore.h dedup.h
	gcc $(VIPS_CFLAGS) $(LSSLLIBS) $(LCRYPTOCFLAGS) -c $<

imgst_gbcollect.o: imgst_gbcollect.c imgStore.h tools.c

$(LIBMONGOOSEDIR)/libmongoose.so: $(LIBMONGOOSEDIR)/mongoose.c  $(LIBMONGOOSEDIR)/mongoose.h
	make -C $(LIBMONGOOSEDIR)


# ----------------------------------------------------------------------
# UTILITIES
util.o: util.c

tools.o: tools.c imgStore.h error.h

error.o: error.c

## ======================================================================
## Tests

# target to run black-box tests
check:: all
	@if ls tests/*.*.sh 1> /dev/null 2>&1; then \
	    for file in tests/*.*.sh; do [ -x $$file ] || echo "Launching $$file"; ./$$file || exit 1; done; \
	fi

# all those libs are required on Debian, adapt to your box
$(CHECK_TARGETS): LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

check:: CFLAGS += -I.
check:: $(CHECK_TARGETS)
	export LD_LIBRARY_PATH=.; $(foreach target,$(CHECK_TARGETS),./$(target) &&) true

clean::
	-@/bin/rm -f *.o libmongoose/libmongoose.so *.txt *~ $(CHECK_TARGETS) imgStoreMgr imgStore_server

new: clean all

static-check:
	CCC_CC=$(CC) scan-build -analyze-headers --status-bugs -maxloop 64 make -j1 new

style:
	astyle -n -o -A8 -xt0 *.[ch]

## ======================================================================
## Feedback

IMAGE=chappeli/pps21-feedback:latest
## Note: vous pouvez changer le tag latest pour week04, ou week05, etc.

REPO := $(shell git config --get remote.origin.url)
SSH_DIR := $(HOME)/.ssh

feedback:
	@echo Will use $(REPO) inside container
	@docker pull $(IMAGE)
	@docker run -it --rm -e REPO=$(REPO) -v $(SSH_DIR):/opt/.ssh $(IMAGE)

clone-ssh:
	@-$(eval SSH_DIR := $(HOME)/.$(shell date "+%s;$$"|sha256sum|cut -c-32))
	@cp -r $(HOME)/.ssh/. $(SSH_DIR)

clean-fake-ssh:
	@case $(SSH_DIR) in $(HOME)/\.????????????????????????????????) $(RM) -fr $(SSH_DIR) ;; *) echo "Dare not remove \"$(SSH_DIR)\"" ;; esac

feedback-VM-CO: clone-ssh feedback clean-fake-ssh

## ======================================================================
## Submit

SUBMIT_SCRIPT=../provided/submit.sh
submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit:
	@printf 'what "make submit"??\nIt'\''s either "make submit1" or "make submit2"...\n'

