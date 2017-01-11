#include <iostream>
using namespace std;

#include <string>
#include <math.h> 
#include <stdlib.h>
#include <stdio.h>

#include "Predictor.h"
#include "Hybrid.h"

Hybrid::Hybrid(int iC, int iG, int h, int iB, int iBTB, int assocBTB)
{
	int iChooserTableEntries = 0;

	m_iCTable = iC;

	m_pobjBimodal = new Bimodal(iB, iBTB, assocBTB);
	m_pobjGshare = new Gshare(iG, h, iBTB, assocBTB);

	m_pobjBimodal->m_bHybridObject = true;
	m_pobjGshare ->m_bHybridObject = true;

	iChooserTableEntries = pow(2, iC);

	m_piChooserTable = new int[iChooserTableEntries];

	if (iBTB != 0 && assocBTB != 0)
	{
		int iBlockSize = 64; // TO_DO : Check if this is correct
		int iSize = iBlockSize * assocBTB * pow(2, iBTB);

		m_pobjBTB = new BTB(1, iBlockSize, iSize, assocBTB, 0, 0);
		m_pobjBTB->Init();
	}

	m_iPredictionsFromBranch = m_iMispredctionFromBranch = m_iMispredctionFromBTB = 0;

	// initialise to 1
	for (int i = 0; i < iChooserTableEntries; i++)
	{
		m_piChooserTable[i] = 1;
	}

	for (int i = 0; i < m_iCTable; i++)
	{
		kmask <<= 1;
		kmask |= 1;
	}

	kmask <<= 2;
	
	for (int i = 0; i < m_pobjBimodal->m_idxBTB; i++)
	{
		mask_btb <<= 1;
		mask_btb |= 1;
	}

	mask_btb <<= 2;
}

int Hybrid::getChooserTableIndex(int address){
	int index = address & kmask;
	index >>= 2;
	return index;
}

void Hybrid::incrementCounter(int index)
{
	if (m_piChooserTable[index] < 3)
	{
		m_piChooserTable[index] ++;
	}
}

void Hybrid::decrementCounter(int index)
{
	if (m_piChooserTable[index] > 0)
	{
		m_piChooserTable[index] --;
	}
}

char Hybrid::prediction_btb(int address)
{
	char cPrediction = 't';

	int index_btb = address & mask_btb;
	index_btb >>= 2;

	int tag_btb = address;

	if (true == m_pobjBTB->Read(index_btb, tag_btb))
	{
		// When read returns true it means that the cache was not placed, 
		// i.e. it was a btb miss -> Predict not taken
		cPrediction = 'n';
	}

	return cPrediction;
}

void Hybrid::update(int address, char direction)
{
	int iBimodalIndex, iGshareIndex, iHybridIndex, iCurrentPredictor;
	char cBimodalPrediction, cGsharePrediction, cHybridPrediction;
	
	// Step 0: Check BTB
	bool bBranchPredictionNeeded = true;
	if (m_pobjBimodal->m_assocBTB != 0 && m_pobjBimodal->m_idxBTB != 0)
	{
		char cBtbPredict = prediction_btb(address);
		if (cBtbPredict == 'n')
		{
			bBranchPredictionNeeded = false;

			if (cBtbPredict == direction)
			{
				// Correct prediction
			}
			else
			{
				// Misprediction
				m_iMispredctionFromBTB++;
			}
		}
	}

	if (bBranchPredictionNeeded)
	{
		m_iPredictionsFromBranch++;
		
		// Step 1: Obtain predictions from bimodal and gshare
		iBimodalIndex = m_pobjBimodal->getIndex(address);
		cBimodalPrediction = m_pobjBimodal->prediction(iBimodalIndex);

		iGshareIndex = m_pobjGshare->getIndex(address);
		cGsharePrediction = m_pobjGshare->prediction(iGshareIndex);

		// Step 2: Get index to chooser table
		iHybridIndex = getChooserTableIndex(address);

		// Step 3: Get overall prediction
		if (m_piChooserTable[iHybridIndex] >= 2)
		{
			cHybridPrediction = cGsharePrediction;
			iCurrentPredictor = 1; // 1 = Gshare , 0 = Bimodal
		}
		else
		{
			cHybridPrediction = cBimodalPrediction;
			iCurrentPredictor = 0; // 1 = Gshare , 0 = Bimodal
		}

		if (cHybridPrediction == direction)
		{
			// Correct prediction
		}
		else
		{
			// Misprediction
			m_iMispredctionFromBranch++;
		}

		// Step 4: Update selected branch predictor
		if (iCurrentPredictor == 1)
		{
			m_pobjGshare->update(address, direction);
		}
		else
		{
			m_pobjBimodal->update(address, direction);

			// Step 5: Update GHR
			m_pobjGshare->m_iGlobalHistoryRegister >>= 1; //Shift the register right by 1 bit position

			switch (direction)
			{
			case 't':
				m_pobjGshare->m_iGlobalHistoryRegister += (int)pow(2, m_pobjGshare->m_idxH - 1); // Place 1 in the MSB
				break;
			case 'n':
				// do nothing
				break;
			}
		}

		// Step 6: Update chooser table
		if ((cBimodalPrediction == direction) && (cGsharePrediction != direction))
		{
			decrementCounter(iHybridIndex);
		}
		else if ((cBimodalPrediction != direction) && (cGsharePrediction == direction))
		{
			incrementCounter(iHybridIndex);
		}
	}
}

void Hybrid::printBtbContents()
{
	if (m_pobjBimodal->m_idxBTB != 0 && m_pobjBimodal->m_assocBTB != 0)
	{
		cout << "Final BTB Tag Array Contents {valid, pc}: \n";
		for (int i = 0; i < pow(2, m_pobjBimodal->m_idxBTB); i++)
		{
			cout << "Set \t" << i << ": ";
			for (int j = 0; j < m_pobjBimodal->m_assocBTB; j++)
			{
				printf("  {%d, 0x %x}", m_pobjBTB->m_ppbValidityFlag[i][j], (unsigned int)m_pobjBTB->m_ppullTagArray[i][j]);
			}
			cout << endl;
		}
		cout << endl << endl;
	}
}

void Hybrid::printChooserTable()
{
	cout << "\nFinal Chooser Table Contents:\n";
	int iTableEntries = pow(2, m_iCTable);
	for (int i = 0; i < iTableEntries; i++)
	{
		cout << "Choice table[" << i << "]: " << m_piChooserTable[i] << " \n";
	}
}

void Hybrid::printStatistics()
{
	cout << "\nFinal Branch Predictor Statistics:\n";
	cout << "a. Number of branches: " << m_iNumberOfBranches << "\n";
	cout << "b. Number of predictions from the branch predictor: " << m_iPredictionsFromBranch << "\n";
	cout << "c. Number of mispredictions from the branch predictor: " << m_iMispredctionFromBranch << "\n";
	cout << "d. Number of mispredictions from the BTB: " << m_iMispredctionFromBTB << "\n";
	printf("e. Misprediction Rate: %2.2f percent\n", (m_iMispredctionFromBranch + m_iMispredctionFromBTB) * 100.0 / m_iNumberOfBranches);
}

void Hybrid::printSimOutput()
{
	printBtbContents();
	m_pobjBimodal->printTableContents();
	cout<<endl;
	m_pobjGshare->printTableContents();
	printf("\nFinal GHR Contents : 0x\t\t %x \n", m_pobjGshare->m_iGlobalHistoryRegister);
	printChooserTable();
	printStatistics();
}