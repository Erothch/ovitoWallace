///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/wallace/modifier/atomic_strain_mod_burgers/AtomicStrainModBurgers.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanRadioButtonParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include <gui/properties/Vector3ParameterUI.h>
#include <core/dataset/io/FileSource.h>
#include "AtomicStrainModBurgersEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(AtomicStrainModBurgersEditor);
SET_OVITO_OBJECT_EDITOR(AtomicStrainModBurgers, AtomicStrainModBurgersEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AtomicStrainModBurgersEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Atomic strain"), rolloutParams, "particles.modifiers.atomic_strain.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Cutoff parameter.
    FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);

	layout->addLayout(gridlayout);

    // BurgersContent

    //Box I am adding to include burgers content
    QGroupBox* burgersContentGroupBox = new QGroupBox(tr("Burgers Content"));
    layout->addWidget(burgersContentGroupBox);

    //layout for burgerscontent
    gridlayout = new QGridLayout(burgersContentGroupBox);
    gridlayout->setContentsMargins(4,4,4,4);
    gridlayout->setSpacing(4);

    // need an updateParameterValue() and a connect to onSpinnerDrag, etc.
    for(int col = 0; col < 3; col++) {
        int row = 0;

        Vector3ParameterUI* burgersUI = new Vector3ParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::burgersContent), col);
        gridlayout->addLayout(burgersUI->createFieldLayout(), row+1, col*3+0);
    }


	QGroupBox* mappingGroupBox = new QGroupBox(tr("Affine mapping of simulation cell"));
	layout->addWidget(mappingGroupBox);

	QGridLayout* sublayout = new QGridLayout(mappingGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);

	IntegerRadioButtonParameterUI* affineMappingUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::affineMapping));
    sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::NO_MAPPING, tr("Off")), 0, 0);
	sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::TO_REFERENCE_CELL, tr("To reference")), 0, 1);
    sublayout->addWidget(affineMappingUI->addRadioButton(ReferenceConfigurationModifier::TO_CURRENT_CELL, tr("To current")), 1, 1);

	BooleanParameterUI* useMinimumImageConventionUI = new BooleanParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::useMinimumImageConvention));
	layout->addWidget(useMinimumImageConventionUI->checkBox());

#if 0
	QCheckBox* calculateShearStrainsBox = new QCheckBox(tr("Output von Mises shear strains"));
	calculateShearStrainsBox->setEnabled(false);
	calculateShearStrainsBox->setChecked(true);
	layout->addWidget(calculateShearStrainsBox);

	QCheckBox* calculateVolumetricStrainsBox = new QCheckBox(tr("Output volumetric strains"));
	calculateVolumetricStrainsBox->setEnabled(false);
	calculateVolumetricStrainsBox->setChecked(true);
	layout->addWidget(calculateVolumetricStrainsBox);
#endif	

    BooleanParameterUI* calculateDeformationGradientsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::calculateDeformationGradients));
	layout->addWidget(calculateDeformationGradientsUI->checkBox());

    BooleanParameterUI* calculateStrainTensorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::calculateStrainTensors));
	layout->addWidget(calculateStrainTensorsUI->checkBox());

    BooleanParameterUI* calculateNonaffineSquaredDisplacementsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::calculateNonaffineSquaredDisplacements));
	layout->addWidget(calculateNonaffineSquaredDisplacementsUI->checkBox());

    BooleanParameterUI* calculateRotationsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::calculateRotations));
	layout->addWidget(calculateRotationsUI->checkBox());

    BooleanParameterUI* calculateStretchTensorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::calculateStretchTensors));
	layout->addWidget(calculateStretchTensorsUI->checkBox());

    BooleanParameterUI* selectInvalidParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::selectInvalidParticles));
	layout->addWidget(selectInvalidParticlesUI->checkBox());

	QGroupBox* referenceFrameGroupBox = new QGroupBox(tr("Reference frame"));
	layout->addWidget(referenceFrameGroupBox);

	sublayout = new QGridLayout(referenceFrameGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(0, 5);
	sublayout->setColumnStretch(2, 95);

	// Add box for selection between absolute and relative reference frames.
	BooleanRadioButtonParameterUI* useFrameOffsetUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::useReferenceFrameOffset));
	useFrameOffsetUI->buttonFalse()->setText(tr("Constant reference configuration"));
	sublayout->addWidget(useFrameOffsetUI->buttonFalse(), 0, 0, 1, 3);

	IntegerParameterUI* frameNumberUI = new IntegerParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::referenceFrameNumber));
	frameNumberUI->label()->setText(tr("Frame number:"));
	sublayout->addWidget(frameNumberUI->label(), 1, 1, 1, 1);
	sublayout->addLayout(frameNumberUI->createFieldLayout(), 1, 2, 1, 1);
	frameNumberUI->setEnabled(false);
	connect(useFrameOffsetUI->buttonFalse(), &QRadioButton::toggled, frameNumberUI, &IntegerParameterUI::setEnabled);

	useFrameOffsetUI->buttonTrue()->setText(tr("Relative to current frame"));
	sublayout->addWidget(useFrameOffsetUI->buttonTrue(), 2, 0, 1, 3);
	IntegerParameterUI* frameOffsetUI = new IntegerParameterUI(this, PROPERTY_FIELD(ReferenceConfigurationModifier::referenceFrameOffset));
	frameOffsetUI->label()->setText(tr("Frame offset:"));
	sublayout->addWidget(frameOffsetUI->label(), 3, 1, 1, 1);
	sublayout->addLayout(frameOffsetUI->createFieldLayout(), 3, 2, 1, 1);
	frameOffsetUI->setEnabled(false);
	connect(useFrameOffsetUI->buttonTrue(), &QRadioButton::toggled, frameOffsetUI, &IntegerParameterUI::setEnabled);

	QGroupBox* referenceSourceGroupBox = new QGroupBox(tr("Reference configuration source"));
	layout->addWidget(referenceSourceGroupBox);

	sublayout = new QGridLayout(referenceSourceGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);

	_sourceButtonGroup = new QButtonGroup(this);
    connect(_sourceButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &AtomicStrainModBurgersEditor::onSourceButtonClicked);
	QRadioButton* upstreamPipelineBtn = new QRadioButton(tr("Upstream pipeline"));
	QRadioButton* externalFileBtn = new QRadioButton(tr("External file"));
	_sourceButtonGroup->addButton(upstreamPipelineBtn, 0);
	_sourceButtonGroup->addButton(externalFileBtn, 1);
	sublayout->addWidget(upstreamPipelineBtn, 0, 0);
	sublayout->addWidget(externalFileBtn, 1, 0);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	// Open a sub-editor for the reference object.
    new SubObjectParameterUI(this, PROPERTY_FIELD(AtomicStrainModBurgers::referenceConfiguration), RolloutInsertionParameters().setTitle(tr("Reference")));

    connect(this, &PropertiesEditor::contentsChanged, this, &AtomicStrainModBurgersEditor::onContentsChanged);
}


