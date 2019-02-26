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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlesVis.h>
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "ParticlesObject.h"
#include "ParticlesVis.h"
#include "BondsVis.h"
#include "VectorVis.h"
#include "ParticleBondMap.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticlesObject);
DEFINE_REFERENCE_FIELD(ParticlesObject, bonds);
SET_PROPERTY_FIELD_LABEL(ParticlesObject, bonds, "Bonds");

/******************************************************************************
* Constructor.
******************************************************************************/
ParticlesObject::ParticlesObject(DataSet* dataset) : PropertyContainer(dataset)
{
	// Attach a visualization element for rendering the particles.
	addVisElement(new ParticlesVis(dataset));
}

/******************************************************************************
* Duplicates the BondsObject if it is shared with other particle objects.
* After this method returns, all BondsObject are exclusively owned by the 
* container and can be safely modified without unwanted side effects.
******************************************************************************/
BondsObject* ParticlesObject::makeBondsMutable()
{
    OVITO_ASSERT(bonds());
    return makeMutable(bonds());
}

/******************************************************************************
* Convinience method that makes sure that there is a BondsObject. 
* Throws an exception if there isn't.
******************************************************************************/
const BondsObject* ParticlesObject::expectBonds() const
{
    if(!bonds())
		throwException(tr("There are no bonds."));
	return bonds();
}

/******************************************************************************
* Convinience method that makes sure that there is a BondsObject and the 
* bond topology property. Throws an exception if there isn't.
******************************************************************************/
const PropertyObject* ParticlesObject::expectBondsTopology() const
{
    return expectBonds()->expectProperty(BondsObject::TopologyProperty);
}

/******************************************************************************
* Deletes the particles for which bits are set in the given bit-mask.
* Returns the number of deleted particles.
******************************************************************************/
size_t ParticlesObject::deleteParticles(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(mask.size() == elementCount());

	size_t deleteCount = mask.count();
	size_t oldParticleCount = elementCount();
	size_t newParticleCount = oldParticleCount - deleteCount;
	if(deleteCount == 0)
		return 0;	// Nothing to delete.

    // Make sure the properties can be safely modified.
    makePropertiesMutable();

	// Modify particle properties.
	for(PropertyObject* property : properties()) {
        OVITO_ASSERT(property->size() == oldParticleCount);
        property->filterResize(mask);
        OVITO_ASSERT(property->size() == newParticleCount);
	}
    OVITO_ASSERT(elementCount() == newParticleCount);

	// Delete dangling bonds, i.e. those that are incident on deleted particles.
    if(bonds()) {
        // Make sure we can safely modify the bonds object.
        BondsObject* mutableBonds = makeBondsMutable();

        size_t newBondCount = 0;
        size_t oldBondCount = mutableBonds->elementCount();
        boost::dynamic_bitset<> deletedBondsMask(oldBondCount);

        // Build map from old particle indices to new indices.
        std::vector<size_t> indexMap(oldParticleCount);
        auto index = indexMap.begin();
        size_t count = 0;
        for(size_t i = 0; i < oldParticleCount; i++)
            *index++ = mask.test(i) ? std::numeric_limits<size_t>::max() : count++;

        // Remap particle indices of stored bonds and remove dangling bonds.
        if(const PropertyObject* topologyProperty = mutableBonds->getTopology()) {
            PropertyPtr mutableTopology = mutableBonds->makeMutable(topologyProperty)->modifiableStorage();
			for(size_t bondIndex = 0; bondIndex < oldBondCount; bondIndex++) {
                size_t index1 = mutableTopology->getInt64Component(bondIndex, 0);
                size_t index2 = mutableTopology->getInt64Component(bondIndex, 1);

                // Remove invalid bonds, i.e. whose particle indices are out of bounds.
                if(index1 >= oldParticleCount || index2 >= oldParticleCount) {
                    deletedBondsMask.set(bondIndex);
                    continue;
                }

                // Remove dangling bonds whose particles have gone.
                if(mask.test(index1) || mask.test(index2)) {
                    deletedBondsMask.set(bondIndex);
                    continue;
                }

                // Keep bond and remap particle indices.
                mutableTopology->setInt64Component(bondIndex, 0, indexMap[index1]);
                mutableTopology->setInt64Component(bondIndex, 1, indexMap[index2]);
            }

            // Delete the marked bonds.
            mutableBonds->deleteBonds(deletedBondsMask);
        }
    }

	return deleteCount;
}

