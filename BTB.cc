#include "BTB.h"

BTB::BTB(int iLevel, int iBlockSize, int iSize, int iAssoc, int iRepPolicy, int iIncPolicy)
{
	m_iLevel = iLevel;
	m_iBlockSize = iBlockSize;
	m_iSize = iSize;
	m_iAssoc = iAssoc;
	m_iRepPolicy = iRepPolicy;
	m_iInclusion = iIncPolicy;
}

void BTB::Init()
{
	if (m_iSets != 0 || m_iAssoc != 0)
	{
		setNumberOfSets();
		setIndexWidth();
		setOffestWidth();
		setTagWidth();

		m_ullWriteCounter = 0;
		m_ullReadCounter = 0;

		m_ullWriteBackCounter = 0;
		m_ullCleanEvictions = 0;

		m_ppullTagArray = new unsigned long long*[m_iSets];
		for (int i = 0; i < m_iSets; ++i)
			m_ppullTagArray[i] = new unsigned long long[m_iAssoc];

		m_ppullLRUCacheAccessCounter = new unsigned long long *[m_iSets];
		for (int i = 0; i < m_iSets; ++i)
			m_ppullLRUCacheAccessCounter[i] = new unsigned long long[m_iAssoc];

		m_ppbValidityFlag = new bool*[m_iSets];
		for (int i = 0; i < m_iSets; ++i)
			m_ppbValidityFlag[i] = new bool[m_iAssoc];

		m_ppbDirtyBit = new bool*[m_iSets];
		for (int i = 0; i < m_iSets; ++i)
			m_ppbDirtyBit[i] = new bool[m_iAssoc];

		m_ppullFIFOCounter = new unsigned long long *[m_iSets];
		for (int i = 0; i < m_iSets; ++i)
			m_ppullFIFOCounter[i] = new unsigned long long[m_iAssoc];

		m_pullWriteMissCounter = new unsigned long long[m_iSets];

		m_pullWriteHitCounter = new unsigned long long[m_iSets];

		m_pullReadMissCounter = new unsigned long long[m_iSets];

		m_pullReadHitCounter = new unsigned long long[m_iSets];

		for (int i = 0; i < m_iSets; ++i)
		{
			for (int j = 0; j < m_iAssoc; ++j)
			{
				m_ppullTagArray[i][j] = 0;
				m_ppullLRUCacheAccessCounter[i][j] = 0;
				m_ppbValidityFlag[i][j] = false;
				m_ppullFIFOCounter[i][j] = 0;
				m_ppbDirtyBit[i][j] = false;
			}

			m_pullWriteMissCounter[i] = 0;
			m_pullWriteHitCounter[i] = 0;
			m_pullReadMissCounter[i] = 0;
			m_pullReadHitCounter[i] = 0;
		}

		m_ppbPseudoLRU = new bool*[m_iSize];
		for (int i = 0; i < m_iSize; ++i)
		{
			m_ppbPseudoLRU[i] = new bool[m_iAssoc - 1];
			for (int j = 0; j < m_iAssoc - 1; ++j)
			{
				m_ppbPseudoLRU[i][j] = true;
			}
		}
	}
}

void BTB::setNumberOfSets()
{
	m_iSets = m_iSize / (m_iBlockSize*m_iAssoc);
}

void BTB::setIndexWidth()
{
	m_ullIndexWidth = log2(m_iSets);
}

void BTB::setOffestWidth()
{
	m_ullOffsetWidth = log2(m_iBlockSize);
}

void BTB::setTagWidth()
{
	m_ullTagWidth = 64 - m_ullIndexWidth - m_ullOffsetWidth; // TO_DO: Check if this is correct: 64 - We will always pad the address to 64-bits
}

unsigned long long BTB::calculateReadHits()
{
	// Sum of read hits
	unsigned long long ullTotalHits = 0;
	for (int i = 0; i < m_iSets; ++i)
	{
		ullTotalHits += m_pullReadHitCounter[i];
	}
	return ullTotalHits;
}

unsigned long long BTB::calculateReadMisses()
{
	// Sum of read misses
	unsigned long long ullTotalMissess = 0;
	for (int i = 0; i < m_iSets; ++i)
	{
		ullTotalMissess += m_pullReadMissCounter[i];
	}
	return ullTotalMissess;
}

