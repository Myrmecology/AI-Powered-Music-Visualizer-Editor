#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QOpenGLWidget>
#include <QTimer>
#include <QLabel>
#include <QVector3D>
#include <QProcess>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QFile>
#include <QElapsedTimer>
#include <iostream>
#include <cmath>

// AnalysisClient class with improved resource management
class AnalysisClient : public QObject {
    Q_OBJECT
    
public:
    struct AnalysisResult {
        bool success = false;
        QString error_message;
        float duration = 0.0f;
        int sample_rate = 44100;
        float tempo = 0.0f;
        QVector<float> beat_times;
        QVector<float> waveform;
        QString predicted_mood;
        float mood_confidence = 0.0f;
    };
    
    AnalysisClient(QObject* parent = nullptr) : QObject(parent) {
        process = new QProcess(this);
        
        // Set up the Python path - try to find the venv Python
        QString projectDir = QApplication::applicationDirPath() + "/..";
        QString venvPython = projectDir + "/venv/Scripts/python.exe";
        
        // Check if venv Python exists, otherwise use system Python
        if (QFile::exists(venvPython)) {
            pythonExecutable = venvPython;
            qDebug() << "Using venv Python:" << pythonExecutable;
        } else {
            pythonExecutable = "python";
            qDebug() << "Using system Python:" << pythonExecutable;
        }
    }
    
    // Added destructor for proper cleanup
    ~AnalysisClient() {
        cleanupProcesses();
    }
    
    void analyzeFile(const QString& filePath) {
        // Clean up any existing processes first
        cleanupProcesses();
        
        emit analysisStarted();
        
        // Set working directory to project root
        QString projectDir = QApplication::applicationDirPath() + "/..";
        process->setWorkingDirectory(projectDir);
        
        // Prepare Python command
        QStringList arguments;
        arguments << "main.py" << "analyze" << filePath;
        
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, filePath](int exitCode, QProcess::ExitStatus exitStatus) {
                    Q_UNUSED(exitStatus)
                    qDebug() << "Analysis process finished with exit code:" << exitCode;
                    
                    AnalysisResult result;
                    if (exitCode == 0) {
                        // Parse the output
                        QString output = process->readAllStandardOutput();
                        parseAnalysisOutput(output, result);
                        
                        // Now run mood classification
                        runMoodClassification(filePath);
                    } else {
                        result.success = false;
                        QString stderrOutput = process->readAllStandardError();
                        result.error_message = QString("Analysis failed (exit code %1): %2")
                                               .arg(exitCode).arg(stderrOutput);
                        qDebug() << "Analysis error:" << result.error_message;
                        emit analysisCompleted(result);
                    }
                });
        
        qDebug() << "Running command:" << pythonExecutable << arguments.join(" ");
        qDebug() << "Working directory:" << process->workingDirectory();
        
        process->start(pythonExecutable, arguments);
    }
    
    // Added cleanup function
    void cleanupProcesses() {
        if (process && process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
        
        // Clean up any lingering mood classification processes
        for (auto* proc : findChildren<QProcess*>()) {
            if (proc != process && proc->state() != QProcess::NotRunning) {
                proc->kill();
                proc->waitForFinished(500);
            }
        }
    }
    
signals:
    void analysisStarted();
    void analysisCompleted(const AnalysisResult& result);
    
private:
    QString pythonExecutable;
    QProcess* process;
    AnalysisResult tempResult;
    
    void parseAnalysisOutput(const QString& output, AnalysisResult& result) {
        result.success = true;
        
        // Parse duration
        QRegularExpression durationRegex("Duration: ([\\d\\.]+) seconds");
        QRegularExpressionMatch match = durationRegex.match(output);
        if (match.hasMatch()) {
            result.duration = match.captured(1).toFloat();
        }
        
        // Parse tempo
        QRegularExpression tempoRegex("Tempo: ([\\d\\.]+) BPM");
        match = tempoRegex.match(output);
        if (match.hasMatch()) {
            result.tempo = match.captured(1).toFloat();
        }
        
        // Store this result temporarily
        tempResult = result;
    }
    
    void runMoodClassification(const QString& filePath) {
        // Create a new process for mood classification
        QProcess* moodProcess = new QProcess(this);
        QString projectDir = QApplication::applicationDirPath() + "/..";
        moodProcess->setWorkingDirectory(projectDir);
        
        QStringList arguments;
        arguments << "main.py" << "classify" << filePath;
        
        connect(moodProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, moodProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                    Q_UNUSED(exitStatus)
                    
                    if (exitCode == 0) {
                        QString output = moodProcess->readAllStandardOutput();
                        parseMoodOutput(output, tempResult);
                    } else {
                        // Set default mood if classification fails
                        tempResult.predicted_mood = "energetic";
                        tempResult.mood_confidence = 0.5f;
                    }
                    
                    emit analysisCompleted(tempResult);
                    
                    // Ensure cleanup after completion
                    moodProcess->deleteLater();
                });
        
        moodProcess->start(pythonExecutable, arguments);
    }
    
    void parseMoodOutput(const QString& output, AnalysisResult& result) {
        // Parse predicted mood
        QRegularExpression moodRegex("Predicted mood: (\\w+)");
        QRegularExpressionMatch match = moodRegex.match(output);
        if (match.hasMatch()) {
            result.predicted_mood = match.captured(1);
        }
        
        // Parse confidence
        QRegularExpression confidenceRegex("Confidence: ([\\d\\.]+)");
        match = confidenceRegex.match(output);
        if (match.hasMatch()) {
            result.mood_confidence = match.captured(1).toFloat();
        }
    }
};

class VisualizerWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    VisualizerWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        // Setup timer for animation
        animationTimer = new QTimer(this);
        connect(animationTimer, &QTimer::timeout, this, &VisualizerWidget::animate);
        animationTimer->start(16); // ~60 FPS
        
        // Initialize animation time
        animationTime.start();
        
        // Initialize default values
        beatIntensity = 0.0f;
        moodColor = QVector3D(0.0f, 1.0f, 0.5f); // Default green
        isPlaying = false;
        isAnalyzed = false;
        currentTempo = 120.0f;
        frameCount = 0; // Added for performance monitoring
    }
    
    void setMoodColor(const QVector3D& color) {
        moodColor = color;
    }
    
    void setAnalysisData(const AnalysisClient::AnalysisResult& result) {
        if (result.success) {
            isAnalyzed = true;
            currentTempo = result.tempo;
            currentDuration = result.duration;
            
            // Set mood color based on detected mood
            if (result.predicted_mood == "happy") {
                setMoodColor(QVector3D(1.0f, 0.7f, 0.0f));
            } else if (result.predicted_mood == "sad") {
                setMoodColor(QVector3D(0.2f, 0.3f, 0.8f));
            } else if (result.predicted_mood == "energetic") {
                setMoodColor(QVector3D(1.0f, 0.0f, 0.3f));
            } else if (result.predicted_mood == "calm") {
                setMoodColor(QVector3D(0.3f, 0.8f, 0.5f));
            } else if (result.predicted_mood == "angry") {
                setMoodColor(QVector3D(0.9f, 0.1f, 0.1f));
            }
        }
    }
    
    void startAnimation() {
        isPlaying = true;
        beatIntensity = 1.0f;
        animationTime.restart();
        frameCount = 0; // Reset frame counter
        
        // Reset animation timer to prevent accumulation of timing errors
        animationTimer->stop();
        animationTimer->start(16);
    }
    
    void stopAnimation() {
        isPlaying = false;
        // Optional: force a final update to clear any remaining artifacts
        update();
    }
    
    void setPlaybackProgress(qint64 position, qint64 duration) {
        if (duration > 0) {
            float progress = (float)position / duration;
            // Could be used for more precise sync
        }
    }
    
    // Added function to reset visualization state
    void resetVisualization() {
        isPlaying = false;
        isAnalyzed = false;
        beatIntensity = 0.0f;
        currentTempo = 120.0f;
        frameCount = 0;
        animationTime.restart();
        update();
    }

