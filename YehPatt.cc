#include <iostream>
using namespace std;

#include <string>
#include <math.h> 
#include <stdlib.h>
#include <stdio.h>

#include "YehPatt.h"
#include "Predictor.h"

YehPatt::YehPatt(int h, int p, int iBTB, int iAssocBTB)
{
	int iHistTableEntries = 0, iPredictionTableEntries = 0;

	m_idxBTB = iBTB;
	m_idxH = h;
	m_idxP = p;
	m_assocBTB = iAssocBTB;

	iHistTableEntries = pow(2, m_idxH);
	iPredictionTableEntries = pow(2, m_idxP);

	m_piHistoryTable = new int[iHistTableEntries];
	m_piPredictionTable = new int[iPredictionTableEntries];

	// initialise history table to 0
	for (int i = 0; i < iHistTableEntries; i++)
	{
		m_piHistoryTable[i] = 0;
	}

	// initialise prediction table to 2
	for (int i = 0; i < iPredictionTableEntries; i++)
	{
		m_piPredictionTable[i] = 2;
	}
	
	if (iBTB != 0 && iAssocBTB != 0)
	{
		int iBlockSize = 64; // TO_DO : Check if this is correct
		int iSize = iBlockSize * iAssocBTB * pow(2, iBTB);

		m_pobjBTB = new BTB(1, iBlockSize, iSize, iAssocBTB, 0, 0);
		m_pobjBTB->Init();
	}

	m_iNumberOfBranches = m_iPredictionsFromBranch = m_iMispredctionFromBranch = m_iMispredctionFromBTB = 0;

	mask = mask_p = mask_btb = 0;

	// Create mask for history table
	for (int i = 0; i < m_idxH; i++)
	{
		mask <<= 1;
		mask |= 1;
	}

	mask <<= 2;

	for (int i = 0; i < m_idxP; i++)
	{
		mask_p <<= 1;
		mask_p |= 1;
	}

	for (int i = 0; i < m_idxBTB; i++)
	{
		mask_btb <<= 1;
		mask_btb |= 1;
	}

	mask_btb <<= 2;
}

int YehPatt::getHIndex(int address)
{
	int index = address & mask;
	index >>= 2;
	return index;
}

int YehPatt::getPIndex(int history)
{
	int index = history & mask_p;
	return index;
}

char YehPatt::prediction(int index)
{
	char cPredict;
	if (m_piPredictionTable[index] >= 2)
	{
		cPredict = 't';
	}
	else
	{
		cPredict = 'n';
	}
	return cPredict;
}

char YehPatt::prediction_btb(int address)
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

void YehPatt::update(int address, char direction)
{
	// Step 0: Check BTB
	bool bBranchPredictionNeeded = true;
	if (m_assocBTB != 0 && m_idxBTB != 0)
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

		// Step 1: Find index to history table
		int h = getHIndex(address);

		// Step 2: Find index to prediction table
		int p = getPIndex(m_piHistoryTable[h]);

		// Step 3: Prediction
		char cPredict = prediction(p);

		if (cPredict != direction)
		{
			// Misprediction
			m_iMispredctionFromBranch++;
		}

		// Step 4: Update counter and history table
		switch (direction)
		{
		case 't':
			incrementCounter(p);
			break;
		case 'n':
			decrementCounter(p);
			break;
		}

		m_piHistoryTable[h] >>= 1;

		switch (direction)
		{
		case 't':
			m_piHistoryTable[h] += (int)pow(2, m_idxP - 1); // Place 1 in the MSB
			break;
		case 'n':
			// do nothing
			break;
		}
	}
}

void YehPatt::incrementCounter(int index)
{
	if (m_piPredictionTable[index] < 3)
	{
		m_piPredictionTable[index] ++;
	}
}

void YehPatt::decrementCounter(int index)
{
	if (m_piPredictionTable[index] > 0)
	{
		m_piPredictionTable[index] --;
	}
}

void YehPatt::printBtbContents()
{
	if (m_idxBTB != 0 && m_assocBTB != 0)
	{
		cout << "Final BTB Tag Array Contents {valid, pc}: \n";
		for (int i = 0; i < pow(2, m_idxBTB); i++)
		{
			cout << "Set \t" << i << ": ";
			for (int j = 0; j < m_assocBTB; j++)
			{
				printf("  {%d, 0x %x}", m_pobjBTB->m_ppbValidityFlag[i][j], (unsigned int)m_pobjBTB->m_ppullTagArray[i][j]);
			}
			cout << endl;
		}
		cout<<endl<<endl;
	}
}

void YehPatt::printPredictionTableContents()
{
	cout << "\nFinal Prediction Table Contents: \n";
	int iTableEntries = pow(2, m_idxP);
	for (int i = 0; i < iTableEntries; i++)
	{
		cout << "table[" << i << "]: " << m_piPredictionTable[i] << " \n";
	}
}

void YehPatt::printHistoryTableContents()
{
	cout << "Final History Table Contents:\n";
	int iTableEntries = pow(2, m_idxH);
	for (int i = 0; i < iTableEntries; i++)
	{
		printf("table[%d]: 0x %x\n", i, m_piHistoryTable[i]);
	}
}

void YehPatt::printStatistics()
{
	cout << "\nFinal Branch Predictor Statistics:\n";
	cout << "a. Number of branches: " << m_iNumberOfBranches << "\n";
	cout << "b. Number of predictions from the branch predictor: " << m_iPredictionsFromBranch << "\n";
	cout << "c. Number of mispredictions from the branch predictor: " << m_iMispredctionFromBranch << "\n";
	cout << "d. Number of mispredictions from the BTB: " << m_iMispredctionFromBTB << "\n";
	cout.precision(2);
	cout << "e. Misprediction Rate: " << fixed << (m_iMispredctionFromBranch + m_iMispredctionFromBTB) * 100.0 / m_iNumberOfBranches << " percent\n";
}

void YehPatt::printSimOutput()
{
	printBtbContents();
	printHistoryTableContents();
	printPredictionTableContents();
	printStatistics();
}