unsigned long long BTB::calculateWriteHits()
{
	// Sum of write hits
	unsigned long long ullTotalHits = 0;
	for (int i = 0; i < m_iSets; ++i)
	{
		ullTotalHits += m_pullWriteHitCounter[i];
	}
	return ullTotalHits;
}

unsigned long long BTB::calculateWriteMisses()
{
	// Sum of write misses
	unsigned long long ullTotalMissess = 0;
	for (int i = 0; i < m_iSets; ++i)
	{
		ullTotalMissess += m_pullWriteMissCounter[i];
	}
	return ullTotalMissess;
}

float BTB::calculateMissRate()
{
	unsigned long long ullTotalReadMissess = calculateReadMisses();
	unsigned long long ullTotalWriteMisses = calculateWriteMisses();
	float fMissRate = (float)(ullTotalReadMissess + ullTotalWriteMisses) / (m_ullReadCounter + m_ullWriteCounter);

	return fMissRate;
}

unsigned long long BTB::FindCountOfMRU(unsigned long long ullIndex)
{
	unsigned long long ullMax = m_ppullLRUCacheAccessCounter[(int)ullIndex][0];

	// Find mru number
	for (int i = 1; i < m_iAssoc; i++)
	{

		if (ullMax < m_ppullLRUCacheAccessCounter[(int)ullIndex][i])
		{
			ullMax = m_ppullLRUCacheAccessCounter[(int)ullIndex][i];
		}
	}
	return ullMax;
}

unsigned long long BTB::FindMaxCountOfFIFO(unsigned long long ullIndex)
{
	unsigned long long ullMax = m_ppullFIFOCounter[(int)ullIndex][0];

	// Find mru number
	for (int i = 1; i < m_iAssoc; i++)
	{

		if (ullMax < m_ppullFIFOCounter[(int)ullIndex][i])
		{
			ullMax = m_ppullFIFOCounter[(int)ullIndex][i];
		}
	}
	return ullMax;
}

