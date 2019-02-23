///////////////////////////////////////////////////////////////////////////////
// Written by Eric 2019
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/wallace/modifier/elastic_stability/CalculateElasticStabilityModifier.h>
#include <plugins/wallace/modifier/elastic_stability/ElasticConstants.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanRadioButtonParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include "CalculateElasticStabilityModifierEditor.h"
#include <QtAlgorithms>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(CalculateElasticStabilityModifierEditor);
SET_OVITO_OBJECT_EDITOR(CalculateElasticStabilityModifier, CalculateElasticStabilityModifierEditor);

/*****
 * Set up UI Widgets
 */
void CalculateElasticStabilityModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
    //Create the first rollout
    QWidget* rollout = createRollout(tr("Elastic Stability"), rolloutParams, "wallace.modifiers.elastic_stability.html");


    //create rollout contents
    QVBoxLayout* layout = new QVBoxLayout(rollout);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);



    //Structure symmetry type
    QGroupBox* structureBox = new QGroupBox(tr("Input crystal type"));
    layout->addWidget(structureBox);
    QVBoxLayout* sublayout1 = new QVBoxLayout(structureBox);
    sublayout1->setContentsMargins(4,4,4,4);
    crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(CalculateElasticStabilityModifier::structure));

    crystalStructureUI->comboBox()->addItem(tr("Cubic m-3m"), QVariant::fromValue((int)ElasticConstants::CUBIC_HIGH));
    crystalStructureUI->comboBox()->addItem(tr("Hexagonal 6/mmm"), QVariant::fromValue((int)ElasticConstants::HEXAGONAL_HIGH));
    sublayout1->addWidget(crystalStructureUI->comboBox());

    //
    //stacked layout for SOEC
    //
    QStackedLayout* stackedSoecLayout = new QStackedLayout;

    layout->addLayout(stackedSoecLayout);

    //ADD PAGES TO THE QSTACKWIDGET for SOEC

    int currStruct[] = {ElasticConstants::CUBIC_HIGH, ElasticConstants::HEXAGONAL_HIGH};
    //Should be able to loop over the allowed cases, make some association between list of labels and elastic constants...
    for ( unsigned int i = 0; i < 2; i++){
    switch(i) {
        case(1): {
            QWidget* CubicHighPage = new QWidget;
            QGridLayout* sublayout = new QGridLayout(CubicHighPage);
            stackedSoecLayout->addWidget(CubicHighPage);
            sublayout -> setContentsMargins(30, 4, 4, 4);
            SpinnerWidget* soecSpinners[3];
            QString labels[3] = {"C11", "C12", "C44"};
            for (int row = 0; row < 3; row++) {
                QLineEdit* lineEdit = new QLineEdit();
                SpinnerWidget* spinner = new SpinnerWidget();
                QLabel* currentLabel = new QLabel(labels[row]);

                soecSpinners[row] = spinner;
                spinner->setProperty("row", row);
                spinner->setTextBox(lineEdit);
                sublayout->addWidget(lineEdit, row+1, 4);
                sublayout->addWidget(spinner, row+1, 4+1);
                sublayout->addWidget(currentLabel, row+1, 2);

                connect(spinner, &SpinnerWidget::spinnerValueChanged, this, &CalculateElasticStabilityModifierEditor::setSoecSpinners);
                connect(spinner, &SpinnerWidget::spinnerDragStart, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStart);
                connect(spinner, &SpinnerWidget::spinnerDragStop, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStop);
                connect(spinner, &SpinnerWidget::spinnerDragAbort, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragAbort);

            }
        }
        case(2): {
            QWidget* HexagonalHighPage = new QWidget;
            stackedSoecLayout->addWidget(HexagonalHighPage);
            QGridLayout* sublayout = new QGridLayout(HexagonalHighPage);
            sublayout -> setContentsMargins(30, 4, 4, 4);
            SpinnerWidget* soecSpinners[5];
            QString labels[5] = {"C11", "C12", "C13", "C33", "C44"};
            for (int row = 0; row < 5; row++) {
                QLineEdit* lineEdit = new QLineEdit();
                SpinnerWidget* spinner = new SpinnerWidget();
                QLabel* currentLabel = new QLabel(labels[row]);

                soecSpinners[row] = spinner;
                spinner->setProperty("row", row);
                spinner->setTextBox(lineEdit);
                sublayout->addWidget(lineEdit, row+1, 4);
                sublayout->addWidget(spinner, row+1, 4+1);
                sublayout->addWidget(currentLabel, row+1, 2);

                connect(spinner, &SpinnerWidget::spinnerValueChanged, this, &CalculateElasticStabilityModifierEditor::setSoecSpinners);
                connect(spinner, &SpinnerWidget::spinnerDragStart, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStart);
                connect(spinner, &SpinnerWidget::spinnerDragStop, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStop);
                connect(spinner, &SpinnerWidget::spinnerDragAbort, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragAbort);
                }
            }
        }
    }
    connect(crystalStructureUI->comboBox(), SIGNAL(activated(int)), stackedSoecLayout, SLOT(setCurrentIndex(int)));


    //
    //stacked layout for TOEC
    //
    QStackedLayout* stackedToecLayout = new QStackedLayout;

    layout->addLayout(stackedToecLayout);

    //ADD PAGES TO THE QSTACKWIDGET for TOEC

    //Should be able to loop over the allowed cases, make some association between list of labels and elastic constants...
    for ( unsigned int i = 0; i < 2; i++){
    switch(i) {
        case(1): {
            QWidget* CubicHighPage = new QWidget;
            QGridLayout* sublayout = new QGridLayout(CubicHighPage);
            stackedToecLayout->addWidget(CubicHighPage);
            sublayout -> setContentsMargins(30, 4, 4, 4);
            SpinnerWidget* toecSpinners[6];
            QString labels[6] = {"C111", "C112", "C133", "C144", "C155", "C456"};
            for (int row = 0; row < 6; row++) {
                QLineEdit* lineEdit = new QLineEdit();
                SpinnerWidget* spinner = new SpinnerWidget();
                QLabel* currentLabel = new QLabel(labels[row]);

                toecSpinners[row] = spinner;
                spinner->setProperty("row", row);
                spinner->setTextBox(lineEdit);
                sublayout->addWidget(lineEdit, row+1, 4);
                sublayout->addWidget(spinner, row+1, 4+1);
                sublayout->addWidget(currentLabel, row+1, 2);

                connect(spinner, &SpinnerWidget::spinnerValueChanged, this, &CalculateElasticStabilityModifierEditor::setToecSpinners);
                connect(spinner, &SpinnerWidget::spinnerDragStart, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStart);
                connect(spinner, &SpinnerWidget::spinnerDragStop, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStop);
                connect(spinner, &SpinnerWidget::spinnerDragAbort, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragAbort);

            }
        }
        case(2): {
            QWidget* HexagonalHighPage = new QWidget;
            stackedToecLayout->addWidget(HexagonalHighPage);
            QGridLayout* sublayout = new QGridLayout(HexagonalHighPage);
            sublayout -> setContentsMargins(30, 4, 4, 4);
            SpinnerWidget* toecSpinners[10];
            QString labels[10] = {"C111", "C112", "C113", "C123", "C133", "C144", "C155", "C222", "C333", "C344"};
            for (int row = 0; row < 10; row++) {
                QLineEdit* lineEdit = new QLineEdit();
                SpinnerWidget* spinner = new SpinnerWidget();
                QLabel* currentLabel = new QLabel(labels[row]);

                toecSpinners[row] = spinner;
                spinner->setProperty("row", row);
                spinner->setTextBox(lineEdit);
                sublayout->addWidget(lineEdit, row+1, 4);
                sublayout->addWidget(spinner, row+1, 4+1);
                sublayout->addWidget(currentLabel, row+1, 2);

                connect(spinner, &SpinnerWidget::spinnerValueChanged, this, &CalculateElasticStabilityModifierEditor::setToecSpinners);
                connect(spinner, &SpinnerWidget::spinnerDragStart, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStart);
                connect(spinner, &SpinnerWidget::spinnerDragStop, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragStop);
                connect(spinner, &SpinnerWidget::spinnerDragAbort, this, &CalculateElasticStabilityModifierEditor::onSpinnerDragAbort);
                }
            }
        }
    }
    connect(crystalStructureUI->comboBox(), SIGNAL(activated(int)), stackedToecLayout, SLOT(setCurrentIndex(int)));


    connect(crystalStructureUI->comboBox(), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CalculateElasticStabilityModifierEditor::onCurrentTextChanged);



    //Press button to do calculation
    QGridLayout* sublayout = new QGridLayout();
    sublayout->setContentsMargins(0,0,0,0);
    sublayout->setSpacing(0);
    sublayout->setColumnStretch(0, 1);
    QAction* enterParametersAction = new QAction(tr("Run Calculation"), this);
    QToolButton* enterParametersButton = new QToolButton();
    enterParametersButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    enterParametersButton->setDefaultAction(enterParametersAction);
    sublayout->addWidget(enterParametersButton, 0, 1, Qt::Alignment(Qt::AlignBottom | Qt::AlignRight));
    enterParametersAction->setEnabled(true);
    connect(enterParametersAction, &QAction::triggered, this, &CalculateElasticStabilityModifierEditor::onButtonPressed);
    layout->addLayout(sublayout);
} 

