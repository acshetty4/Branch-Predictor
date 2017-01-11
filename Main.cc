#include <iostream>
using namespace std;

#include <string.h>
#include <math.h> 
#include <stdlib.h>
#include <stdio.h>

#include "Predictor.h"
#include "Bimodal.h"
#include "Gshare.h"
#include "Hybrid.h"
#include "YehPatt.h"

const char* gpcPredictorType[] = { "bimodal", "gshare", "hybrid", "yehpatt" };

void printCommandLine(int argc, char* argv[])
{
	cout << "Command Line:\n";
	cout << "./sim_bp ";
	for (int i = 1; i < argc; i++)
	{
		cout << argv[i] << " ";
	}
	cout << endl << endl;
}

int main(int argc, char* argv[])
{
	char * predictor, * tracefile;
	tracefile = (char*)malloc(32);

	Predictor * pobjPredictor = NULL;
	FILE * pFile;
	int addr;
	char direction;

	if (argc > 1)
	{
		printCommandLine(argc, argv);

		predictor = argv[1];
		if (strcmp(predictor, gpcPredictorType[0]) == 0)
		{
			// Create object of type bimodal
			Bimodal * pobjBimodal = new Bimodal(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
			pobjPredictor = pobjBimodal;

			tracefile = argv[5];
		}
		else if (strcmp(predictor, gpcPredictorType[1]) == 0)
		{
			// Create object of type gshare
			Gshare * pobjGshare = new Gshare(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
			pobjPredictor = pobjGshare;

			tracefile = argv[6];
		}
		else if (strcmp(predictor, gpcPredictorType[2]) == 0)
		{
			// Create object of type hybrid
			Hybrid * pobjHybrid = new Hybrid(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
			pobjPredictor = pobjHybrid;

			tracefile = argv[8];
		}
		else if (strcmp(predictor, gpcPredictorType[3]) == 0)
		{
			// Create object of type yehpatt
			YehPatt * pobjYehpatt = new YehPatt(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
			pobjPredictor = pobjYehpatt;

			tracefile = argv[6];
		}

		if (pobjPredictor != NULL)
		{
			pFile = fopen(tracefile, "r");
			if (pFile == NULL)
			{
				cout << "Incorrect tracefile";
				return 1;
			}

			while (fscanf(pFile, "%x %c", &addr, &direction) != EOF)
			{
				pobjPredictor->m_iNumberOfBranches++;
				pobjPredictor->update(addr, direction);
			}

			// Print output
			pobjPredictor->printSimOutput();
		}
	}
	return 0;
}