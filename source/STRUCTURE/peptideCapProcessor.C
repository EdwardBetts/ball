// -*- Mode: C++; tab-width: 2; -*-
// vi: set ts=2:
//

#include <BALL/STRUCTURE/peptideCapProcessor.h>
#include <BALL/KERNEL/system.h>
#include <BALL/KERNEL/protein.h>
#include <BALL/KERNEL/bond.h>
#include <BALL/STRUCTURE/fragmentDB.h>
#include <BALL/STRUCTURE/peptides.h>
#include <BALL/STRUCTURE/geometricTransformations.h>


namespace BALL
{
	PeptideCapProcessor::PeptideCapProcessor()
	{
	}

	float PeptideCapProcessor::computeDistance(std::vector<Atom*>& a, std::vector<Atom*>& b)
	{
		float d = 0.0;

		for (Position i = 0; i < a.size(); ++i)
		{
			for (Position j = 0; j < b.size(); ++j)
			{
				d += (a[i]->getPosition()-b[j]->getPosition()).getSquareLength();
			}
		}
		return d;
	}

	void PeptideCapProcessor::optimizeCapPosition(Protein* p, bool start)
	{
		Vector3 translation;
		Atom* axis   = NULL;
		Residue* cap = NULL;
		std::vector<Atom*> a;
		std::vector<Atom*> b;

		Size nr = p->countResidues();

		// cap at the beginning of a peptide
		if (start)
		{
			// put ACE-C to the center
			for (AtomIterator it = p->getResidue(1)->beginAtom(); +it; ++it)
			{
				if (it->getName() == "N")
				{
					translation = it->getPosition();
				}

				b.push_back(&*it);
			}

			cap = p->getResidue(0);
			for (AtomIterator it = cap->beginAtom(); +it; ++it)
			{
				a.push_back(&*it);
				if (it->getName() == "C")
				{
					axis = &*it;
				}
			}
		}
		//cap at the end of a peptide
		else
		{
			for (AtomIterator it = p->getResidue(nr-2)->beginAtom(); +it; ++it)
			{
				if (it->getName() == "C")
				{
					translation = it->getPosition();
				}

				b.push_back(&*it);
			}

			cap = p->getResidue(nr-1);
			for (AtomIterator it = cap->beginAtom(); +it; ++it)
			{
				a.push_back(&*it);
				if (it->getName() == "N")
				{
					axis = &*it;
				}
			}
		}

		//translate the anchor to origin
		TranslationProcessor tlp;
		tlp.setTranslation(translation*-1.0);
		p->apply(tlp);

		//try all torsions
		float largest_distance = 0.0;
		float tmp_distance = 0.0;
		float torsion      = 0.0;
		float step         = 2.0;

		TransformationProcessor tfp;
		Matrix4x4 m;
		m.setRotation( Angle(step, false), axis->getPosition());
		tfp.setTransformation(m);

		for (Position r = step; r <= 360; r+=step)
		{
			cap->apply(tfp);

			tmp_distance = computeDistance(a,b);

			if (largest_distance < tmp_distance)
			{
				largest_distance = tmp_distance;
				torsion = r;
			}
		}

		//apply best rotation angle
		m.setRotation( Angle(torsion, false), axis->getPosition());
		tfp.setTransformation(m);
		cap->apply(tfp);

		//now translate the protein back
		tlp.setTranslation(translation);
		p->apply(tlp);

	}