/*******************************************
 * Call when a structure is chosen
 * ***************************************/
 void CalculateElasticStabilityModifierEditor::onCurrentTextChanged() {
     //get the number of constants for currently selected structure
     int currentStruct = crystalStructureUI->comboBox()->currentData().toInt();

     //set _vecSoec to 0 vector of correct length for current structure
     int numSoec = ElasticConstants::numberSoec(currentStruct);
     std::vector<float> vecSoec(numSoec, 0);
     _vecSoec = vecSoec;

     //set _vecToec to 0 vector of correct length for current structure
     int numToec = ElasticConstants::numberToec(currentStruct);
     std::vector<float> vecToec(numToec, 0);
     _vecToec = vecToec;
 }


/******************************************************************************
* Is called when the user begins dragging the spinner interactively.
******************************************************************************/
void CalculateElasticStabilityModifierEditor::onSpinnerDragStart()
{
    OVITO_ASSERT(!dataset()->undoStack().isRecording());
    dataset()->undoStack().beginCompoundOperation(tr("Change parameter"));
}

/******************************************************************************
* Is called when the user stops dragging the spinner interactively.
******************************************************************************/
void CalculateElasticStabilityModifierEditor::onSpinnerDragStop()
{
    OVITO_ASSERT(dataset()->undoStack().isRecording());
    dataset()->undoStack().endCompoundOperation();
}

