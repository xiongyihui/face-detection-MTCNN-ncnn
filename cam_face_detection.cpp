#include <ctime>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "raspicam/raspicam_cv.h"

#include "Mtcnn.h"

using namespace std;
using namespace cv;


int main(int argc, char **argv)
{
    time_t timer_begin, timer_end;
    raspicam::RaspiCam_Cv Camera;
    cv::Mat frame;

    CMtcnn mtcnn;
    bool bSetParamToMtcnn = false;
    mtcnn.LoadModel("det1.param", "det1.bin", "det2.param", "det2.bin", "det3.param", "det3.bin");

    int nCount = 100;
    //set camera params
//    Camera.set(CV_CAP_PROP_FORMAT, CV_8UC3);
    Camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

    //Open camera
    cout << "Opening Camera..." << endl;
    if (!Camera.open())
    {
        cerr << "Error opening the camera" << endl;
        return -1;
    }
    //Start capture

    double sumMs = 0;
    int count = 0;
    while (1)
    {
        double t1 = (double)getTickCount();
        Camera.grab();
        Camera.retrieve(frame);
        double t2 = (double)getTickCount();

        std::vector<SMtcnnFace> finalBbox;

        if (!bSetParamToMtcnn && frame.cols > 0)
        {
            cout << "width: " << frame.cols << ", height: " << frame.rows << endl;
            SImageFormat format(frame.cols, frame.rows, eBGR888);
            const float faceScoreThreshold[3] = {0.6f, 0.6f, 0.6f};
            mtcnn.SetParam(format, 90, 0.709, -1, faceScoreThreshold);
            bSetParamToMtcnn = true;
        }

        mtcnn.Detect(frame.data, finalBbox);
        if (finalBbox.size()) {
            cout << "find " << finalBbox.size() << endl;
        }

        double t3 = (double)getTickCount();

        double t = 1000 * double(t3 - t1) / getTickFrequency();
        sumMs += t;
        ++count;
        cout << "time = " << t << " ms, FPS = " << 1000 / t << ", Average time = " << sumMs / count <<endl;

        if (waitKey(1) == 'q')
            break;

//        imwrite("/home/pi/capture.jpg", frame);
    }

    cout << "Stop camera..." << endl;
    Camera.release();

    return 0;
}
