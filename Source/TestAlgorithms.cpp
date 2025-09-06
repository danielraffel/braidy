// Test harness to verify all 48 algorithms produce unique sounds
#include "../JuceLibraryCode/JuceHeader.h"
#include "BraidyCore/MacroOscillator.h"
#include "BraidyCore/BraidySettings.h"
#include "BraidyCore/BraidyResources.h"
#include <vector>
#include <cmath>

class AlgorithmTester : public juce::Component, public juce::Timer
{
public:
    AlgorithmTester()
    {
        // Initialize resources
        braidy::BraidyResources::Init();
        
        // Initialize settings
        settings_.Init();
        
        // Initialize oscillator
        oscillator_.Init();
        oscillator_.set_pitch(60 << 7); // Middle C
        
        // Start cycling through algorithms
        currentAlgorithm_ = 0;
        startTimerHz(2); // Change algorithm every 500ms
        
        setSize(600, 400);
    }
    
    ~AlgorithmTester() override
    {
        stopTimer();
    }
    
    void timerCallback() override
    {
        // Cycle through algorithms
        currentAlgorithm_ = (currentAlgorithm_ + 1) % 48;
        
        // Set the algorithm
        auto shape = static_cast<braidy::MacroOscillatorShape>(currentAlgorithm_);
        oscillator_.set_shape(shape);
        
        // Generate a short buffer of audio to test
        int16_t buffer[128];
        uint8_t sync[128] = {0};
        
        // Render audio with the current algorithm
        oscillator_.Render(sync, buffer, 128);
        
        // Calculate RMS to verify audio is being generated
        float rms = 0.0f;
        for (int i = 0; i < 128; ++i)
        {
            float sample = static_cast<float>(buffer[i]) / 32768.0f;
            rms += sample * sample;
        }
        rms = std::sqrt(rms / 128.0f);
        
        // Store RMS for this algorithm
        if (algorithmRMS_.size() <= currentAlgorithm_)
        {
            algorithmRMS_.resize(currentAlgorithm_ + 1);
        }
        algorithmRMS_[currentAlgorithm_] = rms;
        
        // Log the result
        DBG("Algorithm " << currentAlgorithm_ << " (" << getAlgorithmName(currentAlgorithm_) 
            << ") - RMS: " << rms);
        
        // After testing all algorithms, analyze uniqueness
        if (currentAlgorithm_ == 47)
        {
            analyzeUniqueness();
        }
        
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        
        // Display current algorithm
        juce::String text = "Testing Algorithm: " + juce::String(currentAlgorithm_) + "/47\n";
        text += getAlgorithmName(currentAlgorithm_);
        
        g.drawMultiLineText(text, 10, 30, getWidth() - 20);
        
        // Draw RMS values as bars
        if (!algorithmRMS_.empty())
        {
            float barWidth = static_cast<float>(getWidth() - 20) / 48.0f;
            for (size_t i = 0; i < algorithmRMS_.size(); ++i)
            {
                float height = algorithmRMS_[i] * 200.0f;
                
                // Color code by algorithm type
                if (i < 17) // Analog algorithms
                    g.setColour(juce::Colours::green);
                else // Digital algorithms
                    g.setColour(juce::Colours::cyan);
                    
                if (i == currentAlgorithm_)
                    g.setColour(juce::Colours::yellow);
                    
                g.fillRect(10.0f + i * barWidth, 
                          getHeight() - height - 50.0f,
                          barWidth - 1.0f, 
                          height);
            }
        }
        
        // Show analysis results if complete
        if (!analysisResults_.isEmpty())
        {
            g.setColour(juce::Colours::yellow);
            g.drawMultiLineText(analysisResults_, 10, 100, getWidth() - 20);
        }
    }
    
private:
    juce::String getAlgorithmName(int index)
    {
        const char* names[] = {
            "CSAW", "MORPH", "SAW_SQUARE", "SINE_TRIANGLE", "BUZZ",
            "SQUARE_SUB", "SAW_SUB", "SQUARE_SYNC", "SAW_SYNC",
            "TRIPLE_SAW", "TRIPLE_SQUARE", "TRIPLE_TRIANGLE", "TRIPLE_SINE",
            "TRIPLE_RING_MOD", "SAW_SWARM", "SAW_COMB", "TOY",
            "DIGITAL_FILTER_LP", "DIGITAL_FILTER_PK", "DIGITAL_FILTER_BP", "DIGITAL_FILTER_HP",
            "VOSIM", "VOWEL", "VOWEL_FOF", "FM", "FEEDBACK_FM",
            "CHAOTIC_FEEDBACK_FM", "PLUCKED", "BOWED", "BLOWN", "FLUTED",
            "STRUCK_BELL", "STRUCK_DRUM", "KICK", "CYMBAL", "SNARE",
            "WAVETABLES", "WAVE_MAP", "WAVE_LINE", "WAVE_PARAPHONIC",
            "FILTERED_NOISE", "TWIN_PEAKS_NOISE", "CLOCKED_NOISE", "GRANULAR_CLOUD",
            "PARTICLE_NOISE", "DIGITAL_MODULATION", "QUESTION_MARK"
        };
        
        if (index >= 0 && index < 48)
            return names[index];
        return "UNKNOWN";
    }
    
    void analyzeUniqueness()
    {
        analysisResults_ = "\n=== ALGORITHM UNIQUENESS ANALYSIS ===\n";
        
        // Check for silent algorithms
        int silentCount = 0;
        for (size_t i = 0; i < algorithmRMS_.size(); ++i)
        {
            if (algorithmRMS_[i] < 0.001f)
            {
                silentCount++;
                analysisResults_ += "Algorithm " + juce::String(i) + " is SILENT!\n";
            }
        }
        
        // Check for duplicate RMS values (potential identical algorithms)
        int duplicates = 0;
        for (size_t i = 0; i < algorithmRMS_.size(); ++i)
        {
            for (size_t j = i + 1; j < algorithmRMS_.size(); ++j)
            {
                float diff = std::abs(algorithmRMS_[i] - algorithmRMS_[j]);
                if (diff < 0.001f)
                {
                    duplicates++;
                    analysisResults_ += "Algorithms " + juce::String(i) + " and " + 
                                       juce::String(j) + " have similar RMS\n";
                }
            }
        }
        
        analysisResults_ += "\nSilent algorithms: " + juce::String(silentCount) + "\n";
        analysisResults_ += "Potential duplicates: " + juce::String(duplicates) + "\n";
        
        if (silentCount == 0 && duplicates == 0)
        {
            analysisResults_ += "\n✓ All algorithms produce unique output!\n";
        }
        else
        {
            analysisResults_ += "\n✗ Issues detected with algorithm uniqueness\n";
        }
        
        DBG(analysisResults_);
    }
    
    braidy::MacroOscillator oscillator_;
    braidy::Settings settings_;
    int currentAlgorithm_;
    std::vector<float> algorithmRMS_;
    juce::String analysisResults_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmTester)
};

// Create a simple test application
class TestApplication : public juce::JUCEApplication
{
public:
    TestApplication() {}
    
    const juce::String getApplicationName() override { return "Algorithm Tester"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    
    void initialise(const juce::String&) override
    {
        mainWindow.reset(new MainWindow("Algorithm Tester", new AlgorithmTester()));
    }
    
    void shutdown() override
    {
        mainWindow = nullptr;
    }
    
private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(const juce::String& name, juce::Component* component)
            : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setContentOwned(component, true);
            setResizable(true, false);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }
        
        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };
    
    std::unique_ptr<MainWindow> mainWindow;
};

// Register the application
START_JUCE_APPLICATION(TestApplication)