/******************************************************************************
* Adds a set of new bonds to the particle system.
******************************************************************************/
void ParticlesObject::addBonds(const std::vector<Bond>& newBonds, BondsVis* bondsVis, const std::vector<PropertyPtr>& bondProperties)
{
	// Create the bonds object.
	if(!bonds())
		setBonds(new BondsObject(dataset()));
	else
		makeBondsMutable();
	if(bondsVis)
		bonds()->setVisElement(bondsVis);

	// Check if there are existing bonds.
	OORef<PropertyObject> existingBondsTopology = bonds()->getProperty(BondsObject::TopologyProperty);
	if(!existingBondsTopology) {
		OVITO_ASSERT(bonds()->properties().empty());
		OVITO_ASSERT(bonds()->elementCount() == 0);

		// Create essential bond properties.
		PropertyPtr topologyProperty = BondsObject::OOClass().createStandardStorage(newBonds.size(), BondsObject::TopologyProperty, false);
		PropertyPtr periodicImageProperty = BondsObject::OOClass().createStandardStorage(newBonds.size(), BondsObject::PeriodicImageProperty, false);

		// Copy data into property arrays.
		auto t = topologyProperty->dataInt64();
		auto pbc = periodicImageProperty->dataVector3I();
		for(const Bond& bond : newBonds) {
			OVITO_ASSERT(bond.index1 < elementCount());
			OVITO_ASSERT(bond.index2 < elementCount());
			*t++ = bond.index1;
			*t++ = bond.index2;
			*pbc++ = bond.pbcShift;
		}

		// Insert property objects into the output pipeline state.
		PropertyObject* topologyPropertyObj = bonds()->createProperty(topologyProperty);
		PropertyObject* periodicImagePropertyObj = bonds()->createProperty(periodicImageProperty);

		// Insert other bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondsObject::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondsObject::PeriodicImageProperty);
			bonds()->createProperty(bprop);
		}
	}
	else {

		// This is needed to determine which bonds already exist.
		ParticleBondMap bondMap(*expectBonds());

		// Check which bonds are new and need to be merged.
		size_t originalBondCount = bonds()->elementCount();
		size_t outputBondCount = originalBondCount;
		std::vector<size_t> mapping(newBonds.size());
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			// Check if there is already a bond like this.
			const Bond& bond = newBonds[bondIndex];
			auto existingBondIndex = bondMap.findBond(bond);
			if(existingBondIndex == originalBondCount) {
				// It's a new bond.
				mapping[bondIndex] = outputBondCount;
				outputBondCount++;
			}
			else {
				// It's an already existing bond.
				mapping[bondIndex] = existingBondIndex;
			}
		}

		// Duplicate the existing property objects.
		PropertyObject* newBondsTopology = bonds()->makeMutable(existingBondsTopology.get());
		PropertyObject* newBondsPeriodicImages = bonds()->createProperty(BondsObject::PeriodicImageProperty, true);

		// Copy bonds information into the extended arrays.
		newBondsTopology->resize(outputBondCount, true);
		newBondsPeriodicImages->resize(outputBondCount, true);
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			if(mapping[bondIndex] >= originalBondCount) {
				const Bond& bond = newBonds[bondIndex];
				OVITO_ASSERT(bond.index1 < elementCount());
				OVITO_ASSERT(bond.index2 < elementCount());
				newBondsTopology->setInt64Component(mapping[bondIndex], 0, bond.index1);
				newBondsTopology->setInt64Component(mapping[bondIndex], 1, bond.index2);
				newBondsPeriodicImages->setVector3I(mapping[bondIndex], bond.pbcShift);
			}
		}

		// Extend existing bond property arrays.
		bonds()->makePropertiesMutable();
		for(PropertyObject* bondPropertyObject : bonds()->properties()) {
			if(bondPropertyObject == newBondsTopology) continue;
			if(bondPropertyObject == newBondsPeriodicImages) continue;
			if(bondPropertyObject->size() != originalBondCount) continue;

			// Extend array.
			bondPropertyObject->resize(outputBondCount, true);
		}

		// Merge new bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondsObject::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondsObject::PeriodicImageProperty);

			OORef<PropertyObject> propertyObject;

			if(bprop->type() != BondsObject::UserProperty) {
				propertyObject = bonds()->createProperty(bprop->type(), true);
			}
			else {
				propertyObject = bonds()->createProperty(bprop->name(), bprop->dataType(), bprop->componentCount(), bprop->stride(), true);
			}

			// Copy bond property data.
			propertyObject->modifiableStorage()->mappedCopy(*bprop, mapping);
		}
	}
}

