///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <plugins/stdmod/gui/StdModGui.h>
#include <plugins/stdmod/modifiers/ExpressionSelectionModifier.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/properties/ModifierDelegateParameterUI.h>
#include <gui/widgets/general/AutocompleteTextEdit.h>
#include "ExpressionSelectionModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ExpressionSelectionModifierEditor);
SET_OVITO_OBJECT_EDITOR(ExpressionSelectionModifier, ExpressionSelectionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ExpressionSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Expression selection"), rolloutParams, "particles.modifiers.expression_select.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(0);

	ModifierDelegateParameterUI* delegateUI = new ModifierDelegateParameterUI(this, ExpressionSelectionModifierDelegate::OOClass());
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(delegateUI->comboBox());

	layout->addWidget(new QLabel(tr("Boolean expression:")));
	StringParameterUI* expressionUI = new StringParameterUI(this, PROPERTY_FIELD(ExpressionSelectionModifier::expression));
	expressionEdit = new AutocompleteTextEdit();
	expressionUI->setTextBox(expressionEdit);
	layout->addWidget(expressionUI->textBox());

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout), "particles.modifiers.expression_select.html");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
	variableNamesList = new QLabel();
	variableNamesList->setWordWrap(true);
	variableNamesList->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(variableNamesList, 1);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &ExpressionSelectionModifierEditor::contentsReplaced, this, &ExpressionSelectionModifierEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ExpressionSelectionModifierEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ObjectStatusChanged) {
		updateEditorFields();
	}
	return ModifierPropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the enabled/disabled status of the editor's controls.
******************************************************************************/
void ExpressionSelectionModifierEditor::updateEditorFields()
{
	ExpressionSelectionModifier* mod = static_object_cast<ExpressionSelectionModifier>(editObject());
	if(!mod) return;

	variableNamesList->setText(mod->inputVariableTable() + QStringLiteral("<p></p>"));
	container()->updateRolloutsLater();
	expressionEdit->setWordList(mod->inputVariableNames());
}

}	// End of namespace
}	// End of namespace
