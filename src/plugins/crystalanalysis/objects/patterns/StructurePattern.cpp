///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "StructurePattern.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(StructurePattern);
DEFINE_PROPERTY_FIELD(StructurePattern, shortName);
DEFINE_PROPERTY_FIELD(StructurePattern, structureType);
DEFINE_PROPERTY_FIELD(StructurePattern, symmetryType);
DEFINE_REFERENCE_FIELD(StructurePattern, burgersVectorFamilies);
SET_PROPERTY_FIELD_LABEL(StructurePattern, shortName, "Short name");
SET_PROPERTY_FIELD_LABEL(StructurePattern, structureType, "Structure type");
SET_PROPERTY_FIELD_LABEL(StructurePattern, symmetryType, "Symmetry type");
SET_PROPERTY_FIELD_LABEL(StructurePattern, burgersVectorFamilies, "Burgers vector families");

/******************************************************************************
* Constructs the StructurePattern object.
******************************************************************************/
StructurePattern::StructurePattern(DataSet* dataset) : ParticleType(dataset),
		_structureType(OtherStructure), _symmetryType(OtherSymmetry)
{
	// Create "unknown" Burgers vector family.
	BurgersVectorFamily* family = new BurgersVectorFamily(dataset);
	family->setColor(Color(0.9f, 0.2f, 0.2f));
	family->setNumericId(0);
	family->setName(tr("Other"));
	family->setBurgersVector(Vector3::Zero());
	addBurgersVectorFamily(family);
}

/******************************************************************************
* Returns the display color to be used for a given Burgers vector.
******************************************************************************/
Color StructurePattern::getBurgersVectorColor(const QString& latticeName, const Vector3& b)
{
	if(latticeName == "bcc") {
		static const Color predefinedLineColors[] = {
				Color(0.4f,1.0f,0.4f),
				Color(1.0f,0.2f,0.2f),
				Color(0.4f,0.4f,1.0f),
				Color(0.9f,0.5f,0.0f),
				Color(1.0f,1.0f,0.0f),
				Color(1.0f,0.4f,1.0f),
				Color(0.7f,0.0f,1.0f)
		};		
		static const Vector3 burgersVectors[] = {
				{ FloatType(0.5), FloatType(0.5), FloatType(0.5) },
				{ FloatType(-0.5), FloatType(0.5), FloatType(0.5) },
				{ FloatType(0.5), FloatType(-0.5), FloatType(0.5) },
				{ FloatType(0.5), FloatType(0.5), FloatType(-0.5) },
				{ FloatType(1.0), FloatType(0.0), FloatType(0.0) },
				{ FloatType(0.0), FloatType(1.0), FloatType(0.0) },
				{ FloatType(0.0), FloatType(0.0), FloatType(1.0) }
		};
		OVITO_STATIC_ASSERT(sizeof(burgersVectors)/sizeof(burgersVectors[0]) == sizeof(predefinedLineColors)/sizeof(predefinedLineColors[0]));
		for(size_t i = 0; i < sizeof(burgersVectors)/sizeof(burgersVectors[0]); i++) {
			if(b.equals(burgersVectors[i]) || b.equals(-burgersVectors[i]))
				return predefinedLineColors[i];
		}
	}
	else if(latticeName == "fcc") {
		static const Color predefinedLineColors[] = {
				Color(0.4f,1.0f,0.4f),
				Color(1.0f,0.2f,0.2f),
				Color(0.4f,0.4f,1.0f),
				Color(0.9f,0.5f,0.0f),
				Color(1.0f,1.0f,0.0f),
				Color(1.0f,0.4f,1.0f),
				Color(0.7f,0.0f,1.0f),
				Color(0.2f,1.0f,1.0f),
				Color(0.2f,1.0f,0.2f),
				Color(0.2f,0.0f,1.0f),
				Color(0.0f,0.8f,0.2f),
				Color(0.2f,0.0f,0.8f),

				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
		};
		static const Vector3 burgersVectors[] = {
				{ FloatType(1.0/6.0), FloatType(-2.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-2.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), FloatType(2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), FloatType(-2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), FloatType(2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), FloatType(-2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(2.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(2.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(-1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(-1.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(1.0/6.0), FloatType(-1.0/6.0) },				
				{ FloatType(2.0/6.0), FloatType(1.0/6.0), FloatType(1.0/6.0) },

				{ 0, FloatType(1.0/6.0), FloatType(1.0/6.0) },
				{ 0, FloatType(1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), 0, FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), 0, FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), 0 },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), 0 },
		};
		OVITO_STATIC_ASSERT(sizeof(burgersVectors)/sizeof(burgersVectors[0]) == sizeof(predefinedLineColors)/sizeof(predefinedLineColors[0]));
		for(size_t i = 0; i < sizeof(burgersVectors)/sizeof(burgersVectors[0]); i++) {
			if(b.equals(burgersVectors[i]) || b.equals(-burgersVectors[i]))
				return predefinedLineColors[i];
		}
	}
	return Color(0.9f, 0.9f, 0.9f);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