/******************************************************************************
* Returns a vector with the input particle colors.
******************************************************************************/
std::vector<Color> ParticlesObject::inputParticleColors() const
{
	std::vector<Color> colors(elementCount());

	// Obtain the particle vis element.
	if(ParticlesVis* particleVis = visElement<ParticlesVis>()) {

		// Query particle colors from vis element.
		particleVis->particleColors(colors,
				getProperty(ParticlesObject::ColorProperty),
				getProperty(ParticlesObject::TypeProperty));

		return colors;
	}

	std::fill(colors.begin(), colors.end(), Color(1,1,1));
	return colors;
}

/******************************************************************************
* Returns a vector with the input bond colors.
******************************************************************************/
std::vector<Color> ParticlesObject::inputBondColors() const
{
	// Obtain the bonds vis element.
    if(bonds()) {
		if(BondsVis* bondsVis = bonds()->visElement<BondsVis>()) {

			// Additionally, look up the particles vis element.
			ParticlesVis* particleVis = visElement<ParticlesVis>();

			// Query half-bond colors from vis element.
			std::vector<ColorA> halfBondColors = bondsVis->halfBondColors(
					elementCount(), 
					bonds()->getProperty(BondsObject::TopologyProperty),
					bonds()->getProperty(BondsObject::ColorProperty),
					bonds()->getProperty(BondsObject::TypeProperty),
					bonds()->getProperty(BondsObject::SelectionProperty),
					nullptr, // No transparency needed here
					particleVis, 
					getProperty(ParticlesObject::ColorProperty),
					getProperty(ParticlesObject::TypeProperty));
			OVITO_ASSERT(bonds()->elementCount() * 2 == halfBondColors.size());

			// Map half-bond colors to full bond colors.
			std::vector<Color> colors(bonds()->elementCount());				
			auto ci = halfBondColors.cbegin();
			for(Color& co : colors) {
				co = Color(ci->r(), ci->g(), ci->b());
				ci += 2;
			}
			return colors;
		}
    	return std::vector<Color>(bonds()->elementCount(), Color(1,1,1));
    }
	return {};
}

/******************************************************************************
* Returns a vector with the input particle radii.
******************************************************************************/
std::vector<FloatType> ParticlesObject::inputParticleRadii() const
{
	std::vector<FloatType> radii(elementCount());

	// Obtain the particle vis element.
	if(ParticlesVis* particleVis = visElement<ParticlesVis>()) {

		// Query particle radii from vis element.
		particleVis->particleRadii(radii,
				getProperty(ParticlesObject::RadiusProperty),
				getProperty(ParticlesObject::TypeProperty));

		return radii;
	}

	std::fill(radii.begin(), radii.end(), FloatType(1));
	return radii;
}


