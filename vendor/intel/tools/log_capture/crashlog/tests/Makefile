COMPILER	= gcc
#COMPILER	= clang
CC 		= ${COMPILER}
LINK 		= ${COMPILER} -o

TEST_USER 	= $(shell grep `whoami` /etc/passwd > .tmp.txt && awk -F: '{ print $$3 }' .tmp.txt && rm -f .tmp.txt)

CFLAGS 		= -D__LINUX__ -DTEST_USER=$(TEST_USER) -D__TEST__
CFLAGS 	   	+= -g3 -Istubs -I ..

CHECKFLAGS 	= -Wall -Wextra

LDFLAGS 	= -g3 -Wall -Wextra

# TESTTARGETS = \
# 	bin/test_fsutils \
# 	bin/test_inotify \
# 	bin/test_crashutils \
# 	bin/test_history \
# 	bin/test_crashlogd

TESTTARGETS = \
	bin/test_fsutils

FULLTARTGET	= bin/crashlogd

# all: check_dirs $(TESTTARGETS) $(FULLTARTGET)
all: check_dirs $(TESTTARGETS)


# Rule for test sources
obj/%.o:%.c
	$(CC) -c $(CFLAGS) $(CHECKFLAGS) $< -o $@

# Rule for project sources
obj/%.o:../%.c
	$(CC) -c $(CFLAGS) $(CHECKFLAGS) $< -o $@
	
obj/crashlogorig.o:crashlogorig.c
	$(CC) -c $(CFLAGS) $< -o $@

bin/test_fsutils: obj/test_fsutils/main.o \
	obj/fsutils.o \
	obj/stubs/properties.o
	$(CC) $(LDFLAGS) $(CHECKFLAGS) -o $@ $^

bin/test_inotify: obj/test_inotify/main.o \
	obj/inotify_handler.o
	$(CC) $(LDFLAGS) $(CHECKFLAGS) -o $@ $^
	
bin/test_crashutils: obj/test_crashutils/main.o \
	obj/crashutils.o \
	obj/history.o \
	obj/fsutils.o \
	obj/crashlogorig.o \
	obj/stubs/properties.o \
	obj/stubs/sha1.o
	$(CC) $(LDFLAGS) $(CHECKFLAGS) -o $@ $^

bin/test_history: obj/test_history/main.o \
	obj/crashutils.o \
	obj/history.o \
	obj/fsutils.o \
	obj/stubs/properties.o \
	obj/stubs/sha1.o
	$(CC) $(LDFLAGS) $(CHECKFLAGS) -o $@ $^
	
bin/test_crashlogd: obj/test_crashlogd/main.o \
	obj/crashutils.o \
	obj/anruiwdt.o \
	obj/history.o \
	obj/dropbox.o \
	obj/fsutils.o \
	obj/crashlogorig.o \
	obj/stubs/properties.o \
	obj/stubs/sha1.o
	$(CC) $(LDFLAGS) $(CHECKFLAGS) -o $@ $^
	
bin/crashlogd: obj/main.o \
	obj/inotify_handler.o \
	obj/startupreason.o \
	obj/mmgr_source.o \
	obj/crashutils.o \
	obj/usercrash.o \
	obj/anruiwdt.o \
	obj/recovery.o \
	obj/history.o \
	obj/dropbox.o \
	obj/fsutils.o \
	obj/trigger.o \
	obj/fabric.o \
	obj/modem.o \
	obj/panic.o \
	obj/crashlogorig.o \
	obj/stubs/properties.o \
	obj/stubs/sha1.o
	$(CC) $(LDFLAGS) $(CHECKFLAGS) -o $@ $^ -lpthread

cleanup_resources:
	@echo "Cleanup resources"
	@$(RM) res/*_copy
	@$(RM) res/file_to_append
	@$(RM) res/properties.txt
	@$(RM) res/logs/current*
	@$(RM) res/logs/uuid.txt
	@$(RM) res/logs/history_event
	@$(RM) res/logs/modemid.txt
	@$(RM) res/logs/info/*
	@$(RM) res/logs/modemcrash/*
	@$(RM) -r res/logs/crashlog*
	@$(RM) -r res/logs/bz*
	@$(RM) -r res/logs/aplogs/*
	@$(RM) -r res/logs/stats/*
	@$(RM) -r res/mnt/sdcard/logs/crashlog*
	@$(RM) -r res/mnt/sdcard/logs/bz*
	@$(RM) -r res/mnt/sdcard/logs/aplogs*
	@$(RM) -r res/mnt/sdcard/logs/stats*

clean: cleanup_resources
	@echo "Clean objects and binaries"
	@if [ -d obj ]; then \
	    find obj -name "*.o" -exec $(RM) {} \; 2>&1 || exit 0; \
	fi
	@$(RM) $(TESTTARGETS) $(FULLTARTGET)
	@$(RM) .tmp.out

check_dirs:
	@if [ ! -d res/logs/aplogs ]; then \
	    echo "Create res directories" ; \
	    mkdir -p res/logs/aplogs res/logs/stats res/logs/core ; \
	    mkdir -p res/logs/modemcrash res/logs/info res/data/tombstones ; \
	    mkdir -p res/mnt/sdcard/logs res/data/system/dropbox ; \
	fi
	@if [ ! -d obj ]; then \
	    echo "Create obj directories" ; \
	    mkdir -p bin obj/test_fsutils obj/test_inotify obj/test_crashutils ; \
	    mkdir -p obj/test_crashlogd obj/test_history obj/stubs ; \
	fi

tests: $(TESTTARGETS)
	@for t in $(TESTTARGETS); do \
		echo -n "Running test $$t..."; \
		./$$t 2>/dev/null > .tmp.out; \
		if [ $$? != 0 ]; then echo !!!!failed!!!!; \
		else \
		    grep -i "failed" .tmp.out; \
		    if [ $$? != 0 ]; then echo succeeded; \
		    else echo !!!!failed!!!!; \
		fi fi \
	done
