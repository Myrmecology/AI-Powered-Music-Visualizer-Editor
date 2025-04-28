#ifndef ANALYZER_CLIENT_H
#define ANALYZER_CLIENT_H

#include <zmq.hpp>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct AnalysisResult {
    bool success;
    std::string error_message;
    float duration;
    int sample_rate;
    float tempo;
    std::vector<float> beat_times;
    std::vector<float> waveform;
    std::string predicted_mood;
    float mood_confidence;
    std::map<std::string, float> mood_probabilities;
};

class AnalyzerClient {
public:
    AnalyzerClient(const std::string& address = "tcp://localhost:5555") 
        : context_(1), socket_(context_, ZMQ_REQ) {
        socket_.connect(address);
    }
    
    ~AnalyzerClient() {
        socket_.close();
        context_.close();
    }
    
    AnalysisResult analyzeFile(const std::string& file_path) {
        json request = {
            {"command", "analyze_file"},
            {"file_path", file_path}
        };
        
        return sendRequest(request);
    }
    
    AnalysisResult analyzeChunk(const std::vector<float>& audio_data, int sample_rate) {
        json request = {
            {"command", "analyze_chunk"},
            {"audio_data", audio_data},
            {"sample_rate", sample_rate}
        };
        
        return sendRequest(request);
    }
    
private:
    zmq::context_t context_;
    zmq::socket_t socket_;
    
    AnalysisResult sendRequest(const json& request) {
        AnalysisResult result;
        
        try {
            // Send request
            std::string request_str = request.dump();
            zmq::message_t zmq_request(request_str.size());
            memcpy(zmq_request.data(), request_str.data(), request_str.size());
            socket_.send(zmq_request, zmq::send_flags::none);
            
            // Receive response
            zmq::message_t zmq_reply;
            socket_.recv(zmq_reply, zmq::recv_flags::none);
            
            std::string reply_str(static_cast<char*>(zmq_reply.data()), zmq_reply.size());
            json response = json::parse(reply_str);
            
            // Parse response
            if (response["status"] == "success") {
                result.success = true;
                auto data = response["data"];
                
                // Basic info
                result.duration = data.value("duration", 0.0f);
                result.sample_rate = data.value("sample_rate", 44100);
                
                // Beats
                if (data.contains("beats")) {
                    result.tempo = data["beats"].value("tempo", 0.0f);
                    result.beat_times = data["beats"]["beat_times"].get<std::vector<float>>();
                }
                
                // Waveform
                if (data.contains("waveform")) {
                    result.waveform = data["waveform"].get<std::vector<float>>();
                }
                
                // Mood
                if (data.contains("mood")) {
                    result.predicted_mood = data["mood"]["predicted_mood"];
                    result.mood_confidence = data["mood"]["confidence"];
                    result.mood_probabilities = data["mood"]["probabilities"].get<std::map<std::string, float>>();
                }
            } else {
                result.success = false;
                result.error_message = response.value("message", "Unknown error");
            }
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = e.what();
        }
        
        return result;
    }
};

#endif // ANALYZER_CLIENT_H