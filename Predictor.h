// To prevent redefinition
#ifndef PREDICTOR_H
#define PREDICTOR_H

// Predictor will be the base class which will provide an uniform interface from main
class Predictor
{
public:
	int m_iNumberOfBranches;

	virtual void update(int address, char direction);
	virtual void printSimOutput();
};

#endif