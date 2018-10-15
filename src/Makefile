YICES_DIR = ../lib/yices-1.0.40

CFLAGS = -I. -I$(YICES_DIR)/include -O2 -Wno-narrowing
CXXFLAGS = $(CFLAGS)
LDFLAGS = -L$(YICES_DIR)/lib
LOADLIBES = -lyices

BASE_LIBS = base/basic_types.o base/symbolic_execution.o \
            base/symbolic_interpreter.o base/symbolic_path.o \
            base/symbolic_predicate.o base/symbolic_expression.o \
            base/yices_solver.o

PARA = run_crest/concolic_search.o run_crest/feature.o

all: libcrest/libcrest.a run_crest/run_crest \
     process_cfg/process_cfg tools/print_execution \
     install

libcrest/libcrest.a: libcrest/crest.o $(BASE_LIBS)
	$(AR) rsv $@ $^

run_crest/run_crest: $(PARA) $(BASE_LIBS)

$(PARA): %.o: %.cc %.h
	g++ $(CXXFLAGS) $(LDFLAGS) -std=c++11 -c $< -o $@ 

tools/print_execution: $(BASE_LIBS)


install:
	cp libcrest/libcrest.a ../lib
	cp run_crest/run_crest ../bin
	cp process_cfg/process_cfg ../bin
	cp tools/print_execution ../bin
	cp libcrest/crest.h ../include

clean:
	rm -f libcrest/libcrest.a run_crest/run_crest
	rm -f process_cfg/process_cfg tools/print_execution
	rm -f */*.o */*~ *~
