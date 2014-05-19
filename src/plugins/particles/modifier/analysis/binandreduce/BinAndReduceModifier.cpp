///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
//  Copyright (2014) Lars Pastewka
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
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/IntegerRadioButtonParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/gui/mainwin/MainWindow.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/animation/AnimationSettings.h>
#include <plugins/particles/util/ParticlePropertyParameterUI.h>
#include "BinAndReduceModifier.h"

namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, BinAndReduceModifier, ParticleModifier);
IMPLEMENT_OVITO_OBJECT(Particles, BinAndReduceModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(BinAndReduceModifier, BinAndReduceModifierEditor);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, _reductionOperation, "ReductionOperation", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, _binDirection, "BinDirection", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, _numberOfBinsX, "NumberOfBinsX", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, _numberOfBinsY, "NumberOfBinsY", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(BinAndReduceModifier, _fixPropertyAxisRange, "FixPropertyAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, _propertyAxisRangeStart, "PropertyAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, _propertyAxisRangeEnd, "PropertyAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(BinAndReduceModifier, _sourceProperty, "SourceProperty");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _reductionOperation, "Reduction operation");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _binDirection, "Bin direction");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _numberOfBinsX, "Number of spatial bins");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _numberOfBinsY, "Number of spatial bins");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _fixPropertyAxisRange, "Fix property-axis range");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _propertyAxisRangeStart, "Property-axis range start");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _propertyAxisRangeEnd, "Property-axis range end");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, _sourceProperty, "Source property");

inline int modulo(int i, int n)
{
    while (i < 0) i += n;
    return i%n;
}

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
BinAndReduceModifier::BinAndReduceModifier(DataSet* dataset) : 
    ParticleModifier(dataset), _reductionOperation(RED_MEAN), _binDirection(CELL_VECTOR_3),_numberOfBinsX(200),
    _numberOfBinsY(1), _fixPropertyAxisRange(false), _propertyAxisRangeStart(0), _propertyAxisRangeEnd(0)
{
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_reductionOperation);
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_binDirection);
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_numberOfBinsX);
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_numberOfBinsY);
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_fixPropertyAxisRange);
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_propertyAxisRangeStart);
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_propertyAxisRangeEnd);
	INIT_PROPERTY_FIELD(BinAndReduceModifier::_sourceProperty);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void BinAndReduceModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull()) {
		PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
		ParticlePropertyReference bestProperty;
		for(SceneObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull()) {
			setSourceProperty(bestProperty);
		}
	}
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus BinAndReduceModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
    size_t binDataSizeX = std::max(1, numberOfBinsX());
    size_t binDataSizeY = std::max(1, numberOfBinsY());
    if (is1D()) binDataSizeY = 1;
    size_t binDataSize = binDataSizeX*binDataSizeY;
	_binData.resize(binDataSize);
	std::fill(_binData.begin(), _binData.end(), 0);

    // Return coordinate indices (0, 1 or 2).
    int binDirX = binDirectionX(_binDirection);
    int binDirY = binDirectionY(_binDirection);

    // Number of particles for averaging.
    std::vector<int> numberOfParticlesPerBin(binDataSize, 0);

	// Get the source property.
	if(sourceProperty().isNull())
		throw Exception(tr("Select a particle property first."));
	ParticlePropertyObject* property = sourceProperty().findInState(input());
	if(!property)
		throw Exception(tr("The selected particle property with the name '%1' does not exist.").arg(sourceProperty().name()));
	if(sourceProperty().vectorComponent() >= (int)property->componentCount())
		throw Exception(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(sourceProperty().name()).arg(property->componentCount()));

	size_t vecComponent = sourceProperty().vectorComponent() >= 0 ? sourceProperty().vectorComponent() : 0;
	size_t vecComponentCount = property->componentCount();

    // Get bottom-left and top-right corner of the simulation cell.
    AffineTransformation reciprocalCell = expectSimulationCell()->reciprocalCellMatrix();

    // Get periodic boundary flag.
	std::array<bool, 3> pbc = expectSimulationCell()->pbcFlags();

    // Compute the surface normal vector.
    Vector3 normalX, normalY(1, 1, 1);
    if (_binDirection == CELL_VECTOR_1) {
        normalX = expectSimulationCell()->edgeVector2().cross(expectSimulationCell()->edgeVector3());
    }
    else if (_binDirection == CELL_VECTOR_2) {
        normalX = expectSimulationCell()->edgeVector1().cross(expectSimulationCell()->edgeVector3());
    }
    else if (_binDirection == CELL_VECTOR_3) {
        normalX = expectSimulationCell()->edgeVector1().cross(expectSimulationCell()->edgeVector2());
    }
    else if (_binDirection == CELL_VECTORS_1_2) {
        normalX = expectSimulationCell()->edgeVector2().cross(expectSimulationCell()->edgeVector3());
        normalY = expectSimulationCell()->edgeVector1().cross(expectSimulationCell()->edgeVector3());
    }
    else if (_binDirection == CELL_VECTORS_2_3) {
        normalX = expectSimulationCell()->edgeVector1().cross(expectSimulationCell()->edgeVector3());
        normalY = expectSimulationCell()->edgeVector1().cross(expectSimulationCell()->edgeVector2());
    }
    else if (_binDirection == CELL_VECTORS_1_3) {
        normalX = expectSimulationCell()->edgeVector2().cross(expectSimulationCell()->edgeVector3());
        normalY = expectSimulationCell()->edgeVector1().cross(expectSimulationCell()->edgeVector3());
    }

    // Compute the distance of the two cell faces (normal.length() is area of face).
    FloatType cellVolume = expectSimulationCell()->volume();
    _xAxisRangeStart = 0.0;
    _xAxisRangeEnd = cellVolume / normalX.length();
    _yAxisRangeStart = 0.0;
    _yAxisRangeEnd = cellVolume / normalY.length();

	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	if(property->size() > 0) {
        const Point3* pos = posProperty->constDataPoint3();
        const Point3* pos_end = pos + posProperty->size();

		if(property->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* v = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v + (property->size() * vecComponentCount);

            while (pos != pos_end && v != v_end) {
                if (!std::isnan(*v)) {
                    FloatType fractionalPosX = reciprocalCell.prodrow(*pos, binDirX);
                    FloatType fractionalPosY = reciprocalCell.prodrow(*pos, binDirY);
                    ssize_t binIndexX = ssize_t( fractionalPosX * binDataSizeX );
                    ssize_t binIndexY = ssize_t( fractionalPosY * binDataSizeY );
                    if (pbc[binDirX])  binIndexX = modulo(binIndexX, binDataSizeX);
                    if (pbc[binDirY])  binIndexY = modulo(binIndexY, binDataSizeY);
                    if (binIndexX >= 0 && binIndexX < binDataSizeX && binIndexY >= 0 && binIndexY < binDataSizeY) {
                        size_t binIndex = binIndexY*binDataSizeX+binIndexX;
                        if (_reductionOperation == RED_MEAN || _reductionOperation == RED_SUM || _reductionOperation == RED_SUM_VOL) {
                            _binData[binIndex] += *v;
                        } 
                        else {
                            if (numberOfParticlesPerBin[binIndex] == 0) {
                                _binData[binIndex] = *v;  
                            }
                            else {
                                if (_reductionOperation == RED_MAX) {
                                    _binData[binIndex] = std::max(_binData[binIndex], *v);
                                }
                                else if (_reductionOperation == RED_MIN) {
                                    _binData[binIndex] = std::min(_binData[binIndex], *v);
                                }
                            }
                        }
                        numberOfParticlesPerBin[binIndex]++;
                    }
                }
                
                pos++;
                v += vecComponentCount;
            }
		}
		else if(property->dataType() == qMetaTypeId<int>()) {
			const int* v = property->constDataInt() + vecComponent;
			const int* v_end = v + (property->size() * vecComponentCount);

            while (pos != pos_end && v != v_end) {
                FloatType fractionalPosX = reciprocalCell.prodrow(*pos, binDirX);
                FloatType fractionalPosY = reciprocalCell.prodrow(*pos, binDirY);
                ssize_t binIndexX = ssize_t( fractionalPosX * binDataSizeX );
                ssize_t binIndexY = ssize_t( fractionalPosY * binDataSizeY );
                if (pbc[binDirX])  binIndexX = modulo(binIndexX, binDataSizeX);
                if (pbc[binDirY])  binIndexY = modulo(binIndexY, binDataSizeY);
                if (binIndexX >= 0 && binIndexX < binDataSizeX && binIndexY >= 0 && binIndexY < binDataSizeY) {
                    size_t binIndex = binIndexY*binDataSizeX+binIndexX;
                    if (_reductionOperation == RED_MEAN || _reductionOperation == RED_SUM || _reductionOperation == RED_SUM) {
                        _binData[binIndex] += *v;
                    }
                    else {
                        if (numberOfParticlesPerBin[binIndex] == 0) {
                            _binData[binIndex] = *v;  
                        }
                        else {
                            if (_reductionOperation == RED_MAX) {
                                _binData[binIndex] = std::max(_binData[binIndex], FloatType(*v));
                            }
                            else if (_reductionOperation == RED_MIN) {
                                _binData[binIndex] = std::min(_binData[binIndex], FloatType(*v));
                            }
                        }
                    }
                    numberOfParticlesPerBin[binIndex]++;
                }

                pos++;
                v += vecComponentCount;
            }
		}

        if (_reductionOperation == RED_MEAN) {
            // Normalize.
            std::vector<FloatType>::iterator a = _binData.begin();
            for (auto n: numberOfParticlesPerBin) {
                if (n > 0) *a /= n;
                a++;
            }
        }
        else if (_reductionOperation == RED_SUM_VOL) {
            // Divide by bin volume.
            FloatType binVolume = cellVolume / (binDataSizeX*binDataSizeY);
            std::for_each(_binData.begin(), _binData.end(), [binVolume](FloatType &x) { x /= binVolume; });
        }
	}

	if (!_fixPropertyAxisRange) {
		auto minmax = std::minmax_element(_binData.begin(), _binData.end());
		_propertyAxisRangeStart = *minmax.first;
		_propertyAxisRangeEnd = *minmax.second;
	}

	notifyDependents(ReferenceEvent::ObjectStatusChanged);

	return PipelineStatus(PipelineStatus::Success);
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BinAndReduceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Bin and reduce"), rolloutParams /*, "particles.modifiers.binandreduce.html" */);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	ParticlePropertyParameterUI* sourcePropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_sourceProperty));
	layout->addWidget(new QLabel(tr("Property:"), rollout));
	layout->addWidget(sourcePropertyUI->comboBox());

	layout->addWidget(new QLabel(tr("Reduction operation:"), rollout));
	QGridLayout* gridlayout = new QGridLayout();
	IntegerRadioButtonParameterUI* reductionOperationPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_reductionOperation));
    gridlayout->addWidget(reductionOperationPUI->addRadioButton(BinAndReduceModifier::RED_MEAN, tr("mean")), 0, 0);
    gridlayout->addWidget(reductionOperationPUI->addRadioButton(BinAndReduceModifier::RED_SUM, tr("sum")), 0, 1);
    gridlayout->addWidget(reductionOperationPUI->addRadioButton(BinAndReduceModifier::RED_SUM_VOL, tr("sum/volume")), 0, 2);
    gridlayout->addWidget(reductionOperationPUI->addRadioButton(BinAndReduceModifier::RED_MIN, tr("min")), 0, 3);
    gridlayout->addWidget(reductionOperationPUI->addRadioButton(BinAndReduceModifier::RED_MAX, tr("max")), 0, 4);
    layout->addLayout(gridlayout);

	layout->addWidget(new QLabel(tr("Binning direction:"), rollout));
	gridlayout = new QGridLayout();
	IntegerRadioButtonParameterUI* binDirectionPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_binDirection));
    gridlayout->addWidget(binDirectionPUI->addRadioButton(BinAndReduceModifier::CELL_VECTOR_1, "cell vector 1"), 0, 0);
    gridlayout->addWidget(binDirectionPUI->addRadioButton(BinAndReduceModifier::CELL_VECTOR_2, "cell vector 2"), 0, 1);
    gridlayout->addWidget(binDirectionPUI->addRadioButton(BinAndReduceModifier::CELL_VECTOR_3, "cell vector 3"), 0, 2);
    gridlayout->addWidget(binDirectionPUI->addRadioButton(BinAndReduceModifier::CELL_VECTORS_1_2, "vectors 1 and 2"), 1, 0);
    gridlayout->addWidget(binDirectionPUI->addRadioButton(BinAndReduceModifier::CELL_VECTORS_1_3, "vectors 1 and 3"), 1, 1);
    gridlayout->addWidget(binDirectionPUI->addRadioButton(BinAndReduceModifier::CELL_VECTORS_2_3, "vectors 2 and 3"), 1, 2);
    layout->addLayout(gridlayout);
    connect(binDirectionPUI->buttonGroup(), SIGNAL(buttonClicked(int)), this, SLOT(updateBinDirection(int)));

	gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);
	gridlayout->setColumnStretch(2, 1);

	// Number of bins parameters.
	IntegerParameterUI* numBinsXPUI = new IntegerParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_numberOfBinsX));
	gridlayout->addWidget(numBinsXPUI->label(), 0, 0);
	gridlayout->addLayout(numBinsXPUI->createFieldLayout(), 0, 1);
	numBinsXPUI->setMinValue(1);
	_numBinsYPUI = new IntegerParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_numberOfBinsY));
	gridlayout->addLayout(_numBinsYPUI->createFieldLayout(), 0, 2);
	_numBinsYPUI->setMinValue(1);

	layout->addLayout(gridlayout);

	_averagesPlot = new QCustomPlot();
	_averagesPlot->setMinimumHeight(240);
    _averagesPlot->axisRect()->setRangeDrag(Qt::Vertical);
    _averagesPlot->axisRect()->setRangeZoom(Qt::Vertical);
	_averagesPlot->xAxis->setLabel("Position");
    connect(_averagesPlot->yAxis, SIGNAL(rangeChanged(const QCPRange&)), this, SLOT(updatePropertyAxisRange(const QCPRange&)));

	layout->addWidget(new QLabel(tr("Reduction:")));
	layout->addWidget(_averagesPlot);
	connect(this, &BinAndReduceModifierEditor::contentsReplaced, this, &BinAndReduceModifierEditor::plotAverages);

	QPushButton* saveDataButton = new QPushButton(tr("Save data"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &BinAndReduceModifierEditor::onSaveData);

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
    BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_fixPropertyAxisRange));
    axesSublayout->addWidget(rangeUI->checkBox());
        
    QHBoxLayout* hlayout = new QHBoxLayout();
    axesSublayout->addLayout(hlayout);
    FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_propertyAxisRangeStart));
    FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_propertyAxisRangeEnd));
    hlayout->addWidget(new QLabel(tr("From:")));
    hlayout->addLayout(startPUI->createFieldLayout());
    hlayout->addSpacing(12);
    hlayout->addWidget(new QLabel(tr("To:")));
    hlayout->addLayout(endPUI->createFieldLayout());
    startPUI->setEnabled(false);
    endPUI->setEnabled(false);
    connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
    connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool BinAndReduceModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && 
       event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotAverages();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the averaged data computed by the modifier.