/******************************************************************************
* Is called when the user clicks one of the source mode buttons.
******************************************************************************/
void AtomicStrainModBurgersEditor::onSourceButtonClicked(int id)
{
	ReferenceConfigurationModifier* mod = static_object_cast<ReferenceConfigurationModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Set reference source mode"), [mod,id]() {
		if(id == 1) {
			// Create a file source object, which can be used for loading
			// the reference configuration from a separate file.
			OORef<FileSource> fileSource(new FileSource(mod->dataset()));
			
			// Disable automatic adjustment of animation length for the secondary file source.
			// We don't want the scene's animation interval to be affected by this.
			fileSource->setAdjustAnimationIntervalEnabled(false);
			mod->setReferenceConfiguration(fileSource);
		}
		else {
			mod->setReferenceConfiguration(nullptr);
		}	
	});
}

/******************************************************************************
* Is called when the object being edited changes.
******************************************************************************/
void AtomicStrainModBurgersEditor::onContentsChanged(RefTarget* editObject)
{
    ReferenceConfigurationModifier* mod = static_object_cast<ReferenceConfigurationModifier>(editObject);
    if(mod) {
        _sourceButtonGroup->button(0)->setEnabled(true);
        _sourceButtonGroup->button(1)->setEnabled(true);
        _sourceButtonGroup->button(mod->referenceConfiguration() ? 1 : 0)->setChecked(true);
    }
    else {
        _sourceButtonGroup->button(0)->setEnabled(false);
        _sourceButtonGroup->button(1)->setEnabled(false);
    }
}

/******************************************************************************
* Is called when the spinner value has changed.
******************************************************************************/
//void AtomicStrainModBurgersEditor::onSpinnerValueChanged()
//{
   // if(!dataset()->undoStack().isRecording()) {
 //       UndoableTransaction transaction(dataset()->undoStack(), tr("Change parameter"));
//        updateParameterValue();
//        transaction.commit();
//    }
//    else {
//        dataset()->undoStack().resetCurrentCompoundOperation();
//        updateParameterValue();
//    }
//}

/******************************************************************************
* Takes the value entered by the user and stores it in transformation controller.
******************************************************************************/
//void AtomicStrainModBurgersEditor::updateParameterValue()
//{
    //AtomicStrainModBurgers* mod = dynamic_object_cast<AtomicStrainModBurgers>(editObject());
  //  if(!mod) return;
//
    // Get the spinner whose value has changed.
//    SpinnerWidget* spinner = qobject_cast<SpinnerWidget*>(sender());
//    OVITO_CHECK_POINTER(spinner);
//
//    Vector3 b = mod-> burgersContent();
//
//    int column = spinner->property("column").toInt();
//    int row = spinner->property("row").toInt();
//
//    b[column] = spinner->floatValue();
//    mod->setBurgersContent(b);
//}

/******************************************************************************
* Is called when the user begins dragging the spinner interactively.
******************************************************************************/
//void AtomicStrainModBurgersEditor::onSpinnerDragStart()
//{
//    OVITO_ASSERT(!dataset()->undoStack().isRecording());
//    dataset()->undoStack().beginCompoundOperation(tr("Change parameter"));
//}

/******************************************************************************
* Is called when the user stops dragging the spinner interactively.
******************************************************************************/
//void AtomicStrainModBurgersEditor::onSpinnerDragStop()
//{
//    OVITO_ASSERT(dataset()->undoStack().isRecording());
//    dataset()->undoStack().endCompoundOperation();
//}

/******************************************************************************
* Is called when the user aborts dragging the spinner interactively.
******************************************************************************/
//void AtomicStrainModBurgersEditor::onSpinnerDragAbort()
//{
//    OVITO_ASSERT(dataset()->undoStack().isRecording());
//    dataset()->undoStack().endCompoundOperation(false);
//}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
