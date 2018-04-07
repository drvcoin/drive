CC = g++
BINDIR = ../out/bin
LIBDIR = ../out/lib
INCDIR = ../out/inc
OBJDIR = ../out/obj/$(TARGET)

_OBJS0   = ${SOURCES:.cpp=.o}
_OBJS1   = ${_OBJS0:.c=.o}
_OBJS2   = ${_OBJS1:.y=.y.o}
_OBJS    = ${_OBJS2:.l=.l.o}
OBJS    = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

_DEPS0   = ${SRCS:.cpp=.dep}
_DEPS    = ${SRCS:.c=.dep}
DEPS     = $(patsubst %,$(OBJDIR)/%,$(_DEPS0))
DEPS     += $(patsubst %,$(OBJDIR)/%,$(_DEPS))

_HEADERS = $(patsubst %,$(INCDIR)/%,$(HEADERS))

ifeq ($(TARGET_TYPE),executable)
  _TARGET = $(BINDIR)/$(TARGET)
else
  ifeq ($(TARGET_TYPE),static)
    _TARGET = $(LIBDIR)/$(TARGET).a
  else
    ifeq ($(TARGET_TYPE),dynamic)
      _TARGET = $(LIBDIR)/$(TARGET).so
    else
      ifeq ($(TARGET_TYPE),)
        _TARGET = $(BINDIR)/$(TARGET)
      else
        $(error "Unknown value for TARGET_TYPE: $(TARGET_TYPE)")
      endif
    endif
  endif
endif

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $$(dirname $@)
	@echo "compile: $<"
	@$(CC) $(CCFLAGS) -Wp,-MD,${patsubst %.o,%.dep,$@} -Wp,-MT,$@ -Wp,-MP -o $@ -c $<

all: $(_TARGET) $(_HEADERS)

$(BINDIR)/$(TARGET): $(OBJS) $(LIBS)
	@echo "link: $@"
	@mkdir -p $$(dirname $@)
	@$(CXX) -o $@ $^ ${LDFLAGS}

$(LIBDIR)/$(TARGET).a: $(OBJS)
	@echo "link: $@"
	@mkdir -p $$(dirname $@)
	@rm -f $@
	@ar -cvq $@ $^

$(LIBDIR)/$(TARGET).so: $(OBJS) $(LIBS)
	@echo "link: $@"
	@mkdir -p $$(dirname $@)
	@$(CXX) -o $@ $^ ${LDFLAGS}

$(INCDIR)/%: %
	@echo "copy: $<"
	@mkdir -p $$(dirname $@)
	@cp -f $< $@

clean:
	@rm cm256.a
