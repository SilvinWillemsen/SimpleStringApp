#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (0, 2);
    }
}

MainComponent::~MainComponent()
{
    // Stop the graphics update
    stopTimer();

    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    //// Set the paramters ///
    NamedValueSet parameters;
    
    // parameters you'll use to initialise more than one other parameter should be defined here
    double r = 0.0005;
    
    parameters.set ("L", 1);
    parameters.set ("rho", 7850);
    parameters.set ("A", r * r * double_Pi);
    parameters.set ("T", 300);
    parameters.set ("E", 2e11);
    parameters.set ("I", r * r * r * r * double_Pi * 0.25);
    parameters.set ("sigma0", 1);
    parameters.set ("sigma1", 0.005);
    
    //// Initialise an instance of the SimpleString class ////
    mySimpleString = std::make_unique<SimpleString> (parameters, 1.0 / sampleRate);
    
    addAndMakeVisible (mySimpleString.get()); // add the string to the application
    
    // Call resized again as our components need a sample rate before they can get initialised.
    resized();
    
    startTimerHz (15); // start the timer (15 Hz is a nice tradeoff between CPU usage and update speed)
    
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    int numChannels = bufferToFill.buffer->getNumChannels();
    
    // Get pointers to output locations
    float* const channelData1 = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
    float* const channelData2 = numChannels > 1 ? bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample) : nullptr;

    float output = 0.0;

    std::vector<float* const*> curChannel {&channelData1, &channelData2};
    
    // only do control stuff out of the buffer (at least work with flags so that control doesn't interfere with the scheme calculation)
    if (mySimpleString->shouldExcite())
        mySimpleString->excite();
        
    for (int i = 0; i < bufferToFill.numSamples; ++i)
    {
        mySimpleString->calculateScheme();
        mySimpleString->updateStates();
        
        output = mySimpleString->getOutput (0.8); // get output at 0.8L of the string
        for (int channel = 0; channel < numChannels; ++channel)
            curChannel[channel][0][i] = limit(output);
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
}

void MainComponent::resized()
{
    // put the string in the application
    if (mySimpleString != nullptr)
        mySimpleString->setBounds (getLocalBounds());
}

// limiter
double MainComponent::limit (double val)
{
    if (val < -1)
    {
        val = -1;
        return val;
    }
    else if (val > 1)
    {
        val = 1;
        return val;
    }
    return val;
}


void MainComponent::timerCallback()
{
    repaint(); // update the graphics X times a second
}
