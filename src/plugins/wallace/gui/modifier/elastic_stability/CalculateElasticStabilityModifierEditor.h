///
//Written By Eric 2019
///

#pragma once

#include <plugins/particles/gui/ParticlesGui.h>
#include <gui/properties/ModifierPropertiesEditor.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for CalculateElasticStability class
 */
class CalculateElasticStabilityModifierEditor : public ModifierPropertiesEditor
{
    Q_OBJECT
        OVITO_CLASS(CalculateElasticStabilityModifierEditor)

public:

    /// Default constructor
    Q_INVOKABLE CalculateElasticStabilityModifierEditor() :
        _vecSoec(std::vector<float>({0 ,0, 0})),
        _vecToec(std::vector<float>({0 ,0, 0, 0, 0, 0}))
    {}


private Q_SLOTS:

    /// Is called when type value changed
    void onCurrentTextChanged();

    /// Is called when the spinner value has changed.
    //void onSpinnerValueChanged();
    void setSoecSpinners();

    void setToecSpinners();

    /// Is called when the user begins dragging the spinner interactively.
    void onSpinnerDragStart();

    /// Is called when the user stops dragging the spinner interactively.
    void onSpinnerDragStop();

    /// Is called when the user aborts dragging the spinner interactively.
    void onSpinnerDragAbort();

    /// Is called when button is pressed
    void onButtonPressed();

protected:
    /// Creates the user interface controls for the editor.
    virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

    VariantComboBoxParameterUI* crystalStructureUI;

    std::vector<float> _vecSoec;

    std::vector<float> _vecToec;


    //SpinnerWidget* soecSpinners;
    //SpinnerWidget* toecSpinners;

};


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}
}