bool BTB::Read(unsigned long long ullIndex, unsigned long long ullTag)
{
	bool bCachePlaced = false;
	bool bReturn = false; // When bReturn is true it means read miss.
	// Increment Read Counter
	m_ullReadCounter++;

	for (int col = 0; col < m_iAssoc; ++col)
	{
		if (m_ppbValidityFlag[(int)ullIndex][col] == false)
		{
			// We found an invalid line. Use it!
			m_ppullTagArray[(int)ullIndex][col] = ullTag;

			// Reset its validity flag
			m_ppbValidityFlag[(int)ullIndex][col] = true;

			// This is a cold miss
			m_pullReadMissCounter[(int)ullIndex] ++;

			// Update placed flag 
			bCachePlaced = true;

			// We need to send this tag to next level also
			bReturn = true;

			// Update the access counter - lru
			switch (m_iRepPolicy)
			{
			case 0:
			{
				// Find the max count
				unsigned long long ullMax = FindCountOfMRU(ullIndex);

				m_ppullLRUCacheAccessCounter[(int)ullIndex][col] = ullMax + 1;
				break;
			}

			case 1:
			{
				// FIFO
				// Find the max count
				unsigned long long ullMax = FindMaxCountOfFIFO(ullIndex);

				m_ppullFIFOCounter[(int)ullIndex][col] = ullMax + 1;
				break;
			}

			case 2:
			{
				// psuedo LRU
				int iArrIndex = col + (m_iAssoc - 1);
				int iTempCol = col;
				bool bValue;
				while (iArrIndex != 0)
				{
					iArrIndex = (iArrIndex - 1) / 2;
					bValue = iTempCol % 2 == 0 ? true : false;
					m_ppbPseudoLRU[(int)ullIndex][iArrIndex] = bValue;
					iTempCol = iTempCol / 2;
				}
				break;
			}
			}
			break;
		}
		else
		{
			if (m_ppullTagArray[(int)ullIndex][col] == ullTag)
			{
				// Its a hit
				m_pullReadHitCounter[(int)ullIndex] ++;

				// We dont need to send this tag to next level also
				bReturn = false;

				// Update the access counter - lru
				switch (m_iRepPolicy)
				{
				case 0:
				{
					// Find the max count
					unsigned long long ullMax = FindCountOfMRU(ullIndex);

					m_ppullLRUCacheAccessCounter[(int)ullIndex][col] = ullMax + 1;
					break;
				}

				case 1:
				{
					// When we have a hit do not do anything for FIFO
					break;
				}

				case 2:
				{
					// Psuedo LRU - hit
					int iArrIndex = col + (m_iAssoc - 1);
					int iTempCol = col;
					bool bValue;
					while (iArrIndex != 0)
					{
						iArrIndex = (iArrIndex - 1) / 2;
						bValue = iTempCol % 2 == 0 ? true : false;
						m_ppbPseudoLRU[(int)ullIndex][iArrIndex] = bValue;
						iTempCol = iTempCol / 2;
					}
					break;
				}
				}

				// Update placed flag 
				bCachePlaced = true;
				break;
			}
		}
	}

	if (bCachePlaced == false)
	{
		// We need a replacement policy
		// replaceCache();

		// Its a miss
		m_pullReadMissCounter[(int)ullIndex] ++;

		// We need to send this tag to next level also
		bReturn = true;

		switch (m_iRepPolicy)
		{
		case 0:
		{
			// LRU implementation
			int col;

			// Find the minimum counter, max count, min counter column number
			unsigned long long ullMin = m_ppullLRUCacheAccessCounter[(int)ullIndex][0];
			unsigned long long ullMax = m_ppullLRUCacheAccessCounter[(int)ullIndex][0];

			int iMinCol = 0;
			for (col = 1; col < m_iAssoc; col++)
			{
				if (ullMin > m_ppullLRUCacheAccessCounter[(int)ullIndex][col])
				{
					ullMin = m_ppullLRUCacheAccessCounter[(int)ullIndex][col];
					iMinCol = col;
				}
				if (ullMax < m_ppullLRUCacheAccessCounter[(int)ullIndex][col])
				{
					ullMax = m_ppullLRUCacheAccessCounter[(int)ullIndex][col];
				}
			}

			m_ppullTagArray[(int)ullIndex][iMinCol] = ullTag;

			if (m_ppbDirtyBit[(int)ullIndex][iMinCol] == true)
			{
				// If the dirty bit is set, then unset it because data from L2 will be used to update L1
				m_ppbDirtyBit[(int)ullIndex][iMinCol] = false;

				m_ullWriteBackCounter++;
			}
			else
			{
				m_ullCleanEvictions++;
			}

			// Update Minimum Column's tag
			m_ppullLRUCacheAccessCounter[(int)ullIndex][iMinCol] = ullMax + 1;

			break;
		}

		case 1:
		{
			// FIFO
			// Find the minimum counter, max count, min counter column number
			unsigned long long ullMin = m_ppullFIFOCounter[(int)ullIndex][0];
			unsigned long long ullMax = m_ppullFIFOCounter[(int)ullIndex][0];

			int iMinCol = 0;
			for (int col = 1; col < m_iAssoc; col++)
			{
				if (ullMin > m_ppullFIFOCounter[(int)ullIndex][col])
				{
					ullMin = m_ppullFIFOCounter[(int)ullIndex][col];
					iMinCol = col;
				}
				if (ullMax < m_ppullFIFOCounter[(int)ullIndex][col])
				{
					ullMax = m_ppullFIFOCounter[(int)ullIndex][col];
				}
			}

			m_ppullTagArray[(int)ullIndex][iMinCol] = ullTag;

			if (m_ppbDirtyBit[(int)ullIndex][iMinCol] == true)
			{
				// If the dirty bit is set, then unset it because data from L2 will be used to update L1
				m_ppbDirtyBit[(int)ullIndex][iMinCol] = false;

				m_ullWriteBackCounter++;
			}
			else
			{
				m_ullCleanEvictions++;
			}

			// Update Minimum Column's tag
			m_ppullFIFOCounter[(int)ullIndex][iMinCol] = ullMax + 1;
			break;
		}

		case 2:
		{
			// Find the pseudoLRU
			int iTemp = 0;
			int iMinCol = 0;

			while (iTemp < m_iAssoc - 1)
			{
				iTemp = m_ppbPseudoLRU[(int)ullIndex][iTemp] == false ? 2 * iTemp + 1 : 2 * iTemp + 2;
			}

			iMinCol = iTemp - (m_iAssoc - 1);

			// Update the LRU again
			int iArrIndex = iMinCol + (m_iAssoc - 1);
			int iTempCol = iMinCol;
			bool bValue;
			while (iArrIndex != 0)
			{
				iArrIndex = (iArrIndex - 1) / 2;
				bValue = iTempCol % 2 == 0 ? true : false;
				m_ppbPseudoLRU[(int)ullIndex][iArrIndex] = bValue;
				iTempCol = iTempCol / 2;
			}

			if (m_ppbDirtyBit[(int)ullIndex][iMinCol] == true)
			{
				// If the dirty bit is set, then unset it because data from L2 will be used to update L1
				m_ppbDirtyBit[(int)ullIndex][iMinCol] = false;

				m_ullWriteBackCounter++;
			}
			else
			{
				m_ullCleanEvictions++;
			}

			// Update tag
			m_ppullTagArray[(int)ullIndex][iMinCol] = ullTag;
			break;
		}
		}
	}
	return bReturn;
}

