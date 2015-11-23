.PHONY: all
PROJNAME:=che
SRC_DIR:=./src
TEST_DIR:=./tests
TIME_TEST:=che_time_test

PLATFORM=$(shell uname -s)
ifeq ($(PLATFORM),Darwin)
PLATFORM_DIR=$(SRC_DIR)/platform/apple
else
PLATFORM_DIR=$(SRC_DIR)/platform/linux
endif

CHE_INCLUDES=-I$(SRC_DIR) -I $(PLATFORM_DIR)
CFLAGS+=-g -Wall -Werror $(CHE_INCLUDES)

SRCFILES:=$(SRC_DIR)/che.c\
		$(SRC_DIR)/che_time.c\
		$(SRC_DIR)/che_log.c\
		$(SRC_DIR)/che_machine.c\
		$(SRC_DIR)/che_cycle.c\
		$(SRC_DIR)/che_scr.c\
		$(SRC_DIR)/che_rand.c\
		$(PLATFORM_DIR)/che_time_platform.c
OBJFILES:=$(patsubst %.c,%.o,$(SRCFILES))
DEPFILES:=$(patsubst %.c,%.d,$(SRCFILES))

TIME_TEST_SRCFILES:=$(TEST_DIR)/che_time_test.c\
	  $(SRC_DIR)/che_time.c\
	  $(PLATFORM_DIR)/che_time_platform.c
TIME_TEST_OBJFILES:=$(patsubst %.c,%.o,$(TIME_TEST_SRCFILES))
TIME_TEST_DEPFILES:=$(patsubst %.c,%.d,$(TIME_TEST_SRCFILES))

all: $(PROJNAME) $(TIME_TEST)

clean:
	-@$(RM) $(wildcard $(OBJFILES) $(DEPFILES) $(PROJNAME))

-include $(DEPFILES)

$(PROJNAME): $(OBJFILES)
	@$(CC) $(LDFLAGS) $^ -o $@

$(TIME_TEST): $(TIME_TEST_OBJFILES)
	@$(CC) $(LDFLAGS) $^ -o $@


%.o : %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@