/******************************************************************************
* Gives the property class the opportunity to set up a newly created 
* property object.
******************************************************************************/
void ParticlesObject::OOMetaClass::prepareNewProperty(PropertyObject* property) const
{
	if(property->type() == ParticlesObject::DisplacementProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Displacements"));
		if(!Application::instance()->scriptMode())
			vis->loadUserDefaults();
		vis->setEnabled(false);
		property->addVisElement(vis);
	}

	else if(property->type() == ParticlesObject::ElasticDisplacementProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Elastic Displacements"));
		if(!Application::instance()->scriptMode())
			vis->loadUserDefaults();
		vis->setEnabled(false);
		property->addVisElement(vis);
	}
	else if(property->type() == ParticlesObject::ForceProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Forces"));
		if(!Application::instance()->scriptMode())
			vis->loadUserDefaults();
		vis->setEnabled(false);
		vis->setReverseArrowDirection(false);
		vis->setArrowPosition(VectorVis::Base);
		property->addVisElement(vis);
	}
	else if(property->type() == ParticlesObject::DipoleOrientationProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Dipoles"));
		if(!Application::instance()->scriptMode())
			vis->loadUserDefaults();
		vis->setEnabled(false);
		vis->setReverseArrowDirection(false);
		vis->setArrowPosition(VectorVis::Center);
		property->addVisElement(vis);
	}
}

/******************************************************************************
* Creates a storage object for standard particle properties.
******************************************************************************/
PropertyPtr ParticlesObject::OOMetaClass::createStandardStorage(size_t particleCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case TypeProperty:
	case StructureTypeProperty:
	case SelectionProperty:
	case CoordinationProperty:
	case MoleculeTypeProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case IdentifierProperty:
	case ClusterProperty:
	case MoleculeProperty:
		dataType = PropertyStorage::Int64;
		componentCount = 1;
		stride = sizeof(qlonglong);
		break;
	case PositionProperty:
	case DisplacementProperty:
	case ElasticDisplacementProperty:
	case VelocityProperty:
	case ForceProperty:
	case DipoleOrientationProperty:
	case AngularVelocityProperty:
	case AngularMomentumProperty:
	case TorqueProperty:
	case AsphericalShapeProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = sizeof(Vector3);
		OVITO_ASSERT(stride == sizeof(Point3));
		break;
	case ColorProperty:
	case VectorColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	case PotentialEnergyProperty:
	case KineticEnergyProperty:
	case TotalEnergyProperty:
	case RadiusProperty:
	case MassProperty:
	case ChargeProperty:
	case TransparencyProperty:
	case SpinProperty:
	case DipoleMagnitudeProperty:
	case CentroSymmetryProperty:
	case DisplacementMagnitudeProperty:
	case ElasticDisplacementMagnitudeProperty:
	case VelocityMagnitudeProperty:
		dataType = PropertyStorage::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case StressTensorProperty:
	case StrainTensorProperty:
	case ElasticStrainTensorProperty:
	case StretchTensorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 6;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(SymmetricTensor2));
		break;
	case DeformationGradientProperty:
	case ElasticDeformationGradientProperty:
		dataType = PropertyStorage::Float;
		componentCount = 9;
		stride = componentCount * sizeof(FloatType);
		break;
	case OrientationProperty:
	case RotationProperty:
		dataType = PropertyStorage::Float;
		componentCount = 4;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Quaternion));
		break;
	case PeriodicImageProperty:
		dataType = PropertyStorage::Int;
		componentCount = 3;
		stride = componentCount * sizeof(int);
        break;
    case WallaceTensorProperty:
        dataType = PropertyStorage::Float;
        componentCount = 21;
        stride = componentCount + sizeof(FloatType);
        break;
    case ElasticStabilityParameterProperty:
        dataType = PropertyStorage::Float;
        break;
	default:
		OVITO_ASSERT_MSG(false, "ParticlesObject::createStandardStorage()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	// Allocate the storage array.
	PropertyPtr property = std::make_shared<PropertyStorage>(particleCount, dataType, componentCount, stride, 
								propertyName, false, type, componentNames);

	// Initialize memory if requested.
	if(initializeMemory && !containerPath.empty()) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath.back())) {
				const std::vector<Color>& colors = particles->inputParticleColors();
				OVITO_ASSERT(colors.size() == property->size());
				std::copy(colors.cbegin(), colors.cend(), property->dataColor());
				initializeMemory = false;
			}
		}
		else if(type == RadiusProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath.back())) {
				const std::vector<FloatType>& radii = particles->inputParticleRadii();
				OVITO_ASSERT(radii.size() == property->size());
				std::copy(radii.cbegin(), radii.cend(), property->dataFloat());
				initializeMemory = false;
			}
		}
		else if(type == VectorColorProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath.back())) {
				for(const PropertyObject* p : particles->properties()) {
					if(VectorVis* vectorVis = dynamic_object_cast<VectorVis>(p->visElement())) {
						std::fill(property->dataColor(), property->dataColor() + property->size(), vectorVis->arrowColor());
						initializeMemory = false;
						break;
					}
				}
			}
		}
	}

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		std::memset(property->data(), 0, property->size() * property->stride());
	}

	return property;
}
	
