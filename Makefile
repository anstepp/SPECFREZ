include ../package.conf

NAME = SPECFREZ

OBJS = $(NAME).o
CMIXOBJS += $(PROFILE_O)
CXXFLAGS += -I. -Wall 
PROGS = $(NAME) lib$(NAME).so

all: lib$(NAME).so

standalone: $(NAME)

install: lib$(NAME).so
	$(INSTALL) $(CURDIR)/lib$(NAME).so $(LIBDESTDIR)

lib$(NAME).so: $(OBJS) $(GENLIB)
	$(CXX) $(SHARED_LDFLAGS) -o $@ $(OBJS) $(GENLIB) $(SYSLIBS)

$(NAME): $(OBJS) $(CMIXOBJS)
	$(CXX) -o $@ $(OBJS) $(CMIXOBJS) $(LDFLAGS)

$(OBJS): $(INSTRUMENT_H) $(NAME).h

clean:
	$(RM) $(OBJS) $(PROGS)

