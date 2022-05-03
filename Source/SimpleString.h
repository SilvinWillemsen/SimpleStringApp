/*
  ==============================================================================

    SimpleString.h
    Created: 12 Feb 2021 1:10:03pm
    Author:  Silvin Willemsen

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SimpleString  : public juce::Component
{
public:
    SimpleString (NamedValueSet& parameters, double k);
    ~SimpleString() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    // function to draw the state of the string
    Path visualiseState (Graphics& g, double visualScaling);

    void calculateScheme();
    void updateStates();
    
    //return u at the current sample at a location given by the length ratio

    double getOutput (double Lratio)
    {
        return u[1][static_cast<int> (round(N * Lratio))];
    }
    
    void excite();
    void mouseDown (const MouseEvent& e) override;
    
    bool shouldExcite() { return excitationFlag; };
    
private:
    
    // Model parameters
    double L, rho, A, T, E, I, cSq, kappaSq, sigma0, sigma1, lambdaSq, muSq, h, k;
    
    // Number of intervals (N+1 is number of points including boundaries)
    int N;
    
    // An (N+1) x 3 'matrix' containing the state of the system at all time-steps
    std::vector<std::vector<double>> uStates;
    
    // vector of pointers that point to state vectors
    std::vector<double*> u;
    
    /* Scheme variables
        - Adiv for u^{n+1} (that all terms get divided by)
        - B for u^n
        - C for u^{n-1}
        - S for precalculated sigma terms
    */
    double Adiv, B0, Bss, B1, B2, C0, C1, S0, S1;
    
    // flag to tell MainComponent whether to excite the scheme or not
    bool excitationFlag = false;
    
    // initialise location of excitation
    double excitationLoc = 0.5;
    
    bool clamped = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleString)
};
