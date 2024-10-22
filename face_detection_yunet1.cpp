#include <opencv2/opencv.hpp>
#include <map>
#include <vector>
#include <string>
#include <iostream>

const std::map<std::string, int> str2backend{
    {"opencv", cv::dnn::DNN_BACKEND_OPENCV}, {"cuda", cv::dnn::DNN_BACKEND_CUDA},
    {"timvx",  cv::dnn::DNN_BACKEND_TIMVX},  {"cann", cv::dnn::DNN_BACKEND_CANN}
};

const std::map<std::string, int> str2target{
    {"cpu", cv::dnn::DNN_TARGET_CPU}, {"cuda", cv::dnn::DNN_TARGET_CUDA},
    {"npu", cv::dnn::DNN_TARGET_NPU}, {"cuda_fp16", cv::dnn::DNN_TARGET_CUDA_FP16}
};

class YuNet {
public:
    YuNet(const std::string& model_path,
          const cv::Size& input_size = cv::Size(320, 320),
          float conf_threshold = 0.6f,
          float nms_threshold = 0.3f,
          int top_k = 5000,
          int backend_id = 0,
          int target_id = 0)
        : model_path_(model_path), input_size_(input_size),
          conf_threshold_(conf_threshold), nms_threshold_(nms_threshold),
          top_k_(top_k), backend_id_(backend_id), target_id_(target_id)
    {
        model = cv::FaceDetectorYN::create(model_path_, "", input_size_, conf_threshold_, nms_threshold_, top_k_, backend_id_, target_id_);
    }

    void setInputSize(const cv::Size& input_size) {
        input_size_ = input_size;
        model->setInputSize(input_size_);
    }

    cv::Mat infer(const cv::Mat image) {
        cv::Mat res;
        model->detect(image, res);
        return res;
    }

private:
    cv::Ptr<cv::FaceDetectorYN> model;
    std::string model_path_;
    cv::Size input_size_;
    float conf_threshold_;
    float nms_threshold_;
    int top_k_;
    int backend_id_;
    int target_id_;
};

cv::Mat visualize(const cv::Mat& image, const cv::Mat& faces, float fps = -1.f) {
    static cv::Scalar box_color{0, 255, 0};
    static std::vector<cv::Scalar> landmark_color{
        cv::Scalar(255,   0,   0), // right eye
        cv::Scalar(  0,   0, 255), // left eye
        cv::Scalar(  0, 255,   0), // nose tip
        cv::Scalar(255,   0, 255), // right mouth corner
        cv::Scalar(  0, 255, 255)  // left mouth corner
    };
    
    auto output_image = image.clone();

    if (fps >= 0) {
        cv::putText(output_image, cv::format("FPS: %.2f", fps), 
                    cv::Point(0, 15), 
                    cv::FONT_HERSHEY_SIMPLEX, 
                    0.5, box_color);
    }

    for (int i = 0; i < faces.rows; ++i) {
        // Draw bounding boxes
        int x1 = static_cast<int>(faces.at<float>(i, 0));
        int y1 = static_cast<int>(faces.at<float>(i, 1));
        int w = static_cast<int>(faces.at<float>(i, 2));
        int h = static_cast<int>(faces.at<float>(i, 3));
        
        // Use the correct namespace for Rect
        cv::rectangle(output_image, cv::Rect(x1, y1, w, h), box_color);

        // Confidence as text
        float conf = faces.at<float>(i, 14);
        // Use the correct namespace for format and Point
        cv::putText(output_image,
                    cv::format("%.4f", conf),
                    cv::Point(x1 + (w / 2), y1 - 10),
                    cv::FONT_HERSHEY_DUPLEX,
                    0.5,
                    box_color);

        // Draw landmarks
        for (int j = 0; j < landmark_color.size(); ++j) {
            int x = static_cast<int>(faces.at<float>(i, 2 * j + 4)), 
                y = static_cast<int>(faces.at<float>(i, 2 * j + 5));
            // Use the correct namespace for Point
            cv::circle(output_image, cv::Point(x,y), /* radius */2 , landmark_color[j], /* thickness */-1);
        }
    }
    
    return output_image;
}

