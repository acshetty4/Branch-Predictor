#include "Predictor.h"
#include "Bimodal.h"
#include "Gshare.h"

class Hybrid : public Predictor
{
private:
	int kmask, mask_btb;

public:
	Bimodal* m_pobjBimodal;
	Gshare* m_pobjGshare;

	int * m_piChooserTable;

	int m_iCTable;
	
	BTB * m_pobjBTB;

	int m_iPredictionsFromBranch;
	int m_iMispredctionFromBranch;
	int m_iMispredctionFromBTB;

	Hybrid(int iC, int iG, int h, int iB, int iBTB, int assocBTB);

	void update(int address, char direction);
	char prediction_btb(int address);			// If no tag match, predict not taken. Else, ask branch predictor to predict

	void printSimOutput();

	void printBtbContents();	// prints btb table content
	void printChooserTable();
	void printStatistics();

	int getChooserTableIndex(int address);

private:
	void incrementCounter(int index);	// Increment the counter if the branch was actually taken. Counter saturates at 3
	void decrementCounter(int index);	// Decrement the counter if the branch was not taken. Counter saturates at 0
};