/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void ParticlesObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a ParticlePropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<ParticlePropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, ParticlePropertyReference>();
	
	setPropertyClassDisplayName(tr("Particles"));
	setElementDescriptionName(QStringLiteral("particles"));
	setPythonName(QStringLiteral("particles"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";
	const QStringList symmetricTensorList = QStringList() << "XX" << "YY" << "ZZ" << "XY" << "XZ" << "YZ";
	const QStringList tensorList = QStringList() << "XX" << "YX" << "ZX" << "XY" << "YY" << "ZY" << "XZ" << "YZ" << "ZZ";
	const QStringList quaternionList = QStringList() << "X" << "Y" << "Z" << "W";
	
	registerStandardProperty(TypeProperty, tr("Particle Type"), PropertyStorage::Int, emptyList, tr("Particle types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ClusterProperty, tr("Cluster"), PropertyStorage::Int64, emptyList);
	registerStandardProperty(CoordinationProperty, tr("Coordination"), PropertyStorage::Int, emptyList);
	registerStandardProperty(PositionProperty, tr("Position"), PropertyStorage::Float, xyzList, tr("Particle positions"));
	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Particle colors"));
	registerStandardProperty(DisplacementProperty, tr("Displacement"), PropertyStorage::Float, xyzList, tr("Displacements"));
	registerStandardProperty(DisplacementMagnitudeProperty, tr("Displacement Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(ElasticDisplacementProperty, tr("Elastic Displacement"), PropertyStorage::Float, xyzList, tr("Elastic Displacements"));
	registerStandardProperty(ElasticDisplacementMagnitudeProperty, tr("Elastic Displacement Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(VelocityProperty, tr("Velocity"), PropertyStorage::Float, xyzList, tr("Velocities"));
	registerStandardProperty(PotentialEnergyProperty, tr("Potential Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(KineticEnergyProperty, tr("Kinetic Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(TotalEnergyProperty, tr("Total Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(RadiusProperty, tr("Radius"), PropertyStorage::Float, emptyList, tr("Radii"));
	registerStandardProperty(StructureTypeProperty, tr("Structure Type"), PropertyStorage::Int, emptyList, tr("Structure types"));
	registerStandardProperty(IdentifierProperty, tr("Particle Identifier"), PropertyStorage::Int64, emptyList, tr("Particle identifiers"));
	registerStandardProperty(StressTensorProperty, tr("Stress Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(StrainTensorProperty, tr("Strain Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(DeformationGradientProperty, tr("Deformation Gradient"), PropertyStorage::Float, tensorList);
	registerStandardProperty(OrientationProperty, tr("Orientation"), PropertyStorage::Float, quaternionList);
	registerStandardProperty(ForceProperty, tr("Force"), PropertyStorage::Float, xyzList);
	registerStandardProperty(MassProperty, tr("Mass"), PropertyStorage::Float, emptyList);
	registerStandardProperty(ChargeProperty, tr("Charge"), PropertyStorage::Float, emptyList);
	registerStandardProperty(PeriodicImageProperty, tr("Periodic Image"), PropertyStorage::Int, xyzList);
	registerStandardProperty(TransparencyProperty, tr("Transparency"), PropertyStorage::Float, emptyList);
	registerStandardProperty(DipoleOrientationProperty, tr("Dipole Orientation"), PropertyStorage::Float, xyzList);
	registerStandardProperty(DipoleMagnitudeProperty, tr("Dipole Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(AngularVelocityProperty, tr("Angular Velocity"), PropertyStorage::Float, xyzList);
	registerStandardProperty(AngularMomentumProperty, tr("Angular Momentum"), PropertyStorage::Float, xyzList);
	registerStandardProperty(TorqueProperty, tr("Torque"), PropertyStorage::Float, xyzList);
	registerStandardProperty(SpinProperty, tr("Spin"), PropertyStorage::Float, emptyList);
	registerStandardProperty(CentroSymmetryProperty, tr("Centrosymmetry"), PropertyStorage::Float, emptyList);
	registerStandardProperty(VelocityMagnitudeProperty, tr("Velocity Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(MoleculeProperty, tr("Molecule Identifier"), PropertyStorage::Int64, emptyList);
	registerStandardProperty(AsphericalShapeProperty, tr("Aspherical Shape"), PropertyStorage::Float, xyzList);
	registerStandardProperty(VectorColorProperty, tr("Vector Color"), PropertyStorage::Float, rgbList, tr("Vector colors"));
	registerStandardProperty(ElasticStrainTensorProperty, tr("Elastic Strain"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(ElasticDeformationGradientProperty, tr("Elastic Deformation Gradient"), PropertyStorage::Float, tensorList);
	registerStandardProperty(RotationProperty, tr("Rotation"), PropertyStorage::Float, quaternionList);
	registerStandardProperty(StretchTensorProperty, tr("Stretch Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(MoleculeTypeProperty, tr("Molecule Type"), PropertyStorage::Float, emptyList, tr("Molecule types"));
    registerStandardProperty(WallaceTensorProperty, tr("Wallace Tensor"), PropertyStorage::Float, tensorList);
    registerStandardProperty(ElasticStabilityParameterProperty, tr("Elastic Stability Parameter"), PropertyStorage::Float, emptyList);
}

/******************************************************************************
* Returns the index of the element that was picked in a viewport.
******************************************************************************/
std::pair<size_t, ConstDataObjectPath> ParticlesObject::OOMetaClass::elementFromPickResult(const ViewportPickResult& pickResult) const
{
	// Check if a particle was picked.
	if(const ParticlePickInfo* pickInfo = dynamic_object_cast<ParticlePickInfo>(pickResult.pickInfo())) {
		if(const ParticlesObject* particles = pickInfo->pipelineState().getObject<ParticlesObject>()) {
			size_t particleIndex = pickInfo->particleIndexFromSubObjectID(pickResult.subobjectId());
			if(particleIndex < particles->elementCount())
				return std::make_pair(particleIndex, ConstDataObjectPath({particles}));
		}
	}

	return std::pair<size_t, ConstDataObjectPath>(std::numeric_limits<size_t>::max(), {});
}

/******************************************************************************
* Tries to remap an index from one property container to another, considering the 
* possibility that elements may have been added or removed. 
******************************************************************************/
size_t ParticlesObject::OOMetaClass::remapElementIndex(const ConstDataObjectPath& source, size_t elementIndex, const ConstDataObjectPath& dest) const
{
	const ParticlesObject* sourceParticles = static_object_cast<ParticlesObject>(source.back());
	const ParticlesObject* destParticles = static_object_cast<ParticlesObject>(dest.back());

	// If unique IDs are available try to use them to look up the particle in the other data collection. 
	if(const PropertyObject* sourceIdentifiers = sourceParticles->getProperty(ParticlesObject::IdentifierProperty)) {
		if(const PropertyObject* destIdentifiers = destParticles->getProperty(ParticlesObject::IdentifierProperty)) {
			qlonglong id = sourceIdentifiers->getInt64(elementIndex);
			size_t mappedId = std::find(destIdentifiers->constDataInt64(), destIdentifiers->constDataInt64() + destIdentifiers->size(), id) - destIdentifiers->constDataInt64();
			if(mappedId != destIdentifiers->size())
				return mappedId;
		}
	}

	// Next, try to use the position to find the right particle in the other data collection. 
	if(const PropertyObject* sourcePositions = sourceParticles->getProperty(ParticlesObject::PositionProperty)) {
		if(const PropertyObject* destPositions = destParticles->getProperty(ParticlesObject::PositionProperty)) {
			const Point3& pos = sourcePositions->getPoint3(elementIndex);
			size_t mappedId = std::find(destPositions->constDataPoint3(), destPositions->constDataPoint3() + destPositions->size(), pos) - destPositions->constDataPoint3();
			if(mappedId != destPositions->size())
				return mappedId;
		}
	}

	// Give up.
	return PropertyContainerClass::remapElementIndex(source, elementIndex, dest);
}

/******************************************************************************
* Determines which elements are located within the given 
* viewport fence region (=2D polygon).
******************************************************************************/
boost::dynamic_bitset<> ParticlesObject::OOMetaClass::viewportFenceSelection(const QVector<Point2>& fence, const ConstDataObjectPath& objectPath, PipelineSceneNode* node, const Matrix4& projectionTM) const
{
	const ParticlesObject* particles = static_object_cast<ParticlesObject>(objectPath.back());
	if(const PropertyObject* posProperty = particles->getProperty(ParticlesObject::PositionProperty)) {

		if(!posProperty->visElement() || posProperty->visElement()->isEnabled() == false)
			node->throwException(tr("Cannot select particles while the corresponding visual element is disabled. Please enable the display of particles first."));

		boost::dynamic_bitset<> fullSelection(posProperty->size());
		QMutex mutex;
		parallelForChunks(posProperty->size(), [pos = posProperty->constDataPoint3(), &projectionTM, &fence, &mutex, &fullSelection](size_t startIndex, size_t chunkSize) {
			boost::dynamic_bitset<> selection(fullSelection.size());
			auto p = pos + startIndex;
			for(size_t index = startIndex; chunkSize != 0; chunkSize--, index++, ++p) {

				// Project particle center to screen coordinates.
				Point3 projPos = projectionTM * (*p);

				// Perform z-clipping.
				if(std::abs(projPos.z()) >= FloatType(1))
					continue;

				// Perform point-in-polygon test.
				int intersectionsLeft = 0;
				int intersectionsRight = 0;
				for(auto p2 = fence.constBegin(), p1 = p2 + (fence.size()-1); p2 != fence.constEnd(); p1 = p2++) {
					if(p1->y() == p2->y()) continue;
					if(projPos.y() >= p1->y() && projPos.y() >= p2->y()) continue;
					if(projPos.y() < p1->y() && projPos.y() < p2->y()) continue;
					FloatType xint = (projPos.y() - p2->y()) / (p1->y() - p2->y()) * (p1->x() - p2->x()) + p2->x();
					if(xint >= projPos.x())
						intersectionsRight++;
					else
						intersectionsLeft++;
				}
				if(intersectionsRight & 1)
					selection.set(index);
			}
			// Transfer thread-local results to output bit array.
			QMutexLocker locker(&mutex);
			fullSelection |= selection;
		});
		
		return fullSelection;
	}

	// Give up.
	return PropertyContainerClass::viewportFenceSelection(fence, objectPath, node, projectionTM);
}

}	// End of namespace
}	// End of namespace
