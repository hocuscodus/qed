# Makefile with autodependencies and separate output directories
# - $BUILDDIR/ is used for object and other build files
# - $BINDIR/ is used for native binaries
# - assets/ is used for assets needed when running native binaries
# - $WWWDIR/ is used for emscripten output
#
# For native (Mac OS X, Linux) builds, $(BINDIR)/ and assets/ are needed
# For emscripten builds, $(WWWDIR)/ is needed
SED=s/\.cpp$$//
MODULES = $(shell (cd src && ls -1 *.cpp | sed -e '$(SED)') | tr '\n' ' ')
ASSETS = qedlib.qed
#qed-small6.png res/font/arial.ttf incdec.qed

UNAME = $(shell uname -s)
BUILDDIR = build
BINDIR = bin
WWWDIR = www
_MKDIRS := $(shell mkdir -p $(BINDIR) $(WWWDIR) $(BUILDDIR))

COMMONFLAGS = -std=c++11 -MMD -MP -isystem .
LOCALFLAGS = -g $(COMMONFLAGS) -D_REENTRANT -I/home/linuxbrew/.linuxbrew/include/SDL2
#LOCALFLAGS = -g -O2 $(COMMONFLAGS) -D_REENTRANT -I/home/linuxbrew/.linuxbrew/include/SDL2

# Choose the warnings I want, and disable when compiling third party code
NOWARNDIRS = imgui/ stb/
LOCALWARN = -Wall -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wfloat-equal -Wno-unused-function -Wno-unused-parameter
# NOTE: also useful but noisy -Wconversion -Wshorten-64-to-32

LOCALLIBS = -L/home/linuxbrew/.linuxbrew/lib -Wl,-rpath,/home/linuxbrew/.linuxbrew/lib -Wl,--enable-new-dtags
ifeq ($(UNAME),Darwin)
	LOCALLIBS += -Wl,-dead_strip -framework OpenGL
else
#	LOCALLIBS += -lGL
endif

EMXX = em++
##EMXXFLAGS = $(COMMONFLAGS) -O0 -s USE_SDL_TTF=2 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_GFX=2 -g -fdebug-compilation-dir='..'
##EMXXFLAGS = $(COMMONFLAGS) -Oz -s USE_SDL_TTF=2 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_GFX=2
EMXXFLAGS = $(COMMONFLAGS) -O0 -g -fdebug-compilation-dir='..'
#EMXXFLAGS = $(COMMONFLAGS) -Oz
# -s SAFE_HEAP=1 -s ASSERTIONS=2 --profiling  -s DEMANGLE_SUPPORT=1
EMXXLINK = -s TOTAL_MEMORY=50331648 -s ALLOW_MEMORY_GROWTH=1 --use-preload-plugins \
	-s EXPORTED_RUNTIME_METHODS="['callMain', 'ccall', 'cwrap']" \
	-s EXPORTED_FUNCTIONS="['_main', '_runSource']" \
	-s INVOKE_RUN=0 -sLLD_REPORT_UNDEFINED --bind

all: $(BINDIR)/qed

$(WWWDIR): $(WWWDIR)/index.html $(WWWDIR)/qed.js

$(BINDIR)/qed: $(MODULES:%=$(BUILDDIR)/%.o) Makefile
	$(CXX) $(LOCALFLAGS) $(filter %.o,$^) $(LOCALLIBS) -o $@

$(WWWDIR)/index.html: emscripten-shell.html
	cp emscripten-shell.html $(dir $@)index.html

$(WWWDIR)/qed.js: $(MODULES:%=$(BUILDDIR)/%.em.o) $(ASSETS) Makefile
	$(EMXX) $(EMXXFLAGS) $(EMXXLINK) $(filter %.o,$^) $(ASSETS:%=--preload-file %) -o $@

$(BUILDDIR)/%.em.o: src/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo $(EMXX) -c $< -o $@
	@$(EMXX) $(EMXXFLAGS) -c $< -o $@

# The $(if ...) uses my warning flags only in WARNDIRS
$(BUILDDIR)/%.o: src/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo $(CXX) -c $< -o $@
	@$(CXX) $(LOCALFLAGS) $(if $(filter-out $(NOWARNDIRS),$(dir $<)),$(LOCALWARN)) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)/* $(BINDIR)/* $(WWWDIR)/*

include $(shell find $(BUILDDIR) -name \*.d)