******************************************************************************/
void BinAndReduceModifierEditor::plotAverages()
{
	BinAndReduceModifier* modifier = static_object_cast<BinAndReduceModifier>(editObject());
	if(!modifier)
		return;

    size_t binDataSizeX = std::max(1, modifier->numberOfBinsX());
    size_t binDataSizeY = std::max(1, modifier->numberOfBinsY());
    if (modifier->is1D()) binDataSizeY = 1;
    size_t binDataSize = binDataSizeX*binDataSizeY;

    if (modifier->is1D()) {
        // If previous plot was a color map, delete and create graph.
        if (!_averagesGraph) {
            if (_averagesColorMap) {
                _averagesPlot->removePlottable(_averagesColorMap);
                _averagesColorMap = NULL;
            }
            _averagesGraph = _averagesPlot->addGraph();
        }

        _averagesPlot->setInteraction(QCP::iRangeDrag, true);
        _averagesPlot->axisRect()->setRangeDrag(Qt::Vertical);
        _averagesPlot->setInteraction(QCP::iRangeZoom, true);
        _averagesPlot->axisRect()->setRangeZoom(Qt::Vertical);
        _averagesPlot->yAxis->setLabel(modifier->sourceProperty().name());

        if(modifier->binData().empty())
            return;

        QVector<double> xdata(binDataSize);
        QVector<double> ydata(binDataSize);
        double binSize = ( modifier->xAxisRangeEnd() - modifier->xAxisRangeStart() ) / binDataSize;
        double maxBinData = 0.0;
        for(int i = 0; i < xdata.size(); i++) {
            xdata[i] = binSize * ((double)i + 0.5);
            ydata[i] = modifier->binData()[i];
            maxBinData = std::max(maxBinData, ydata[i]);
        }
        _averagesPlot->graph()->setLineStyle(QCPGraph::lsStepCenter);
        _averagesPlot->graph()->setData(xdata, ydata);

        // Check if range is already correct, because setRange emits the rangeChanged signal
        // which is to be avoided if the range is not determined automatically.
        _rangeUpdate = false;
        _averagesPlot->xAxis->setRange(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());
        _averagesPlot->yAxis->setRange(modifier->PropertyAxisRangeStart(), modifier->PropertyAxisRangeEnd());
        _rangeUpdate = true;
    }
    else {
        // If previous plot was a graph, delete and create color map.
        if (!_averagesColorMap) {
            if (_averagesGraph) {
                _averagesPlot->removeGraph(_averagesGraph);
                _averagesGraph = NULL;
            }
            _averagesColorMap = new QCPColorMap(_averagesPlot->xAxis, _averagesPlot->yAxis);
            _averagesPlot->addPlottable(_averagesColorMap);
        }

        _averagesPlot->setInteraction(QCP::iRangeDrag, false);
        _averagesPlot->setInteraction(QCP::iRangeZoom, false);
        _averagesPlot->yAxis->setLabel("Position");

        if(modifier->binData().empty())
            return;

        _averagesColorMap->setInterpolate(false);
        _averagesColorMap->setTightBoundary(false);
        _averagesColorMap->setGradient(QCPColorGradient::gpJet);

        _averagesColorMap->data()->setSize(binDataSizeX, binDataSizeY);
        _averagesColorMap->data()->setRange(QCPRange(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd()),
                                            QCPRange(modifier->yAxisRangeStart(), modifier->yAxisRangeEnd()));
        for (int j = 0; j < binDataSizeY; j++) {
            for (int i = 0; i < binDataSizeX; i++) {
                _averagesColorMap->data()->setCell(i, j, modifier->binData()[j*binDataSizeX+i]);
            }
        }

        // Check if range is already correct, because setRange emits the rangeChanged signal
        // which is to be avoided if the range is not determined automatically.
        _rangeUpdate = false;
        _averagesColorMap->setDataRange(QCPRange(modifier->PropertyAxisRangeStart(), modifier->PropertyAxisRangeEnd()));
        _rangeUpdate = true;

        _averagesPlot->rescaleAxes();
    }
    
    _averagesPlot->replot();
}

