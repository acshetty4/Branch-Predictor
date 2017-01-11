CC = g++
OPT = -g  
WARN = -Wall
CFLAGS = $(OPT) $(WARN) 

# List all your .cc files here (source files, excluding header files)
SIM_SRC = Main.cc Bimodal.cc BTB.cc Gshare.cc Hybrid.cc Predictor.cc YehPatt.cc

# List corresponding compiled object files here (.o files)
SIM_OBJ = Main.o Bimodal.o BTB.o Gshare.o Hybrid.o Predictor.o YehPatt.o



 
#################################

# default rule

all: sim_bp
	@echo "my work is done here..."


# rule for making sim_cache
sim_bp: $(SIM_OBJ)
	$(CC) -o sim_bp $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH SIM_BP -----------"


# generic rule for converting any .cc file to any .o file
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


# type "make clean" to remove all .o files plus the sim_bp binary
clean:
	rm -f *.o sim_bp


# type "make clobber" to remove all .o files (leaves sim_bp binary)
clobber:
	rm -f *.o

