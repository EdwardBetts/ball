// -*- Mode: C++; tab-width: 2; -*-
// vi: set ts=2:
//
#include <BALL/CONCEPT/classTest.h>
#include <BALLTestConfig.h>

///////////////////////////

#include <BALL/NMR/anisotropyShiftProcessor.h>
#include <BALL/FORMAT/HINFile.h>
#include <BALL/FORMAT/PDBFile.h>
#include <BALL/STRUCTURE/defaultProcessors.h>

///////////////////////////

START_TEST(AnisotropyShiftProcessor)

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

using namespace BALL;
using namespace std;

AnisotropyShiftProcessor* ap = 0;
CHECK(AnisotropyShiftProcessor::AnisotropyShiftProcessor() throw())
	ap = new AnisotropyShiftProcessor;
	TEST_NOT_EQUAL(ap, 0)
RESULT


CHECK(AnisotropyShiftProcessor::~AnisotropyShiftProcessor() throw())
  delete ap;
RESULT


CHECK(AnisotropyShiftProcessor::AnisotropyShiftProcessor(const AnisotropyShiftProcessor& processor) throw())
	AnisotropyShiftProcessor asp;
	AnisotropyShiftProcessor asp2(asp);
	// there's not much here to test...
RESULT


CHECK(AnisotropyShiftProcessor::init() throw())
  // tested below
RESULT


CHECK(AnisotropyShiftProcessor::start() throw())
  // tested below
RESULT


CHECK(AnisotropyShiftProcessor::Processor::Result operator () (Composite& composite) throw())
  // tested below
RESULT


CHECK(AnisotropyShiftProcessor::finish() throw())
  // tested below
RESULT

HINFile f(BALL_TEST_DATA_PATH(AnisotropyShiftProcessor_test.hin));
System S;
f >> S;

Parameters parameters(BALL_TEST_DATA_PATH(AnisotropyShiftProcessor_test.ini));

CHECK(chemical shifts/with rings)
	PRECISION(0.0001)
	StringHashMap<float> aniso_shifts;
	ifstream infile(BALL_TEST_DATA_PATH(AnisotropyShiftProcessor_test.dat));
	String name;
	float shift;
	while (infile.good())
	{
		infile >> name >> shift;
		if (name != "")
		{
			aniso_shifts.insert(name, shift);	
		}
	}
	TEST_EQUAL(aniso_shifts.size(), 15)

	AnisotropyShiftProcessor sp;
	sp.setParameters(parameters);
	sp.init();
	TEST_EQUAL(sp.isValid(), true)
	TEST_EQUAL(S.countAtoms(), 31)
	
	if (S.countAtoms() == 31)
	{
		S.apply(sp);

		AtomIterator atom_it = S.beginAtom();
		Position i = 0;
		for (; +atom_it; ++atom_it)
		{
			if (atom_it->hasProperty(AnisotropyShiftProcessor::PROPERTY__ANISOTROPY_SHIFT))
			{
				shift = atom_it->getProperty(AnisotropyShiftProcessor::PROPERTY__ANISOTROPY_SHIFT).getFloat();
				if (shift != 0)
				{
					STATUS("shift of " << atom_it->getFullName() << ": " << shift)
					TEST_EQUAL(aniso_shifts.has(atom_it->getFullName()), true)
					if (aniso_shifts.has(atom_it->getFullName()))
					{
						TEST_REAL_EQUAL(shift, aniso_shifts[atom_it->getFullName()])
						i++;
					}
					else
					{
						TEST_REAL_EQUAL(shift, 0.0)
					}
				}
			}
		}
		TEST_EQUAL(i, 15)
	}	
RESULT

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