/******************************************************************************
* Enable/disable the editor for number of y-bin
******************************************************************************/
void BinAndReduceModifierEditor::updateBinDirection(int newBinDirection)
{
    _numBinsYPUI->setEnabled(!BinAndReduceModifier::bin1D(BinAndReduceModifier::BinDirectionType(newBinDirection)));
}


/******************************************************************************
* Keep y-axis range updated
******************************************************************************/
void BinAndReduceModifierEditor::updatePropertyAxisRange(const QCPRange &newRange)
{
	if (_rangeUpdate) {
		BinAndReduceModifier* modifier = static_object_cast<BinAndReduceModifier>(editObject());
		if(!modifier)
			return;
        if (!modifier->is1D())
            return;

		// Fix range if user modifies the range by a mouse action in QCustomPlot
		modifier->setFixPropertyAxisRange(true);
		modifier->setPropertyAxisRange(newRange.lower, newRange.upper);
	}
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void BinAndReduceModifierEditor::onSaveData()
{
	BinAndReduceModifier* modifier = static_object_cast<BinAndReduceModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->binData().empty())
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save Data"), QString(), 
        tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			throw Exception(tr("Could not open file for writing: %1").arg(file.errorString()));

        size_t binDataSizeX = std::max(1, modifier->numberOfBinsX());
        size_t binDataSizeY = std::max(1, modifier->numberOfBinsY());
        if (modifier->is1D()) binDataSizeY = 1;
        size_t binDataSize = binDataSizeX*binDataSizeY;
		FloatType binSizeX = (modifier->xAxisRangeEnd() - modifier->xAxisRangeStart()) / binDataSizeX;
		FloatType binSizeY = (modifier->yAxisRangeEnd() - modifier->yAxisRangeStart()) / binDataSizeY;

		QTextStream stream(&file);
        if (binDataSizeY == 1) {
            stream << "# " << modifier->sourceProperty().name() << " bin size: " << binSizeX << endl;
            for(int i = 0; i < modifier->binData().size(); i++) {
                stream << (binSizeX * (FloatType(i) + 0.5f) + modifier->xAxisRangeStart()) << " " << modifier->binData()[i] << endl;
            }
        }
        else {
            stream << "# " << modifier->sourceProperty().name() << " bin size X: " << binDataSizeX << ", bin size Y: " << binDataSizeY << endl;
            for(int i = 0; i < binDataSizeY; i++) {
                for(int j = 0; j < binDataSizeX; j++) {
                    stream << modifier->binData()[i*binDataSizeX+j] << " ";
                }
                stream << endl;
            }
        }
	}
	catch(const Exception& ex) {
		ex.showError();
	}
}


};	// End of namespace
