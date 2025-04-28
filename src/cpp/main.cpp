#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QOpenGLWidget>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <iostream>
#include <cmath>
#include "analyzer_client.h"

class VisualizerWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    VisualizerWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        // Setup timer for animation
        animationTimer = new QTimer(this);
        connect(animationTimer, &QTimer::timeout, this, &VisualizerWidget::animate);
        animationTimer->start(16); // ~60 FPS
        
        // Initialize default values
        currentBeatTime = 0.0f;
        lastBeatTime = 0.0f;
        beatIntensity = 0.0f;
        moodColor = QVector3D(1.0f, 1.0f, 1.0f); // Default white
    }
    
    void setAnalysisData(const AnalysisResult& result) {
        if (result.success) {
            waveformData = result.waveform;
            beatTimes = result.beat_times;
            tempo = result.tempo;
            
            // Set mood color based on prediction
            QVector3D newColor = getMoodColor(result.predicted_mood);
            moodColor = newColor;
        }
    }
    
    void updateRealTimeData(const std::vector<float>& waveform, float rms) {
        realtimeWaveform = waveform;
        currentRMS = rms;
    }

protected:
    void initializeGL() override {
        // Initialize OpenGL functions
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void paintGL() override {
        // Update background color based on mood
        glClearColor(moodColor.x() * 0.2f, moodColor.y() * 0.2f, moodColor.z() * 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Draw visualizations
        drawWaveform();
        drawBeatIndicator();
        drawFrequencyBars();
        drawMoodParticles();
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

private slots:
    void animate() {
        // Update beat intensity decay
        beatIntensity *= 0.95f;
        
        // Check for new beat
        float currentTime = animationTime.elapsed() / 1000.0f;
        for (float beatTime : beatTimes) {
            if (beatTime > lastBeatTime && beatTime <= currentTime) {
                beatIntensity = 1.0f;
                lastBeatTime = beatTime;
            }
        }
        
        update(); // Triggers paintGL
    }

private:
    QTimer *animationTimer;
    QElapsedTimer animationTime;
    std::vector<float> waveformData;
    std::vector<float> realtimeWaveform;
    std::vector<float> beatTimes;
    float tempo;
    float currentBeatTime;
    float lastBeatTime;
    float beatIntensity;
    float currentRMS;
    QVector3D moodColor;
    
    void drawWaveform() {
        const std::vector<float>& data = realtimeWaveform.empty() ? waveformData : realtimeWaveform;
        if (data.empty()) return;
        
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        glColor4f(0.0f, 1.0f, 0.5f, 0.8f);
        
        for (size_t i = 0; i < data.size(); ++i) {
            float x = -1.0f + 2.0f * i / data.size();
            float y = data[i] * 0.5f;
            glVertex2f(x, y);
        }
        
        glEnd();
    }
    
    void drawBeatIndicator() {
        if (beatIntensity > 0.1f) {
            float radius = 0.1f * beatIntensity;
            int segments = 32;
            
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 1.0f, 1.0f, beatIntensity);
            glVertex2f(0.0f, 0.8f);
            
            for (int i = 0; i <= segments; ++i) {
                float angle = i * 2.0f * M_PI / segments;
                float x = radius * cos(angle);
                float y = 0.8f + radius * sin(angle);
                glVertex2f(x, y);
            }
            
            glEnd();
        }
    }
    
    void drawFrequencyBars() {
        // Simple frequency visualization
        int numBars = 32;
        float barWidth = 2.0f / numBars;
        
        for (int i = 0; i < numBars; ++i) {
            float intensity = 0.5f + 0.5f * sin(i * 0.5f + animationTime.elapsed() * 0.002f);
            intensity *= (1.0f + beatIntensity * 0.5f);
            
            float x = -1.0f + i * barWidth;
            float height = intensity * 0.4f;
            
            glBegin(GL_QUADS);
            glColor4f(moodColor.x(), moodColor.y(), moodColor.z(), 0.7f);
            glVertex2f(x, -0.8f);
            glVertex2f(x + barWidth * 0.8f, -0.8f);
            glVertex2f(x + barWidth * 0.8f, -0.8f + height);
            glVertex2f(x, -0.8f + height);
            glEnd();
        }
    }
    
    void drawMoodParticles() {
        // Mood-based particle system
        int numParticles = 50;
        float time = animationTime.elapsed() * 0.001f;
        
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        
        for (int i = 0; i < numParticles; ++i) {
            float t = time + i * 0.1f;
            float x = sin(t * 0.5f + i) * 0.8f;
            float y = sin(t * 0.3f + i * 2.0f) * 0.8f;
            float alpha = 0.5f + 0.5f * sin(t * 2.0f + i);
            
            glColor4f(moodColor.x(), moodColor.y(), moodColor.z(), alpha * 0.5f);
            glVertex2f(x, y);
        }
        
        glEnd();
    }
    
    QVector3D getMoodColor(const std::string& mood) {
        if (mood == "happy") return QVector3D(1.0f, 0.7f, 0.0f);
        if (mood == "sad") return QVector3D(0.2f, 0.3f, 0.8f);
        if (mood == "energetic") return QVector3D(1.0f, 0.0f, 0.3f);
        if (mood == "calm") return QVector3D(0.3f, 0.8f, 0.5f);
        if (mood == "angry") return QVector3D(0.9f, 0.1f, 0.1f);
        return QVector3D(1.0f, 1.0f, 1.0f); // Default white
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("AI Music Visualizer");
        
        // Start Python server
        startPythonServer();
        
        // Initialize analyzer client
        analyzerClient = new AnalyzerClient();
        
        // Create central widget and layout
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);
        
        // Create visualizer widget
        visualizer = new VisualizerWidget(this);
        layout->addWidget(visualizer);
        
        // Create control buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *loadButton = new QPushButton("Load Audio", this);
        QPushButton *micButton = new QPushButton("Start Mic", this);
        QPushButton *stopButton = new QPushButton("Stop", this);
        QPushButton *exportButton = new QPushButton("Export Video", this);
        
        connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadAudioFile);
        connect(micButton, &QPushButton::clicked, this, &MainWindow::startMicCapture);
        connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopCapture);
        connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportVideo);
        
        buttonLayout->addWidget(loadButton);
        buttonLayout->addWidget(micButton);
        buttonLayout->addWidget(stopButton);
        buttonLayout->addWidget(exportButton);
        
        layout->addLayout(buttonLayout);
        
        setCentralWidget(centralWidget);
        resize(800, 600);
    }
    
    ~MainWindow() {
        // Stop Python server
        if (pythonProcess) {
            pythonProcess->terminate();
            pythonProcess->waitForFinished();
            delete pythonProcess;
        }
        delete analyzerClient;
    }

