Q ?= @
PROJ = leitshow
SRC = test.c

OBJ = $(SRC:%.c=%.o) 

## Compile pedantically and save pain later
WARNINGFLAGS ?= -Wall -Wextra -Werror -Wshadow -std=gnu99
DEBUGFLAGS ?= -g -DDEBUG # -pg to generate profiling information
FEATUREFLAGS ?= 

# Swap these two lines if you don't have GCC 4.5+.  
OPTFLAGS ?= -march=native -O3 -msse4.1 -fcx-fortran-rules -flto
# OPTFLAGS ?= -march=native -O3 -msse4.1 -fcx-fortran-rules 

CC = gcc-4.5
# CC ?= gcc

LDFLAGS ?= -lpulse-simple -lpulse -lfftw3f -lm 

CFLAGS ?= $(WARNINGFLAGS) $(DEBUGFLAGS) $(FEATUREFLAGS) $(INCLUDES) $(OPTFLAGS) 

.PHONY: clean all

all: $(PROJ)

$(PROJ): $(OBJ)
	@echo CC $<
	$(Q)$(CC) $(WARNINGFLAGS) $(OPTFLAGS) $< -o $@ $(LDFLAGS)

%.o : %.c
	@echo CC $<
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

%.o : %.cpp
	@echo CXX $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(PROJ)
