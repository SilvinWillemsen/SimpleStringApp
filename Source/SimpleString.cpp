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
SimpleString::SimpleString (NamedValueSet& parameters, double k) :
L (*parameters.getVarPointer ("L")),
rho (*parameters.getVarPointer ("rho")),
A (*parameters.getVarPointer ("A")),
T (*parameters.getVarPointer ("T")),
E (*parameters.getVarPointer ("E")),
I (*parameters.getVarPointer ("I")),
sigma0 (*parameters.getVarPointer ("sigma0")),
sigma1 (*parameters.getVarPointer ("sigma1")),
k (k)
{
    cSq = T / (rho * A);                // Calculate wave speed (squared)
    kappaSq = E * I / (rho * A);        // Calculate stiffness coefficient squared

    double stabilityTerm = cSq * k * k + 4.0 * sigma1 * k; // just easier to write down below
    
    h = sqrt (stabilityTerm + sqrt ((stabilityTerm * stabilityTerm) + 16.0 * kappaSq * k * k));
    N = floor (L / h);
    h = 1.0 / N; // recalculate h
    
    lambdaSq = cSq * k * k / (h * h);
    muSq = kappaSq * k * k / (h * h * h * h);
    
    // initialise vectors
    uStates.reserve (3); // prevents allocation errors
    
    for (int i = 0; i < 3; ++i)
        uStates.push_back (std::vector<double> (N+1, 0));
    
    u.resize (3);
    
    /*  Make u pointers point to the first index of the state vectors.
        To use u (and obtain a vector from the state vectors) use indices like u[n][l] where,
             - n = 0 is u^{n+1},
             - n = 1 is u^n, and
             - n = 2 is u^{n-1}.
        Also see calculateScheme()
     */
    
    for (int i = 0; i < 3; ++i)
        u[i] = &uStates[i][0];
    
    // set coefficients for update equation
    B1 = sigma0 * k;
    B2 = (2.0 * sigma1 * k) / (h * h);
    
    D = 1.0 / (1.0 + sigma0 * k);
    
    A1 = 2.0 - 2.0 * lambdaSq - 6.0 * muSq - 2.0 * B2; // u_l^n
    A2 = lambdaSq + 4.0 * muSq + B2;                   // u_{l+-1}^n
    A3 = -muSq;                                        // u_{l+-2}^n
    A4 = B1 - 1.0 + 2.0 * B2;                          // u_l^{n-1}
    A5 = -B2;                                          // u_{l+-1}^{n-1}
    
    // Divide by u_l^{n+1} term
    A1 *= D;
    A2 *= D;
    A3 *= D;
    A4 *= D;
    A5 *= D;
}

SimpleString::~SimpleString()
{
}

void SimpleString::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour (juce::Colours::grey);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText ("SimpleString", getLocalBounds(),
                juce::Justification::centred, true);   // draw some placeholder text
}

void SimpleString::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void SimpleString::calculateScheme()
{
    for (int l = 2; l < N-1; ++l) // clamped boundaries
        u[0][l] = A1 * u[1][l] + A2 * (u[1][l + 1] + u[1][l - 1]) + A3 * (u[1][l + 2] + u[1][l - 2])
                + A4 * u[2][l] + A5 * (u[2][l + 1] + u[2][l - 1]);
    
}

void SimpleString::updateStates()
{
    double* uTmp = u[2];
    u[2] = u[1];
    u[1] = u[0];
    u[0] = uTmp;
}

void SimpleString::excite()
{
    // Arbitrary excitation function. Just used this for testing purposes
    
    double width = 10;
    double pos = 0.3;
    int start = std::max (floor((N+1) * pos) - floor(width * 0.5), 1.0);

    for (int l = 0; l < width; ++l)
    {
        // make sure we're not going out of bounds (under construction...)
        int idx = l+start;
//        if (idx > N)
//            break;
        u[1][idx] += 0.5 * (1 - cos(2.0 * double_Pi * l / (width-1.0)));
        u[2][idx] += 0.5 * (1 - cos(2.0 * double_Pi * l / (width-1.0)));
//        std::cout << l << std::endl;
    }

    // print excitation
    for (int l = 0; l <= N; ++l)
    {
        std::cout << u[1][l] << std::endl;
    }
}

void SimpleString::mouseDown (const MouseEvent& e)
{
    excite();
}
