#include <iostream>

// #include "opencv2/core/core.hpp"
// #include "opencv2/highgui/highgui.hpp"
// #include "opencv2/imgproc.hpp"

#include "opencv2/opencv.hpp"

#include "Mtcnn.h"

#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "MQTTClient.h"


#define ADDRESS     "127.0.0.1"
#define CLIENTID    "xyz"

using namespace std;
using namespace cv;


void publish(MQTTClient client, char* topic, char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen((char *)(pubmsg.payload));
    pubmsg.qos = 2;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
    printf("Message '%s' with delivery token %d delivered\n", payload, token);
}

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payload = (char*) (message->payload);
    printf("Received operation %s\n", payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void PlotDetectionResult(Mat& frame, const std::vector<SMtcnnFace>& bbox)
{
    for (auto it = bbox.begin(); it != bbox.end(); it++)
    {
        // Plot bounding box
        rectangle(frame, Point(it->boundingBox[0], it->boundingBox[1]), 
            Point(it->boundingBox[2], it->boundingBox[3]), Scalar(0, 0, 255), 2, 8, 0);

        // Plot facial landmark
        for (int num = 0; num < 5; num++)
        {
            circle(frame, Point(it->landmark[num], it->landmark[num + 5]), 3, Scalar(0, 255, 255), -1);
        }
    }
}



int main(int argc, char** argv)
{
    char msg_buf[64];
    MQTTClient client;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    // conn_opts.username = "";
    // conn_opts.password = "";

    MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);

    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    //listen for operation
    // MQTTClient_subscribe(client, "bus", 0);

    // MQTTClient_subscribe(client, "bus", 0);
    publish(client, "bus", "[0]");

    VideoCapture cap(0);

    if (!cap.isOpened())
    {
        cout << "video is not open" << endl;
        return -1;
    }

    Mat frame;
    CMtcnn mtcnn;
    bool bSetParamToMtcnn = false;
    mtcnn.LoadModel("det1.param", "det1.bin", "det2.param", "det2.bin", "det3.param", "det3.bin");

    double sumMs = 0;
    int count = 0;


    while (1)
    {
        cap >> frame;
        std::vector<SMtcnnFace> finalBbox;

        if (!bSetParamToMtcnn && frame.cols > 0)
        {
            SImageFormat format(frame.cols, frame.rows, eBGR888);
            const float faceScoreThreshold[3] = { 0.6f, 0.6f, 0.6f };
            mtcnn.SetParam(format, 90, 0.709, -1, faceScoreThreshold);
            bSetParamToMtcnn = true;
        }

        double t1 = (double)getTickCount();
        mtcnn.Detect(frame.data, finalBbox);
        double t2 = (double)getTickCount();
        double t = 1000 * double(t2 - t1) / getTickFrequency();
        sumMs += t;
        ++count;
        cout << "faces = " << finalBbox.size() << ", time = " << t << " ms, FPS = " << 1000 / t << ", Average time = " << sumMs / count <<endl;

        if (finalBbox.size() > 0) {
            string msg = "[" + to_string(finalBbox.size());
            for (auto it = finalBbox.begin(); it != finalBbox.end(); it++)
            {
                for (int i=0; i<4; i++) {
                    msg += "," + to_string(it->boundingBox[i]); 
                }
            }
            msg += "]";

            char* c_msg = strdup(msg.c_str());
            publish(client, "bus", c_msg);
            free(c_msg);

        } else {
            publish(client, "bus", "[0]");
        }

        PlotDetectionResult(frame, finalBbox);

        // imshow("frame", frame);
        imwrite("/home/pi/camera.jpg", frame);

        if (waitKey(1) == 'q')
            break;
    }

    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);

    return 0;
}