private slots:
    void loadAudioFile() {
        QString fileName = QFileDialog::getOpenFileName(this, 
            tr("Open Audio File"), "", tr("Audio Files (*.wav *.mp3)"));
        
        if (!fileName.isEmpty()) {
            std::cout << "Loading audio file: " << fileName.toStdString() << std::endl;
            
            // Analyze the file using Python server
            AnalysisResult result = analyzerClient->analyzeFile(fileName.toStdString());
            
            if (result.success) {
                visualizer->setAnalysisData(result);
                std::cout << "Analysis complete. Tempo: " << result.tempo << " BPM" << std::endl;
                std::cout << "Mood: " << result.predicted_mood << " (confidence: " 
                          << result.mood_confidence << ")" << std::endl;
            } else {
                std::cerr << "Analysis failed: " << result.error_message << std::endl;
            }
        }
    }
    
    void startMicCapture() {
        std::cout << "Starting microphone capture..." << std::endl;
        // TODO: Implement mic capture with RtAudio
    }
    
    void stopCapture() {
        std::cout << "Stopping capture..." << std::endl;
        // TODO: Implement stop functionality
    }
    
    void exportVideo() {
        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Export Video"), "", tr("Video Files (*.mp4)"));
        
        if (!fileName.isEmpty()) {
            std::cout << "Exporting video to: " << fileName.toStdString() << std::endl;
            // TODO: Implement video export with FFmpeg
        }
    }

private:
    VisualizerWidget *visualizer;
    AnalyzerClient *analyzerClient;
    QProcess *pythonProcess;
    
    void startPythonServer() {
        pythonProcess = new QProcess(this);
        
        // Start Python server in a separate process
        QString pythonScript = "src/python/analysis_server.py";
        pythonProcess->start("python", QStringList() << pythonScript);
        
        if (!pythonProcess->waitForStarted()) {
            std::cerr << "Failed to start Python server" << std::endl;
        } else {
            std::cout << "Python analysis server started" << std::endl;
            // Give the server a moment to initialize
            QThread::msleep(1000);
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "main.moc"