#include "ApplyIPM.h"
#include "LaneFinder.h"
#include "CollissionFinder.h"
#include "Movement.h"
#include <ros/ros.h>

using namespace cv;

int var_ksi = 100; //Kalman filter variance
double line_start_mu_prev = 155;
double line_start_mu_estimate = 155;
double line_start_var_prev = var_ksi;
double line_start_var_estimate = var_ksi;

static void onMouse( int event, int x, int y, int, void* )
{
    if( event != CV_EVENT_LBUTTONDOWN )
        return;
    //std::cout << x << std::endl;
    line_start_mu_prev = (double) x;
    line_start_mu_estimate = (double) x;
}

void getInitialBelief(VideoCapture cap, Vec3b &color) {
    namedWindow("Click on lane", 0);
    setMouseCallback("Click on lane", onMouse, 0);
    Mat frame;
    
    ApplyIPM *initMapper = new ApplyIPM(480, 640);
    Mat ipmFrame = Mat::zeros(initMapper->nCols, initMapper->nRows, CV_8UC3);
    
    while (true) {
        cap >> frame;
        initMapper -> mapBGRfast(frame, ipmFrame);
        color = extractRoadColor(ipmFrame);

        int searchLimitLeft = static_cast<int>((line_start_mu_estimate - sqrt(line_start_var_estimate)));
        int searchLimitRight = static_cast<int>((line_start_mu_estimate + sqrt(line_start_var_estimate)));
        int pos_start = ipmFrame.rows;
        int pos_end = ipmFrame.rows - 20;
        line(ipmFrame, Point(searchLimitLeft,pos_start), Point(searchLimitLeft,pos_end) , Scalar(0,0,0), 3);
        line(ipmFrame, Point(searchLimitRight,pos_start), Point(searchLimitRight,pos_end) , Scalar(0,0,0), 3);
        imshow("Click on lane", ipmFrame);
        
        if ((waitKey(20) & 255) == 27) {
            destroyWindow("Click on lane");
            return;
        }
    }
}

int main(int argc, char* argv[]) {
	ros::init(argc, argv, "turtle_track");
	ros::NodeHandle nh;
	ros::Rate loop_rate(30);
	
	//video feed
	VideoCapture cap; cap.open(1);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	Mat rgbCamFrame; //camera BGR frame
	
	//get the initial road color and lane position manually
	Vec3b roadColor;
	getInitialBelief(cap, roadColor);	
	
	//debug windows
	namedWindow("Original", CV_WINDOW_AUTOSIZE);
	//cvNamedWindow("Output", CV_WINDOW_AUTOSIZE);

	//IPM mapper
	ApplyIPM *mapper = new ApplyIPM(cap.get(CV_CAP_PROP_FRAME_HEIGHT), cap.get(CV_CAP_PROP_FRAME_WIDTH));
	Mat rgbWorFrame = Mat(mapper->nCols, mapper->nRows, CV_8UC3);	//top-down world coordinates frame
	fillRoadColor(rgbWorFrame, roadColor);	//initialize with the road color, we don't detect false collissions
	//std::cout << mapper->nCols << mapper->nRows << std::endl;

	//Lane finder	
	LaneFinder *myLaneFinder = new LaneFinder(mapper->nCols, mapper->nRows);
	vector<CvPoint> line;	//the line to follow
	double alpha = -1;		//the angle the line bends
	IplImage* imgOut = NULL;	//lane detection output for debugging

	//Robot mover
	BotMover* mover = new BotMover(nh, 0.285, 0.5, 0.01);

	//main loop
	while(ros::ok()) {
		cap >> rgbCamFrame;														//capture next frame
		
		mapper->mapBGRfast(rgbCamFrame, rgbWorFrame);	//applyIPM
		Mat bwWorFrame; cvtColor(rgbWorFrame, bwWorFrame, CV_BGR2GRAY);
		blur(bwWorFrame, bwWorFrame, Size(3,3), Point(-1,-1), BORDER_DEFAULT);
		IplImage bwWorImg = bwWorFrame;								//BW blurred image for easier line detection
		imgOut = cvCreateImage(cvSize(bwWorImg.width,bwWorImg.height),IPL_DEPTH_8U,3);

		//Kalman filter limits to prune the line search
		int searchLimitLeft = std::max(110,static_cast<int>((line_start_mu_estimate - sqrt(line_start_var_estimate))));
		int searchLimitRight = std::min(222,static_cast<int>((line_start_mu_estimate + sqrt(line_start_var_estimate))));

		Mat colWorImg(rgbWorFrame.rows, rgbWorFrame.cols, CV_8U);
		boost::thread t0(&findCollisionsBin, rgbWorFrame, colWorImg, roadColor, 35);		
		//get line
		myLaneFinder->extractLane(&bwWorImg, imgOut, line, searchLimitLeft, searchLimitRight, 70, 57, alpha); 
		//find all collission pixels
		t0.join();
		//find closest collission on the path
		int distFromObject = avoidCollisionBinUpdate(colWorImg, rgbWorFrame, roadColor, line, 38, 14, 105);
		//move robot
		mover->move(alpha, distFromObject);
		
		//debug
		//std::cout << distFromObject << std::endl;
		imshow("Original", colWorImg);
		
		//update Kalman filter parameters
		double line_start_c = 0;
		if(line.size()>0) line_start_c = static_cast<double>(line[0].x);
		line_start_mu_prev = line_start_mu_estimate;
		line_start_var_prev = line_start_var_estimate;
		myLaneFinder->doKalmanFiltering(line_start_mu_prev, line_start_var_prev, line_start_c, line_start_mu_estimate, line_start_var_estimate);
     
		line.clear();

		/*/debug
		int pos_start = imgOut->height;
		int pos_end = imgOut->height-20;
		cvLine(imgOut, cvPoint(searchLimitLeft,pos_start), cvPoint(searchLimitLeft,pos_end) , cvScalar(0,0,0), 3);
		cvLine(imgOut, cvPoint(searchLimitRight,pos_start), cvPoint(searchLimitRight,pos_end) , cvScalar(0,0,0), 3);
		cvShowImage( "Output", imgOut);
		if ((waitKey(1) & 255) == 27) break;
		//end debug*/
		if ((waitKey(10) & 255) == 27) break;
		ros::spinOnce();
		loop_rate.sleep();
	}
	
	//debug windows
	destroyWindow("Original");
	//cvDestroyWindow("Output");
	return 0;
}
