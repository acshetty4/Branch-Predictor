#include <iostream>
using namespace std;

#include <string>
#include <math.h> 
#include <stdlib.h>
#include <stdio.h>

#include "Bimodal.h"
#include "Predictor.h"


Bimodal::Bimodal(int iB, int iBTB, int iAssocBTB)
{
	int iTableEntries = 0;

	m_idxTable = iB;
	m_idxBTB = iBTB;
	m_assocBTB = iAssocBTB;

	iTableEntries = pow(2, m_idxTable);

	m_piPredictorTable = new int[iTableEntries];

	// initialise to 2 - weakly taken
	for (int i = 0; i < iTableEntries; i++)
	{
		m_piPredictorTable[i] = 2;
	}
	
	if (iBTB != 0 && iAssocBTB != 0)
	{
		int iBlockSize = 64; // TO_DO : Check if this is correct
		int iSize = iBlockSize * iAssocBTB * pow(2, iBTB);

		m_pobjBTB = new BTB(1, iBlockSize, iSize, iAssocBTB, 0, 0);
		m_pobjBTB->Init();
	}

	m_bHybridObject = false;

	m_iPredictionsFromBranch = m_iMispredctionFromBranch = m_iMispredctionFromBTB = 0;

	for (int i = 0; i < m_idxTable; i++)
	{
		mask <<= 1;
		mask |= 1;
	}

	mask <<= 2;

	for (int i = 0; i < m_idxBTB; i++)
	{
		mask_btb <<= 1;
		mask_btb |= 1;
	}

	mask_btb <<= 2;
}

int Bimodal::getIndex(int address)
{
	int index = address & mask;
	index >>= 2;
	return index;
}

char Bimodal::prediction(int index)
{
	char cPredict;
	if (m_piPredictorTable[index] >= 2)
	{
		cPredict = 't';
	}
	else
	{
		cPredict = 'n';
	}
	return cPredict;
}

char Bimodal::prediction_btb(int address)
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

void Bimodal::update(int address, char direction)
{
	// Step 0: Check BTB
	bool bBranchPredictionNeeded = true;
	if (m_assocBTB != 0 && m_idxBTB != 0
		&& m_bHybridObject == false)
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

	// Step 1: Find Index
	int index = getIndex(address);

	// Step 2: Make a prediction
	if (bBranchPredictionNeeded)
	{
		char cPredict = prediction(index);
		m_iPredictionsFromBranch++;
		if (cPredict == direction)
		{
			// Correct prediction
		}
		else
		{
			// Misprediction
			m_iMispredctionFromBranch++;
		}


		// Step 3: Update counter
		switch (direction)
		{
		case 't':
			incrementCounter(index);
			break;
		case 'n':
			decrementCounter(index);
			break;
		}
	}
}

void Bimodal::incrementCounter(int index)
{
	if (m_piPredictorTable[index] < 3)
	{
		m_piPredictorTable[index] ++;
	}
}

void Bimodal::decrementCounter(int index)
{
	if (m_piPredictorTable[index] > 0)
	{
		m_piPredictorTable[index] --;
	}
}

void Bimodal::printSimOutput()
{
	printBtbContents(); 
	printTableContents();
	printStatistics();
}

void Bimodal::printTableContents()
{
	cout << "Final Bimodal Table Contents: \n";
	int iTableEntries = pow(2, m_idxTable);
	for (int i = 0; i < iTableEntries; i++)
	{
		cout << "table[" << i << "]: " << m_piPredictorTable[i] << " \n";
	}
}

void Bimodal::printBtbContents()
{
	if (m_idxBTB != 0 && m_assocBTB != 0)
	{
		cout << "Final BTB Tag Array Contents {valid, pc}:\n";
		for (int i = 0; i < pow(2, m_idxBTB); i++)
		{
			cout << "Set \t" << i << ": ";
			for (int j = 0; j < m_assocBTB; j++)
			{
				printf("  {%d, 0x %x}", m_pobjBTB->m_ppbValidityFlag[i][j], (unsigned int)m_pobjBTB->m_ppullTagArray[i][j]);
			}
			cout << endl;
		}
		cout << endl << endl;
	}
}

void Bimodal::printStatistics()
{
	cout << "\nFinal Branch Predictor Statistics:\n";
	cout << "a. Number of branches: " << m_iNumberOfBranches << "\n";
	cout << "b. Number of predictions from the branch predictor: " << m_iPredictionsFromBranch << " \n";
	cout << "c. Number of mispredictions from the branch predictor: " << m_iMispredctionFromBranch << "\n";
	cout << "d. Number of mispredictions from the BTB: " << m_iMispredctionFromBTB << " \n";
	cout.precision(2);
	cout << "e. Misprediction Rate: " << fixed << (m_iMispredctionFromBranch + m_iMispredctionFromBTB) * 100.0 / m_iNumberOfBranches << " percent\n";
}