protected:
    void initializeGL() override {
        // Initialize OpenGL functions
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
    }

    void paintGL() override {
        // Update background color based on mood
        glClearColor(moodColor.x() * 0.1f, moodColor.y() * 0.1f, moodColor.z() * 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (isPlaying) {
            // Draw visualizations
            drawWaveform();
            drawBeatIndicator();
            drawFrequencyBars();
            drawMoodParticles();
        }
        
        // Increment frame counter for performance monitoring
        frameCount++;
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

private slots:
    void animate() {
        if (isPlaying) {
            // Update beat intensity decay
            beatIntensity *= 0.98f;
            
            // Use actual tempo if analyzed, otherwise default
            float time = animationTime.elapsed() / 1000.0f;
            float beatInterval = 60.0f / currentTempo; // Convert BPM to seconds per beat
            
            // Trigger beat based on actual tempo
            if (fmod(time, beatInterval) < 0.1f && beatIntensity < 0.5f) {
                beatIntensity = 1.0f;
            }
            
            // Periodically reset timer to prevent drift (every 30 seconds)
            if (frameCount % (30 * 60) == 0) {
                animationTime.restart();
                frameCount = 0;
            }
        }
        
        update(); // Triggers paintGL
    }

private:
    QTimer *animationTimer;
    QElapsedTimer animationTime;
    float beatIntensity;
    QVector3D moodColor;
    bool isPlaying;
    bool isAnalyzed;
    float currentTempo;
    float currentDuration;
    qint64 frameCount; // Added for performance monitoring
    
    void drawWaveform() {
        // Enhanced waveform based on analysis
        float time = animationTime.elapsed() / 1000.0f;
        float tempoMultiplier = currentTempo / 120.0f; // Normalize to 120 BPM
        
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        glColor4f(moodColor.x(), moodColor.y(), moodColor.z(), 0.8f);
        
        for (float x = -1.0f; x <= 1.0f; x += 0.01f) {
            float y = 0.3f * sin(x * 10.0f + time * 3.0f * tempoMultiplier) * (1.0f + beatIntensity * 0.5f);
            glVertex2f(x, y);
        }
        
        glEnd();
    }
    
    void drawBeatIndicator() {
        if (beatIntensity > 0.1f) {
            float radius = 0.05f + 0.1f * beatIntensity;
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
        // Enhanced frequency visualization
        int numBars = 32;
        float barWidth = 2.0f / numBars;
        float time = animationTime.elapsed() / 1000.0f;
        float tempoMultiplier = currentTempo / 120.0f;
        
        for (int i = 0; i < numBars; ++i) {
            float intensity = 0.3f + 0.5f * sin(i * 0.3f + time * 2.0f * tempoMultiplier);
            intensity *= (1.0f + beatIntensity * 0.5f);
            
            float x = -1.0f + i * barWidth;
            float height = intensity * 0.6f;
            
            // Color based on frequency (blue to red across spectrum)
            float colorPhase = (float)i / numBars;
            glBegin(GL_QUADS);
            glColor4f(colorPhase * moodColor.x(), (1.0f - colorPhase) * moodColor.y(), moodColor.z(), 0.7f);
            glVertex2f(x, -0.8f);
            glVertex2f(x + barWidth * 0.8f, -0.8f);
            glVertex2f(x + barWidth * 0.8f, -0.8f + height);
            glVertex2f(x, -0.8f + height);
            glEnd();
        }
    }
    
    void drawMoodParticles() {
        // Enhanced particle system
        int numParticles = 50;
        float time = animationTime.elapsed() * 0.001f;
        
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        
        for (int i = 0; i < numParticles; ++i) {
            float t = time + i * 0.1f;
            float x = sin(t * 0.5f + i) * 0.8f;
            float y = sin(t * 0.3f + i * 2.0f) * 0.8f;
            float alpha = (0.5f + 0.5f * sin(t * 2.0f + i)) * beatIntensity;
            
            glColor4f(moodColor.x(), moodColor.y(), moodColor.z(), alpha * 0.5f);
            glVertex2f(x, y);
        }
        
        glEnd();
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("AI Music Visualizer");
        setMinimumSize(800, 600);
        
        // Initialize audio components - keeping the original working implementation
        mediaPlayer = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        mediaPlayer->setAudioOutput(audioOutput);
        
        // Connect media player signals
        connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::updatePosition);
        connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::updateDuration);
        
        // Initialize analysis client
        analysisClient = new AnalysisClient(this);
        connect(analysisClient, &AnalysisClient::analysisCompleted,
                this, &MainWindow::onAnalysisCompleted);
        
        // Create central widget and layout
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        
        // Create visualizer widget
        visualizer = new VisualizerWidget(this);
        mainLayout->addWidget(visualizer);
        
        // Create control panel
        QWidget *controlPanel = new QWidget(this);
        QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
        
        // Status label
        statusLabel = new QLabel("Ready to visualize music", this);
        controlLayout->addWidget(statusLabel);
        
        // Button layout
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        
        QPushButton *loadButton = new QPushButton("Load Audio", this);
        QPushButton *analyzeButton = new QPushButton("Analyze", this);
        QPushButton *playButton = new QPushButton("Play", this);
        QPushButton *stopButton = new QPushButton("Stop", this);
        QPushButton *refreshButton = new QPushButton("Refresh", this); // Added refresh button
        QPushButton *exportButton = new QPushButton("Export Video", this);
        
        analyzeButton->setEnabled(false);
        
        connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadAudioFile);
        connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::analyzeAudio);
        connect(playButton, &QPushButton::clicked, this, &MainWindow::playAudio);
        connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopAudio);
        connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshApplication);
        connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportVideo);
        
        buttonLayout->addWidget(loadButton);
        buttonLayout->addWidget(analyzeButton);
        buttonLayout->addWidget(playButton);
        buttonLayout->addWidget(stopButton);
        buttonLayout->addWidget(refreshButton);
        buttonLayout->addWidget(exportButton);
        
        // Audio controls
        QHBoxLayout *audioLayout = new QHBoxLayout();
        QLabel *volumeLabel = new QLabel("Volume:", this);
        volumeSlider = new QSlider(Qt::Horizontal, this);
        volumeSlider->setRange(0, 100);
        volumeSlider->setValue(70);
        connect(volumeSlider, &QSlider::valueChanged, this, &MainWindow::setVolume);
        
        audioLayout->addWidget(volumeLabel);
        audioLayout->addWidget(volumeSlider);
        
        controlLayout->addLayout(audioLayout);
        controlLayout->addLayout(buttonLayout);
        
        // Mood color controls
        QHBoxLayout *moodLayout = new QHBoxLayout();
        QLabel *moodLabel = new QLabel("Manual Mood Override:", this);
        moodLayout->addWidget(moodLabel);
        
        QPushButton *happyButton = new QPushButton("Happy", this);
        QPushButton *sadButton = new QPushButton("Sad", this);
        QPushButton *energeticButton = new QPushButton("Energetic", this);
        QPushButton *calmButton = new QPushButton("Calm", this);
        QPushButton *angryButton = new QPushButton("Angry", this);
        
        connect(happyButton, &QPushButton::clicked, [this]() { setMood("happy"); });
        connect(sadButton, &QPushButton::clicked, [this]() { setMood("sad"); });
        connect(energeticButton, &QPushButton::clicked, [this]() { setMood("energetic"); });
        connect(calmButton, &QPushButton::clicked, [this]() { setMood("calm"); });
        connect(angryButton, &QPushButton::clicked, [this]() { setMood("angry"); });
        
        moodLayout->addWidget(happyButton);
        moodLayout->addWidget(sadButton);
        moodLayout->addWidget(energeticButton);
        moodLayout->addWidget(calmButton);
        moodLayout->addWidget(angryButton);
        
        controlLayout->addLayout(moodLayout);
        
        mainLayout->addWidget(controlPanel);
        setCentralWidget(centralWidget);
        
        this->analyzeButton = analyzeButton;
        this->refreshButton = refreshButton;
    }
    
    // Added destructor for cleanup
    ~MainWindow() {
        stopAudio();
        if (analysisClient) {
            analysisClient->cleanupProcesses();
        }
    }