bool BTB::Write(unsigned long long ullIndex, unsigned long long ullTag)
{
	bool bCachePlaced = false;
	bool bReturn = false;
	m_ullWriteCounter++;

	// Find an invalid line or line which contains the tag
	for (int col = 0; col < m_iAssoc; ++col)
	{
		if (m_ppbValidityFlag[(int)ullIndex][col] == false)
		{
			// We found an invalid line. Use it!
			m_ppullTagArray[(int)ullIndex][col] = ullTag;

			// Set its validity flag
			m_ppbValidityFlag[(int)ullIndex][col] = true;

			// Set its dirty bit
			m_ppbDirtyBit[(int)ullIndex][col] = true;

			// This is a cold miss
			m_pullWriteMissCounter[(int)ullIndex] ++;

			// Update placed flag 
			bCachePlaced = true;

			// We need to send this tag to next level also
			bReturn = true;

			// Update the access counter
			switch (m_iRepPolicy)
			{
			case 0:
			{
				// Find the max count
				unsigned long long ullMax = FindCountOfMRU(ullIndex);

				m_ppullLRUCacheAccessCounter[(int)ullIndex][col] = ullMax + 1;
				break;
			}

			case 1:
			{
				// FIFO
				// Find the max count
				unsigned long long ullMax = FindMaxCountOfFIFO(ullIndex);

				m_ppullFIFOCounter[(int)ullIndex][col] = ullMax + 1;
				break;
			}

			case 2:
			{
				// Psuedo LRU - hit
				int iArrIndex = col + (m_iAssoc - 1);
				int iTempCol = col;
				bool bValue;
				while (iArrIndex != 0)
				{
					iArrIndex = (iArrIndex - 1) / 2;
					bValue = iTempCol % 2 == 0 ? true : false;
					m_ppbPseudoLRU[(int)ullIndex][iArrIndex] = bValue;
					iTempCol = iTempCol / 2;
				}
				break;
			}
			} // end of switch case
			break;
		}
		else
		{
			if (m_ppullTagArray[(int)ullIndex][col] == ullTag)
			{
				// Its a hit
				m_pullWriteHitCounter[(int)ullIndex] ++;

				// Set its dirty bit
				m_ppbDirtyBit[(int)ullIndex][col] = true;

				// Update placed flag 
				bCachePlaced = true;

				// We dont need to send this tag to next level for NINE
				bReturn = false;

				// Update the access counter
				switch (m_iRepPolicy)
				{
				case 0:
				{
					// Find the max count
					unsigned long long ullMax = FindCountOfMRU(ullIndex);

					m_ppullLRUCacheAccessCounter[(int)ullIndex][col] = ullMax + 1;
					break;
				}

				case 1:
				{
					// FIFO - Do Nothing
					break;
				}

				case 2:
				{
					// Psuedo LRU - hit
					int iArrIndex = col + (m_iAssoc - 1);
					int iTempCol = col;
					bool bValue;
					while (iArrIndex != 0)
					{
						iArrIndex = (iArrIndex - 1) / 2;
						bValue = iTempCol % 2 == 0 ? true : false;
						m_ppbPseudoLRU[(int)ullIndex][iArrIndex] = bValue;
						iTempCol = iTempCol / 2;
					}
					break;
				}
				}
				break;
			}
		}
	}

	// No space in the set? Evict a line and make space
	if (bCachePlaced == false)
	{
		// We need a replacement policy
		// replaceCache();

		// Its a miss
		m_pullWriteMissCounter[(int)ullIndex] ++;

		// We need to send this tag to next level also
		bReturn = true;

		switch (m_iRepPolicy)
		{
		case 0:
		{
			// LRU implementation
			int col;

			// Find the minimum counter, max count, min counter column number
			unsigned long long ullMin = m_ppullLRUCacheAccessCounter[(int)ullIndex][0];
			unsigned long long ullMax = m_ppullLRUCacheAccessCounter[(int)ullIndex][0];

			int iMinCol = 0;
			for (col = 1; col < m_iAssoc; col++)
			{
				if (ullMin > m_ppullLRUCacheAccessCounter[(int)ullIndex][col])
				{
					ullMin = m_ppullLRUCacheAccessCounter[(int)ullIndex][col];
					iMinCol = col;
				}
				if (ullMax < m_ppullLRUCacheAccessCounter[(int)ullIndex][col])
				{
					ullMax = m_ppullLRUCacheAccessCounter[(int)ullIndex][col];
				}
			}

			m_ppullTagArray[(int)ullIndex][iMinCol] = ullTag;

			// Update Minimum Column's tag
			m_ppullLRUCacheAccessCounter[(int)ullIndex][iMinCol] = ullMax + 1;

			if (m_ppbDirtyBit[(int)ullIndex][iMinCol] == true)
			{
				m_ullWriteBackCounter++;
			}
			else
			{
				m_ullCleanEvictions++;
			}

			// Set its dirty bit
			m_ppbDirtyBit[(int)ullIndex][iMinCol] = true;

			break;
		}

		case 1:
		{
			// FIFO
			// Find the minimum counter, max count, min counter column number
			unsigned long long ullMin = m_ppullFIFOCounter[(int)ullIndex][0];
			unsigned long long ullMax = m_ppullFIFOCounter[(int)ullIndex][0];

			int iMinCol = 0;
			for (int col = 1; col < m_iAssoc; col++)
			{
				if (ullMin > m_ppullFIFOCounter[(int)ullIndex][col])
				{
					ullMin = m_ppullFIFOCounter[(int)ullIndex][col];
					iMinCol = col;
				}
				if (ullMax < m_ppullFIFOCounter[(int)ullIndex][col])
				{
					ullMax = m_ppullFIFOCounter[(int)ullIndex][col];
				}
			}

			m_ppullTagArray[(int)ullIndex][iMinCol] = ullTag;

			// Update Minimum Column's tag
			m_ppullFIFOCounter[(int)ullIndex][iMinCol] = ullMax + 1;

			if (m_ppbDirtyBit[(int)ullIndex][iMinCol] == true)
			{
				m_ullWriteBackCounter++;
			}
			else
			{
				m_ullCleanEvictions++;
			}

			// Set its dirty bit
			m_ppbDirtyBit[(int)ullIndex][iMinCol] = true;
			break;
		}

		case 2:
		{
			// Find the pseudoLRU
			int iTemp = 0;
			int iMinCol = 0;

			while (iTemp < m_iAssoc - 1)
			{
				iTemp = m_ppbPseudoLRU[(int)ullIndex][iTemp] == false ? 2 * iTemp + 1 : 2 * iTemp + 2;
			}

			iMinCol = iTemp - (m_iAssoc - 1);

			int iArrIndex = iMinCol + (m_iAssoc - 1);
			int iTempCol = iMinCol;
			bool bValue;
			while (iArrIndex != 0)
			{
				iArrIndex = (iArrIndex - 1) / 2;
				bValue = iTempCol % 2 == 0 ? true : false;
				m_ppbPseudoLRU[(int)ullIndex][iArrIndex] = bValue;
				iTempCol = iTempCol / 2;
			}

			// Update tag
			m_ppullTagArray[(int)ullIndex][iMinCol] = ullTag;

			if (m_ppbDirtyBit[(int)ullIndex][iMinCol] == true)
			{
				m_ullWriteBackCounter++;
			}
			else
			{
				m_ullCleanEvictions++;
			}

			// Set its dirty bit
			m_ppbDirtyBit[(int)ullIndex][iMinCol] = true;
			break;
		}
		}
	}
	return bReturn;
}