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

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/VectorVis.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include "ParticlesColorCodingModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesColorCodingModifierDelegate);
IMPLEMENT_OVITO_CLASS(ParticleVectorsColorCodingModifierDelegate);
IMPLEMENT_OVITO_CLASS(BondsColorCodingModifierDelegate);

/******************************************************************************
* Creates the property object that will receive the computed colors.
******************************************************************************/
PropertyObject* ParticlesColorCodingModifierDelegate::createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors)
{
    PropertyObject* colorProperty = oh.outputStandardProperty<ParticleProperty>(ParticleProperty::ColorProperty, false);
    if(initializeWithExistingColors) {
        ParticleInputHelper pih(dataset(), ih.input());
        const std::vector<Color> colors = pih.inputParticleColors(time, oh.output().mutableStateValidity());
        OVITO_ASSERT(colors.size() == colorProperty->size());
        std::copy(colors.cbegin(), colors.cend(), colorProperty->dataColor());
    }
    return colorProperty;
}

/******************************************************************************
* Returns whether this function can be applied to the given input data.
******************************************************************************/
bool ParticleVectorsColorCodingModifierDelegate::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	for(DataObject* obj : input.objects()) {
        if(dynamic_object_cast<VectorVis>(obj->visElement()))
            return true;
    }
    return false;
}
    
/******************************************************************************
* Creates the property object that will receive the computed colors.
******************************************************************************/
PropertyObject* ParticleVectorsColorCodingModifierDelegate::createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors)
{
    PropertyObject* colorProperty = oh.outputStandardProperty<ParticleProperty>(ParticleProperty::VectorColorProperty, false);
    if(initializeWithExistingColors) {
        if(VectorVis* vectorVis = dynamic_object_cast<VectorVis>(colorProperty->visElement())) {
            std::fill(colorProperty->dataColor(), colorProperty->dataColor() + colorProperty->size(), vectorVis->arrowColor());
        }
    }
    return colorProperty;
}

/******************************************************************************
* Creates the property object that will receive the computed colors.
******************************************************************************/
PropertyObject* BondsColorCodingModifierDelegate::createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors)
{
    PropertyObject* colorProperty = oh.outputStandardProperty<BondProperty>(BondProperty::ColorProperty, false);
    if(initializeWithExistingColors) {
        ParticleInputHelper pih(dataset(), ih.input());
        const std::vector<Color> colors = pih.inputBondColors(time, oh.output().mutableStateValidity());
        OVITO_ASSERT(colors.size() == colorProperty->size());
        std::copy(colors.cbegin(), colors.cend(), colorProperty->dataColor());
    }
    return colorProperty;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