private slots:
    void loadAudioFile() {
        QString fileName = QFileDialog::getOpenFileName(this, 
            tr("Open Audio File"), "", tr("Audio Files (*.wav *.mp3)"));
        
        if (!fileName.isEmpty()) {
            statusLabel->setText(QString("Loaded: %1").arg(QFileInfo(fileName).baseName()));
            currentFile = fileName;
            analyzeButton->setEnabled(true);
            
            // Load audio file into media player
            mediaPlayer->setSource(QUrl::fromLocalFile(fileName));
            
            std::cout << "Loading audio file: " << fileName.toStdString() << std::endl;
        }
    }
    
    void analyzeAudio() {
        if (!currentFile.isEmpty()) {
            statusLabel->setText("Analyzing audio...");
            analyzeButton->setEnabled(false);
            analysisClient->analyzeFile(currentFile);
        }
    }
    
    void onAnalysisCompleted(const AnalysisClient::AnalysisResult& result) {
        analyzeButton->setEnabled(true);
        
        if (result.success) {
            statusLabel->setText(QString("Analysis complete - Tempo: %1 BPM, Mood: %2")
                                .arg(result.tempo)
                                .arg(result.predicted_mood));
            
            visualizer->setAnalysisData(result);
        } else {
            statusLabel->setText("Analysis failed: " + result.error_message);
        }
    }
    
    void playAudio() {
        if (!currentFile.isEmpty()) {
            statusLabel->setText("Playing audio and visualization...");
            visualizer->startAnimation();
            mediaPlayer->play();
            std::cout << "Starting audio playback and visualization..." << std::endl;
        } else {
            statusLabel->setText("Please load an audio file first");
        }
    }
    
    void stopAudio() {
        statusLabel->setText("Stopped");
        visualizer->stopAnimation();
        mediaPlayer->stop();
        std::cout << "Stopping audio and visualization..." << std::endl;
    }
    
    // Added refresh function to reset everything
    void refreshApplication() {
        // Stop everything
        stopAudio();
        
        // Clean up processes
        analysisClient->cleanupProcesses();
        
        // Reset visualizer
        visualizer->resetVisualization();
        
        // Reset media player
        mediaPlayer->stop();
        mediaPlayer->setSource(QUrl());
        
        // Reset UI state
        analyzeButton->setEnabled(false);
        currentFile.clear();
        statusLabel->setText("Ready to visualize music - Application refreshed");
        
        qDebug() << "Application refreshed - all resources cleaned up";
    }
    
    void exportVideo() {
        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Export Video"), "", tr("Video Files (*.mp4)"));
        
        if (!fileName.isEmpty()) {
            statusLabel->setText("Exporting video...");
            std::cout << "Exporting video to: " << fileName.toStdString() << std::endl;
            // TODO: Implement video export
        }
    }
    
    void setMood(const QString& mood) {
        QVector3D color;
        if (mood == "happy") color = QVector3D(1.0f, 0.7f, 0.0f);
        else if (mood == "sad") color = QVector3D(0.2f, 0.3f, 0.8f);
        else if (mood == "energetic") color = QVector3D(1.0f, 0.0f, 0.3f);
        else if (mood == "calm") color = QVector3D(0.3f, 0.8f, 0.5f);
        else if (mood == "angry") color = QVector3D(0.9f, 0.1f, 0.1f);
        
        visualizer->setMoodColor(color);
        statusLabel->setText(QString("Manual mood override: %1").arg(mood));
    }
    
    void setVolume(int value) {
        audioOutput->setVolume(value / 100.0f);
    }
    
    void updatePosition(qint64 position) {
        visualizer->setPlaybackProgress(position, mediaPlayer->duration());
    }
    
    void updateDuration(qint64 duration) {
        Q_UNUSED(duration)
        // Could add a progress bar here if desired
    }

private:
    VisualizerWidget *visualizer;
    QLabel *statusLabel;
    QString currentFile;
    AnalysisClient *analysisClient;
    QPushButton *analyzeButton;
    QPushButton *refreshButton; // Added refresh button reference
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    QSlider *volumeSlider;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "main.moc"