int main(int argc, char** argv) {
    
   // Command line parser setup...
   cv::CommandLineParser parser(argc, argv,
       "{help h           |                                   | Print this message}"
       "{input i           | /home/itstarkenn/Downloads/VID_337361108_114019_366.mp4                              | Set input to a certain image}"
       "{model m           | /home/itstarkenn/Downloads/face_detection_yunet_2023mar.onnx | Set path to the model}"
       "{backend b         | opencv                            | Set DNN backend}"
       "{target t          | cpu                               | Set DNN target}"
       "{save s            | false                             | Whether to save result image or not}"
       "{vis v             | true                             | Whether to visualize result image or not}"
       "{conf_threshold     | 0.9                               | Minimum confidence for face detection}"
       "{nms_threshold      | 0.3                               | NMS threshold for overlapping boxes}"
       "{top_k             | 5000                              | Keep top_k bounding boxes before NMS}");

   if (parser.has("help")) {
       parser.printMessage();
       return 0;
   }

   std::string input_path = parser.get<std::string>("input");
   std::string model_path = parser.get<std::string>("model");
   std::string backend = parser.get<std::string>("backend");
   std::string target = parser.get<std::string>("target");
   bool save_flag = parser.get<bool>("save");
   bool vis_flag = parser.get<bool>("vis");

   // Model parameters
   float conf_threshold = parser.get<float>("conf_threshold");
   float nms_threshold = parser.get<float>("nms_threshold");
   int top_k = parser.get<int>("top_k");
   
   const int backend_id = str2backend.at(backend);
   const int target_id = str2target.at(target);

   // Instantiate YuNet
   YuNet model(model_path, cv::Size(320, 320), conf_threshold, nms_threshold, top_k, backend_id, target_id);

   // If input is an image
   if (!input_path.empty()) {
       auto image = cv::imread(input_path);

       if (image.empty()) {
           std::cerr << "Error: Could not open or find the image at " << input_path << std::endl;
           return -1;
       }

       // Inference
       model.setInputSize(image.size());
       auto faces = model.infer(image);

       // Print detected faces info...
        // Print faces
        std::cout << cv::format("%d faces detected:\n", faces.rows);
        for (int i = 0; i < faces.rows; ++i)
        {
            int x1 = static_cast<int>(faces.at<float>(i, 0));
            int y1 = static_cast<int>(faces.at<float>(i, 1));
            int w = static_cast<int>(faces.at<float>(i, 2));
            int h = static_cast<int>(faces.at<float>(i, 3));
            float conf = faces.at<float>(i, 14);
            std::cout << cv::format("%d: x1=%d, y1=%d, w=%d, h=%d, conf=%.4f\n", i, x1, y1, w, h, conf);
        }
       // Draw results on the input image
       auto res_image = visualize(image, faces);
       cv::imwrite("Output_IMgggg.jpeg", res_image); 
       // Save result image if required
       
       if (save_flag) {
           cv::imwrite("Output_IMg.jpeg", res_image); // Save output as JPEG
           std::cout << "Results saved to Output_IMg.jpeg\n";
       }
        
       // Visualize result if required
       if (vis_flag) {
           // Use the correct namespace for namedWindow and imshow
           cv::namedWindow("Face Detection Result", cv::WINDOW_AUTOSIZE);
           cv::imshow("Face Detection Result", res_image); // Show output image in a window
           cv::waitKey(0); // Wait for a key press before closing window
       }
       
   } else // Call default camera
    {
        int device_id = 0;
        auto cap = cv::VideoCapture(device_id);
        int w = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int h = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        model.setInputSize(cv::Size(w, h));

        auto tick_meter = cv::TickMeter();
        cv::Mat frame;
        while (cv::waitKey(1) < 0)
        {
            bool has_frame = cap.read(frame);
            if (!has_frame)
            {
                std::cout << "No frames grabbed! Exiting ...\n";
                break;
            }

            // Inference
            tick_meter.start();
            cv::Mat faces = model.infer(frame);
            tick_meter.stop();

            // Draw results on the input image
            auto res_image = visualize(frame, faces, (float)tick_meter.getFPS());
            // Visualize in a new window
            cv::imshow("YuNet Demo", res_image);

            tick_meter.reset();
        }
    }

    return 0;
}
