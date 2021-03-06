/*---------------------------------------------------------------------------*\
dynLocalAverageSmagorinsky - Implementation of the dynamic Smagorinsky
    			     SGS model.
    
Copyright Information
    Copyright (C) 1991-2009 OpenCFD Ltd.
    Copyright (C) 2010 Alberto Passalacqua 
    
License
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "dynLocalAverageSmagorinsky.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace incompressible
{
namespace LESModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(dynLocalAverageSmagorinsky, 0);
addToRunTimeSelectionTable(LESModel, dynLocalAverageSmagorinsky, dictionary);

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void dynLocalAverageSmagorinsky::updateSubGridScaleFields(const volSymmTensorField& D)
{
    // The SGS viscosity is bounded so that nuEff cannot become negative.
    // Values are limited here, and not in nuEff, for consistency in stored
    // data and in submodels using nuSgs().
    // No warning message is printed when this limitation is applied.
    nuSgs_ = max(cD(D)*sqr(delta())*sqrt(magSqr(D)), -nu());
    nuSgs_.correctBoundaryConditions();
}

volScalarField dynLocalAverageSmagorinsky::cD(const volSymmTensorField& D) const
{
    volSymmTensorField LL = dev(filter_(sqr(U())) - (sqr(filter_(U()))));

    volSymmTensorField MM =
        sqr(delta())*(filter_(mag(D)*(D)) - 4*mag(filter_(D))*filter_(D));

    // Locally averaging MMMM on cell faces
    volScalarField MMMM = fvc::average(magSqr(MM));
    
    MMMM.max(VSMALL);
    
    volScalarField LLMM = LL && MM;
    
    // Performing local average on cell faces on return
    return fvc::average(LLMM)/MMMM;
}


volScalarField dynLocalAverageSmagorinsky::cI(const volSymmTensorField& D) const
{
    volScalarField KK = 0.5*(filter_(magSqr(U())) - magSqr(filter_(U())));

    volScalarField mm =
        sqr(delta())*(4*sqr(mag(filter_(D))) - filter_(sqr(mag(D))));

    // Locally averaging mmmm on cell faces
    volScalarField mmmm = fvc::average(magSqr(mm));
    
    mmmm.max(VSMALL);
    
    volScalarField KKmm = KK*mm;    
    
    // Performing local average on cell faces on return
    return fvc::average(KKmm)/mmmm;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

dynLocalAverageSmagorinsky::dynLocalAverageSmagorinsky
(
    const volVectorField& U,
    const surfaceScalarField& phi,
    transportModel& transport
)
:
    LESModel(typeName, U, phi, transport),
    GenEddyVisc(U, phi, transport),

    k_
    (
        IOobject
        (
            "k",
            runTime_.timeName(),
            mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_
    ),

    filterPtr_(LESfilter::New(U.mesh(), coeffDict())),
    filter_(filterPtr_())
{
    updateSubGridScaleFields(dev(symm(fvc::grad(U))));

    printCoeffs();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void dynLocalAverageSmagorinsky::correct(const tmp<volTensorField>& gradU)
{
    LESModel::correct(gradU);

    volSymmTensorField D = dev(symm(gradU));

    k_ = cI(D)*sqr(delta())*magSqr(D);

    updateSubGridScaleFields(D);
}

bool dynLocalAverageSmagorinsky::read()
{
    if (GenEddyVisc::read())
    {
        filter_.read(coeffDict());

        return true;
    }
    else
    {
        return false;
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace LESModels
} // End namespace incompressible
} // End namespace Foam

// ************************************************************************* //
