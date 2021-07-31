/*
  ==============================================================================

    SimpleString.cpp
    Created: 12 Feb 2021 1:10:03pm
    Author:  Silvin Willemsen

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SimpleString.h"

//==============================================================================
SimpleString::SimpleString (NamedValueSet& parameters, double k) : k (k)
{
    // Initialise member variables using the parameter set
    L = *parameters.getVarPointer ("L");
    rho = *parameters.getVarPointer ("rho");
    A = *parameters.getVarPointer ("A");
    T = *parameters.getVarPointer ("T");
    E = *parameters.getVarPointer ("E");
    I = *parameters.getVarPointer ("I");
    sigma0 = *parameters.getVarPointer ("sigma0");
    sigma1 = *parameters.getVarPointer ("sigma1");
    
    // Calculate wave speed (squared)
    cSq = T / (rho * A);
    
    // Calculate stiffness coefficient (squared)
    kappaSq = E * I / (rho * A);

    double stabilityTerm = cSq * k * k + 4.0 * sigma1 * k; // just easier to write down below
    
    h = sqrt (stabilityTerm + sqrt ((stabilityTerm * stabilityTerm) + 16.0 * kappaSq * k * k));
    N = floor (L / h);
    h = 1.0 / N; // recalculate h
    
    lambdaSq = cSq * k * k / (h * h);
    muSq = kappaSq * k * k / (h * h * h * h);
    
    // Initialise vectors
    uStates = std::vector<std::vector<double>> (3,
                                        std::vector<double>(N+1, 0));
    
    /*  Make u pointers point to the first index of the state vectors.
        To use u (and obtain a vector from the state vectors) use indices like u[n][l] where,
             - n = 0 is u^{n+1},
             - n = 1 is u^n, and
             - n = 2 is u^{n-1}.
        Also see calculateScheme()
     */
    
    // Initialise pointer vector
    u.resize (3, nullptr);
    
    // Make set memory addresses to first index of the state vectors.
    for (int i = 0; i < 3; ++i)
        u[i] = &uStates[i][0];
    
    // Coefficients used for damping
    S0 = sigma0 * k;
    S1 = (2.0 * sigma1 * k) / (h * h);
    
    // Scheme coefficients
    B0 = 2.0 - 2.0 * lambdaSq - 6.0 * muSq - 2.0 * S1; // u_l^n
    B1 = lambdaSq + 4.0 * muSq + S1;                   // u_{l+-1}^n
    B2 = -muSq;                                        // u_{l+-2}^n
    C0 = -1.0 + S0 + 2.0 * S1;                         // u_l^{n-1}
    C1 = -S1;                                          // u_{l+-1}^{n-1}
    
    Adiv = 1.0 / (1.0 + S0);                           // u_l^{n+1}
    
    // Divide by u_l^{n+1} term
    B0 *= Adiv;
    B1 *= Adiv;
    B2 *= Adiv;
    C0 *= Adiv;
    C1 *= Adiv;
}

SimpleString::~SimpleString()
{
    
}

void SimpleString::paint (juce::Graphics& g)
{
    // clear the background
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    // choose your favourite colour
    g.setColour(Colours::cyan);
    
    // draw the state
    g.strokePath(visualiseState (g, 100), PathStrokeType(2.0f));

}

Path SimpleString::visualiseState (Graphics& g, double visualScaling)
{
    // String-boundaries are in the vertical middle of the component
    double stringBoundaries = getHeight() / 2.0;
    
    // initialise path
    Path stringPath;
    
    // start path
    stringPath.startNewSubPath (0, -u[1][0] * visualScaling + stringBoundaries);
    
    double spacing = getWidth() / static_cast<double>(N);
    double x = spacing;
    
    for (int l = 1; l <= N; l++) // if you don't save the boundaries use l < N
    {
        // Needs to be -u, because a positive u would visually go down
        float newY = -u[1][l] * visualScaling + stringBoundaries;
        
        // if we get NAN values, make sure that we don't get an exception
        if (isnan(newY))
            newY = 0;
        
        stringPath.lineTo (x, newY);
        x += spacing;
    }
    // if you don't save the boundaries, and add a stringPath.lineTo (x, getWidth()) here to end the statedrawing

    return stringPath;
}

void SimpleString::resized()
{

}

void SimpleString::calculateScheme()
{
    for (int l = 2; l < N-1; ++l) // clamped boundaries
        u[0][l] = B0 * u[1][l] + B1 * (u[1][l + 1] + u[1][l - 1]) + B2 * (u[1][l + 2] + u[1][l - 2])
                + C0 * u[2][l] + C1 * (u[2][l + 1] + u[2][l - 1]);
}

void SimpleString::updateStates()
{
    // Do a pointer-switch. MUCH quicker than copying two entire state vectors every time-step.
    double* uTmp = u[2];
    u[2] = u[1];
    u[1] = u[0];
    u[0] = uTmp;
}

void SimpleString::excite()
{
    //// Arbitrary excitation function (raised cosine) ////
    
    // width (in grid points) of the excitation
    double width = 10;
    
    // make sure we're not going out of bounds at the left boundary
    int start = std::max (floor((N+1) * excitationLoc) - floor(width * 0.5), 1.0);

    for (int l = 0; l < width; ++l)
    {
        // make sure we're not going out of bounds at the right boundary (this does 'cut off' the raised cosine)
        if (l+start > (clamped ? N - 2 : N - 1))
            break;
        
        u[1][l+start] += 0.5 * (1 - cos(2.0 * double_Pi * l / (width-1.0)));
        u[2][l+start] += 0.5 * (1 - cos(2.0 * double_Pi * l / (width-1.0)));
    }
    // Disable the excitation flag to only excite once
    excitationFlag = false;
}

void SimpleString::mouseDown (const MouseEvent& e)
{
    // Get the excitation location as a ratio between the x-location of the mouse-click and the width of the app
    excitationLoc = e.x / static_cast<double> (getWidth());
    
    // Activate the excitation flag to be used by the MainComponent to excite the string
    excitationFlag = true;
}
