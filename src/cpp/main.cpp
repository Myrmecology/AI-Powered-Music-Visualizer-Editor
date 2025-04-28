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
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Basic visualization placeholder
        drawWaveform();
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

private slots:
    void animate() {
        update(); // Triggers paintGL
    }

private:
    QTimer *animationTimer;
    
    void drawWaveform() {
        // Placeholder for waveform visualization
        glBegin(GL_LINE_STRIP);
        glColor3f(0.0f, 1.0f, 0.0f);
        
        for (float x = -1.0f; x <= 1.0f; x += 0.01f) {
            float y = 0.5f * sin(x * 10.0f); // Simple sine wave for now
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
        
        // Create central widget and layout
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);
        
        // Create visualizer widget
        visualizer = new VisualizerWidget(this);
        layout->addWidget(visualizer);
        
        // Create control buttons
        QPushButton *loadButton = new QPushButton("Load Audio", this);
        QPushButton *micButton = new QPushButton("Start Mic", this);
        
        connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadAudioFile);
        connect(micButton, &QPushButton::clicked, this, &MainWindow::startMicCapture);
        
        layout->addWidget(loadButton);
        layout->addWidget(micButton);
        
        setCentralWidget(centralWidget);
        resize(800, 600);
    }

private slots:
    void loadAudioFile() {
        QString fileName = QFileDialog::getOpenFileName(this, 
            tr("Open Audio File"), "", tr("Audio Files (*.wav *.mp3)"));
        
        if (!fileName.isEmpty()) {
            std::cout << "Loading audio file: " << fileName.toStdString() << std::endl;
            // TODO: Implement audio loading
        }
    }
    
    void startMicCapture() {
        std::cout << "Starting microphone capture..." << std::endl;
        // TODO: Implement mic capture
    }

private:
    VisualizerWidget *visualizer;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "main.moc"