	Processor::Result PeptideCapProcessor::operator() (Composite &composite)
	{
		// if it is a protein --> better Chain???
		if (RTTI::isKindOf<Protein>(composite))
		{
			TranslationProcessor tlp;
			TransformationProcessor tfp;
			Matrix4x4 m;

			FragmentDB fragment_db("");
			ReconstructFragmentProcessor reconstruct(fragment_db);

			Residue* ace_residue = NULL;
			Residue* nme_residue = NULL;

			Protein* p = RTTI::castTo<Protein> (composite);

			// check if a ACE cap was already added
			bool add_cap = (p->getResidue(0)->getName() != "ACE");

			if (add_cap)
			{
				// create ACE-Cap
				ace_residue = new Residue("ACE");
				ace_residue->setProperty(Residue::PROPERTY__AMINO_ACID);
				ace_residue->apply(reconstruct);
				ace_residue->apply(fragment_db.normalize_names);
				ace_residue->apply(fragment_db.build_bonds);

				AtomIterator ace_atom = ace_residue->beginAtom();

				//get all atoms needed to compute the transformation of the cap
				Atom* cAtom   = NULL;
				Atom* ch3Atom = NULL;
				Atom* oAtom   = NULL;
				Atom* h1Atom  = NULL;
				Vector3 h2Atom(0.0,0.0,0.0);
				Vector3 h3Atom(0.0,0.0,0.0);
				Vector3 nAtom(0.0,0.0,0.0);

				while(+ace_atom)
				{
					if (ace_atom->getName() == "C")
						cAtom = &*ace_atom;
					else if (ace_atom->getName() == "CH3")
						ch3Atom = &*ace_atom;
					else if (ace_atom->getName() == "O")
						oAtom = &*ace_atom;
					++ace_atom;
				}

				std::vector<Atom*> to_remove;

				AtomIterator n_atom = p->getResidue(0)->beginAtom();

				while (+n_atom)
				{
					if (n_atom->getName() == "1H")
					{
						h1Atom = &*n_atom;
						n_atom->setName("H");

						if(p->getResidue(0)->getName() == "PRO")
							to_remove.push_back(&*n_atom);
					}
					else if (n_atom->getName() == "N")
					{
						nAtom = n_atom->getPosition();
						n_atom->apply(fragment_db.add_hydrogens);
					}
					++n_atom;
				}

				//compute position of OXT... map N of the last residue to it later
				tlp.setTranslation(ch3Atom->getPosition()*-1.0);
				ace_residue->apply(tlp);

				m.setRotation( Angle(180.0 * Constants::PI / 180.0), cAtom->getPosition());

				// map oxt and N of the last residue to the origin
				tlp.setTranslation(m*oAtom->getPosition()*-1.0);
				ace_residue->apply(tlp);

				tlp.setTranslation(nAtom*-1.0);
				p->apply(tlp);

				n_atom = p->getResidue(0)->beginAtom();
				while (+n_atom)
				{
					if (n_atom->getName() == "2H")
					{
						h2Atom = n_atom->getPosition();
						to_remove.push_back(&*n_atom);
					}
					else if (n_atom->getName() == "3H")
					{
						h3Atom = n_atom->getPosition();
						to_remove.push_back(&*n_atom);
					}
					++n_atom;
				}

				//rotate CH3 atom to 2H position
				Angle   angle    = cAtom->getPosition().getAngle(h2Atom+h3Atom);
				Vector3 rot_axis = (cAtom->getPosition()%(h2Atom+h3Atom)).normalize();

				m.setRotation(angle,rot_axis);
				tfp.setTransformation(m);
				ace_residue->apply(tfp);

				//insert ACE-Cap into chain
				p->getChain(0)->insertBefore(*ace_residue,*p->getResidue(0));

				//Add Bond between ACE-Cap and Helix
				n_atom = p->getResidue(1)->beginAtom();
				while (+n_atom)
				{
					if (n_atom->getName() == "N")
					{
						Bond* new_bond = cAtom->createBond(*n_atom);
						new_bond->setOrder(Bond::ORDER__SINGLE);
						break;
					}
					++n_atom;
				}

				//back translation
				tlp.setTranslation(nAtom);
				p->apply(tlp);

				//remove old hydrogens
				for (Position a = 0; a < to_remove.size(); ++a)
				{
					delete to_remove[a];
				}

				//torsional optimzation of ACE
				optimizeCapPosition(p,true);
			}

			//##################################################################

			add_cap = (p->getResidue(p->countResidues()-1)->getName() != "NME");

			if (add_cap)
			{
				//create NME-Cap
				nme_residue = new Residue("NME");
				nme_residue->setProperty(Residue::PROPERTY__AMINO_ACID);
				nme_residue->apply(reconstruct);
				nme_residue->apply(fragment_db.normalize_names);
				nme_residue->apply(fragment_db.build_bonds);

				Residue* last_residue = p->getResidue(p->countResidues()-1);

				//####################################################

				Atom* nAtom = NULL;
				Atom* ch3Atom = NULL;
				Atom* hAtom = NULL;

				AtomIterator nme_atom = nme_residue->beginAtom();
				while (+nme_atom)
				{
					if (nme_atom->getName() == "N")
						nAtom = &*nme_atom;
					else if (nme_atom->getName() == "CH3")
						ch3Atom = &*nme_atom;
					else if (nme_atom->getName() == "H")
						hAtom = &*nme_atom;
					++nme_atom;
				}

				Vector3 anchor((  (hAtom->getPosition()-nAtom->getPosition()).normalize() 
				                + (ch3Atom->getPosition()-nAtom->getPosition()).normalize()).normalize()*-1.335);
				tlp.setTranslation((anchor + nAtom->getPosition())*-1.0);
				nme_residue->apply(tlp);

				Atom* oxt = NULL;
				Atom* cAtom = NULL;

				AtomIterator c_atom = last_residue->beginAtom();

				while (+c_atom)
				{
					if (c_atom->getName() == "OXT")
						oxt = &*c_atom;
					else if (c_atom->getName() == "C")
					{
						anchor = c_atom->getPosition();
						cAtom  = &*c_atom;
					}
					++c_atom;
				}

				tlp.setTranslation(anchor*-1.0);
				p->apply(tlp);

				//transform cap to the old OXT position
				Angle angle = nAtom->getPosition().getAngle(oxt->getPosition());
				Vector3 rot_axis = (nAtom->getPosition()%(oxt->getPosition())).normalize();
				m.setRotation(angle,rot_axis);
				tfp.setTransformation(m);
				nme_residue->apply(tfp);

				//insert NME-Cap into chain
				p->getChain(0)->insertAfter(*nme_residue, *last_residue);

				//Add Bond between NME-Cap and Helix
				Bond* new_bond = cAtom->createBond(*nAtom);
				new_bond->setOrder(Bond::ORDER__SINGLE);

				tlp.setTranslation(anchor);
				p->apply(tlp);

				delete oxt;

				//torsional optimzation of NME
				optimizeCapPosition(p,false);
			}
		}

		return Processor::CONTINUE;
	}

} // namespace BALL