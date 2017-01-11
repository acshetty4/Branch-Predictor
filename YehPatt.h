#include "Predictor.h"
#include "BTB.h"

class YehPatt : public Predictor
{
private:
	int mask, mask_btb, mask_p;

public:
	int m_idxH;
	int m_idxP;
	int m_idxBTB;	// number of PC bits used to index the BTB
	int m_assocBTB; // BTB associativity

	BTB * m_pobjBTB;

	int * m_piPredictionTable;

	int * m_piHistoryTable;

	int m_iPredictionsFromBranch;

	int m_iMispredctionFromBranch;

	int m_iMispredctionFromBTB;

	YehPatt(int h, int p, int iBtb, int iAssocBtb);

	int getHIndex(int address);					// Determine the index into history table
	int getPIndex(int history);					// Determine the index into prediction table 
	int getHistory(int index);					// Determine the h-bits
	char prediction(int index);					// Make a prediction.Predict taken if counter >= 2, else predict not taken
	void update(int address, char direction);	// Update the branch predictor based on the branch’s actual outcome.

	char prediction_btb(int address);			// If no tag match, predict not taken. Else, ask branch predictor to predict

	void printSimOutput();

	void incrementCounter(int index);	// Increment the counter if the branch was actually taken. Counter saturates at 3
	void decrementCounter(int index);	// Decrement the counter if the branch was not taken. Counter saturates at 0

	void printBtbContents();			// prints btb table content
	void printPredictionTableContents();// prints prediction table content
	void printHistoryTableContents();	// prints history table content
	void printStatistics();				// prints final branch predictor statistics
};