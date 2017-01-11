// To prevent redefinition
#ifndef GSHARE_H
#define GSHARE_H

#include "Predictor.h"
#include "BTB.h"

class Gshare : public Predictor
{
private:
	int mask, hmask, mask_btb;
public:
	// input variable
	int m_idxTable; // number of PC bits used to index the share table
	int m_idxH;		// number of GHR bits used to index the gshare table
	int m_idxBTB;	// number of PC bits used to index the BTB
	int m_assocBTB; // BTB associativity

	// 
	int * m_piPredictorTable;

	int m_iGlobalHistoryRegister;

	BTB * m_pobjBTB;

	int m_iPredictionsFromBranch;

	int m_iMispredctionFromBranch;

	int m_iMispredctionFromBTB;
	
	bool m_bHybridObject;
	
	Gshare(int iG, int h, int iBTB, int assocBTB);

	int getIndex(int address);					// Determine the branch�s index into the prediction table
	int getHistory(int index);					// Determine the h-bits
	char prediction(int index);					// Make a prediction.Predict taken if counter >= 2, else predict not taken
	void update(int address, char direction);	// Update the branch predictor based on the branch�s actual outcome.

	char prediction_btb(int address);			// If no tag match, predict not taken. Else, ask branch predictor to predict

	void printSimOutput();

	void incrementCounter(int index);	// Increment the counter if the branch was actually taken. Counter saturates at 3
	void decrementCounter(int index);	// Decrement the counter if the branch was not taken. Counter saturates at 0

	void printBtbContents();	// prints btb table content
	void printTableContents();	// prints final bimodal table content
	void printStatistics();		// prints final branch predictor statistics
};
#endif