/******************************************************************************
* Is called when the user aborts dragging the spinner interactively.
******************************************************************************/
void CalculateElasticStabilityModifierEditor::onSpinnerDragAbort()
{
    OVITO_ASSERT(dataset()->undoStack().isRecording());
    dataset()->undoStack().endCompoundOperation(false);
}

/******************************************************************************
* Is called when the button is pressed - to submit soec and toec and run modifier
******************************************************************************/
void CalculateElasticStabilityModifierEditor::onButtonPressed() {
    CalculateElasticStabilityModifier* mod = dynamic_object_cast<CalculateElasticStabilityModifier>(editObject());
    if(!mod) return;

    //get the number of constants for currently selected structure
    int currentStruct = crystalStructureUI->comboBox()->currentData().toInt();

    //set all parameters for calculation
    mod->setStructure(currentStruct);
    mod->setSoec(_vecSoec); //RUNNING INTO SEG FAULT HERE
    mod->setToec(_vecToec);
}


/******************************************************************************
* Is called when the spinner value has changed. For Soec
******************************************************************************/
void CalculateElasticStabilityModifierEditor::setSoecSpinners() {
    CalculateElasticStabilityModifier* mod = static_object_cast<CalculateElasticStabilityModifier>(editObject());
    if(!mod) return;

    // Get the spinner whose value has changed.
    SpinnerWidget* spinner = qobject_cast<SpinnerWidget*>(sender());
    OVITO_CHECK_POINTER(spinner);

    int row = spinner->property("row").toInt();
    _vecSoec[row] = spinner->floatValue();
}

/******************************************************************************
* Is called when the spinner value has changed. For Toec
******************************************************************************/
void CalculateElasticStabilityModifierEditor::setToecSpinners() {
    CalculateElasticStabilityModifier* mod = static_object_cast<CalculateElasticStabilityModifier>(editObject());
    if(!mod) return;

    // Get the spinner whose value has changed.
    SpinnerWidget* spinner = qobject_cast<SpinnerWidget*>(sender());
    OVITO_CHECK_POINTER(spinner);

    int row = spinner->property("row").toInt();
    _vecToec[row] = spinner->floatValue();
}
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}
}
