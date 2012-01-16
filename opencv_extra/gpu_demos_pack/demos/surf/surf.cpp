#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/gpu/gpu.hpp>

#include "utility_lib/utility_lib.h"

using namespace std;
using namespace cv;
using namespace cv::gpu;

class App : public BaseApp
{
public:
    App() : use_gpu(true), the_same_video_offset(1), match_confidence(0.5) {}

    virtual void run(int argc, char **argv);
    virtual bool processKey(int key);
    virtual void printHelp();
    virtual void parseCmdArgs(int argc, char **argv);

    bool use_gpu;
    int the_same_video_offset;
    double match_confidence;
};

void App::run(int argc, char **argv)
{
    parseCmdArgs(argc, argv);
    if (help_showed) 
        return;

    if (sources.size() == 1 && dynamic_cast<VideoSource*>(static_cast<FrameSource*>(sources[0])) != 0)
    {
        sources.push_back(new VideoSource(sources[0]->path()));
        Mat tmp; 
        for (int i = 0; i < the_same_video_offset; ++i)
            sources.back()->next(tmp);
    }
    else if (sources.size() != 2) 
    {
        cout << "Loading default images..." << endl;
        sources.resize(2);
        sources[0] = new ImageSource("data/matching/t34mA.jpg");
        sources[1] = new ImageSource("data/matching/t34mB.jpg");
    }

    cout << "\nControls:" << endl;
    cout << "  space - change CPU/GPU mode" << endl;
    cout << "  a/s - increase/decrease match confidence\n" << endl;

    Mat h_img1, h_img2, h_img1_gray, h_img2_gray;
    GpuMat d_img1_gray, d_img2_gray;

    SURF surf_cpu(1000);
    SURF_GPU surf_gpu(1000);

    vector<KeyPoint> keypoints1_cpu, keypoints2_cpu;
    vector<float> descriptors1_vec, descriptors2_vec;
    Mat descriptors1_cpu, descriptors2_cpu;
    GpuMat keypoints1_gpu, keypoints2_gpu;
    GpuMat descriptors1_gpu, descriptors2_gpu;

    BruteForceMatcher< L2<float> > matcher_cpu;
    BruteForceMatcher_GPU< L2<float> > matcher_gpu;
    GpuMat trainIdx, distance, allDist;
    vector< vector<DMatch> > matches;
    vector<DMatch> good_matches;

    double total_fps = 0;

    while (!exited)
    {
        int64 start = getTickCount();

        sources[0]->next(h_img1);
        sources[1]->next(h_img2);
        makeGray(h_img1, h_img1_gray);
        makeGray(h_img2, h_img2_gray);

        if (use_gpu)
        {
            d_img1_gray.upload(h_img1_gray);
            d_img2_gray.upload(h_img2_gray);
        }

        int64 proc_start = getTickCount();
        
        int64 surf_start = getTickCount();

        if (use_gpu)
        {
            surf_gpu(d_img1_gray, GpuMat(), keypoints1_gpu, descriptors1_gpu);
            surf_gpu(d_img2_gray, GpuMat(), keypoints2_gpu, descriptors2_gpu);
        }
        else
        {
            surf_cpu(h_img1_gray, Mat(), keypoints1_cpu, descriptors1_vec);
            surf_cpu(h_img2_gray, Mat(), keypoints2_cpu, descriptors2_vec);

            descriptors1_cpu = Mat(descriptors1_vec).reshape(0, keypoints1_cpu.size());
            descriptors2_cpu = Mat(descriptors2_vec).reshape(0, keypoints2_cpu.size());
        }

        double surf_fps = getTickFrequency()  / (getTickCount() - surf_start);
        
        int64 match_start = getTickCount();

        if (use_gpu)
        {
            matcher_gpu.knnMatchSingle(descriptors1_gpu, descriptors2_gpu, trainIdx, distance, allDist, 2);
        }
        else
        {
            matcher_cpu.knnMatch(descriptors1_cpu, descriptors2_cpu, matches, 2);
        }

        double match_fps = getTickFrequency()  / (getTickCount() - match_start);

        if (use_gpu)
        {
            matcher_gpu.knnMatchDownload(trainIdx, distance, matches);
        }

        good_matches.clear();
        good_matches.reserve(matches.size());

        for (size_t i = 0; i < matches.size(); ++i)
        {
            if (matches[i].size() < 2)
                continue;

            const DMatch &m1 = matches[i][0];
            const DMatch &m2 = matches[i][1];

            if (m1.distance < m2.distance * match_confidence)
                good_matches.push_back(m1);
        }

        double proc_fps = getTickFrequency()  / (getTickCount() - proc_start);

        if (use_gpu)
        {
            surf_gpu.downloadKeypoints(keypoints1_gpu, keypoints1_cpu);
            surf_gpu.downloadKeypoints(keypoints2_gpu, keypoints2_cpu);
        }

        theRNG() = RNG(0);

        Mat dst;
        drawMatches(h_img1, keypoints1_cpu, h_img2, keypoints2_cpu, good_matches, dst, Scalar(255, 0, 0, 255), Scalar(0, 0, 255, 255));

        stringstream msg; msg << "Total FPS : " << setprecision(4) << total_fps;
        putText(dst, msg.str(), Point(0, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar::all(255));
        msg.str(""); msg << "Processing FPS : " << setprecision(4) << proc_fps;
        putText(dst, msg.str(), Point(0, 60), FONT_HERSHEY_SIMPLEX, 1, Scalar::all(255));
        msg.str(""); msg << "SURF FPS : " << setprecision(4) << surf_fps;
        putText(dst, msg.str(), Point(0, 90), FONT_HERSHEY_SIMPLEX, 1, Scalar::all(255));
        msg.str(""); msg << "Match FPS : " << setprecision(4) << match_fps;
        putText(dst, msg.str(), Point(0, 120), FONT_HERSHEY_SIMPLEX, 1, Scalar::all(255));
        putText(dst, use_gpu ? "Mode : GPU" : "Mode : CPU", Point(0, 150), FONT_HERSHEY_SIMPLEX, 1, Scalar::all(255));
        
        imshow("surf_demo", dst);
        processKey(waitKey(3));

        total_fps = getTickFrequency()  / (getTickCount() - proc_start);
    }
}

bool App::processKey(int key)
{
    if (BaseApp::processKey(key))
        return true;

    switch (toupper(key))
    {
    case 32:
        use_gpu = !use_gpu;
        cout << "Use gpu = " << use_gpu << endl;
        break;
    case 'A':
        match_confidence += 0.1;
        match_confidence = min(match_confidence, 1.0);
        cout << "match_confidence = " << match_confidence << endl;
        break;
    case 'S':
        match_confidence -= 0.1;
        match_confidence = max(match_confidence, 0.0);
        cout << "match_confidence = " << match_confidence << endl;
        break;
    default:
        return false;
    }
}

void App::parseCmdArgs(int argc, char **argv)
{
    for (int i = 1; i < argc && !help_showed; ++i)
    {
        if (parseBaseCmdArgs(i, argc, argv))
            continue;

        string arg(argv[i]);

        if (arg == "--offset")
            the_same_video_offset = atoi(argv[++i]);
        else
            throwBadArgError(argv[i]);
    }
}

void App::printHelp()
{
    cout << "This program demonstrates using SURF_GPU features detector, descriptor extractor and BruteForceMatcher_GPU" << endl;
    cout << "Usage: demo_surf <frames_source1> [<frames_source2>]" << endl;
    cout << " --offset     - set frames offset for the duplicate video source" << endl;
    BaseApp::printHelp();
}

RUN_APP(App)