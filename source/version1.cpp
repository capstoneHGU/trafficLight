#define ROI_RANGE 50
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include <vector>
#include<Windows.h> 

using namespace cv;
using namespace std;

struct HSV_struct{
	int iLowH;
	int iHighH;

	int iLowS;
	int iHighS;

	int iLowV;
	int iHighV;
};

//ojk

/* 사용 방법 return 값은 Circle vector */
std::vector<cv::Vec3f> FindColorCircle(cv::Mat src, cv::Mat *dst, struct HSV_struct *Range, int n = 1){

	for (int i = 0; i < n; i++){
		//Find Color range of :
		Mat temp;
		cv::inRange(src, cv::Scalar(Range->iLowH, Range->iLowS, Range->iLowV), cv::Scalar(Range->iHighH, Range->iHighS, Range->iHighV), temp);
		//가우시안 블러 처리
		cv::GaussianBlur(temp, temp, cv::Size(5, 5), 2, 2);

		// Combine the above two images
		if (i == 0)
			cv::addWeighted(temp, 1.0, temp, 1.0, 0.0, *dst);
		else
			cv::addWeighted(temp, 1.0, *dst, 1.0, 0.0, *dst);
		Range++;
	}

	//threshold(*dst, *dst, 150, 255, 0);

	//Use the Hough transform to detect circles in the combined threshold image --------Green
	std::vector<cv::Vec3f> circles;
	cv::HoughCircles(*dst, circles, CV_HOUGH_GRADIENT, 2, 45, 100, 10, 1, 30);
	return circles;
}

void ControlPanel(char* title, struct HSV_struct * range){
	cv::namedWindow(title, CV_WINDOW_AUTOSIZE); //create a window called "Control"
	//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", title, &(range->iLowH), 179); //Hue (0 - 179)
	cvCreateTrackbar("HighH", title, &(range->iHighH), 179);

	cvCreateTrackbar("LowS", title, &(range->iLowS), 255); //Saturation (0 - 255)
	cvCreateTrackbar("HighS", title, &(range->iHighS), 255);

	cvCreateTrackbar("LowV", title, &(range->iLowV), 255); //Value (0 - 255)
	cvCreateTrackbar("HighV", title, &(range->iHighV), 255);
	// Control Panel Color End
}

void FindRoiFromCenter(cv::Mat *scr, cv::Mat *des, int x, int y){
	if (y - ROI_RANGE < 0){
		(*des) = (*scr).rowRange(0, y + ROI_RANGE);
	}
	else if (y + ROI_RANGE >(*scr).rows){
		(*des) = (*scr).rowRange(y - ROI_RANGE, (*scr).rows);
	}
	else{
		(*des) = (*scr).rowRange(y - ROI_RANGE, y + ROI_RANGE);
	}

	if (x - ROI_RANGE < 0){
		(*des) = (*des).colRange(0, x + ROI_RANGE);
	}
	else if (x + ROI_RANGE >(*scr).cols){
		(*des) = (*des).colRange(x - ROI_RANGE, (*scr).cols);
	}
	else{
		(*des) = (*des).colRange(x - ROI_RANGE, x + ROI_RANGE);
	}
}

int main()
{
	double frameNumb = 0;
	int count_box = 0;
	vector<Point> approx;

	cv::Mat orig_image;
	cv::Mat bgr_image;
	cv::Mat hsv_image;

	cv::Mat lower_red_hue_range;
	cv::Mat upper_red_hue_range;
	cv::Mat red_hue_image;

	cv::Mat lower_yellow_image;
	cv::Mat upper_yellow_image;
	cv::Mat yellow_hue_image;

	cv::Mat green_hue_image;

	cv::Mat black_image;
	// 1. Create image processor object

	Mat red_thresh, green_thresh;

	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));

	// Video
	cv::VideoCapture capture("2.avi");
	if (!capture.isOpened())
		return -1;
	//프레임률 얻기
	double rate = capture.get(CV_CAP_PROP_FPS);

	bool stop(false);
	bool end(false);

	double ratio;

	// 각 프레임 사잉를 밀리초 단위로 지연(delay)
	// 비디오 프레임률에 해당
	int delay = 1000 / rate;


	// Control Panel Color HSV

	struct HSV_struct green_range;
	green_range.iLowH = 70;
	green_range.iHighH = 100;

	green_range.iLowS = 100;
	green_range.iHighS = 255;

	green_range.iLowV = 100;
	green_range.iHighV = 255;

	ControlPanel("Green Color Range Editor", &green_range);

	struct HSV_struct red_ranges[2];

	red_ranges[0].iLowH = 160;
	red_ranges[0].iHighH = 179;

	red_ranges[0].iLowS = 100;
	red_ranges[0].iHighS = 255;

	red_ranges[0].iLowV = 100;
	red_ranges[0].iHighV = 255;

	red_ranges[1].iLowH = 0;
	red_ranges[1].iHighH = 25;

	red_ranges[1].iLowS = 100;
	red_ranges[1].iHighS = 255;

	red_ranges[1].iLowV = 100;
	red_ranges[1].iHighV = 255;

	ControlPanel("Red Color Low Range Editor", &red_ranges[0]);
	ControlPanel("Red Color High Range Editor", &red_ranges[1]);

	struct HSV_struct yellow_range;

	yellow_range.iLowH = 15;
	yellow_range.iHighH = 50;

	yellow_range.iLowS = 80;
	yellow_range.iHighS = 255;

	yellow_range.iLowV = 80;
	yellow_range.iHighV = 255;


	// 비디오의 모든 프레임에 대해

	Mat canny;

	while (!end){

		bool red = false;
		bool green = false;
		bool show = false;

		if (!stop)
		{
			frameNumb++;
			cout << "framenumb = " << frameNumb << endl;
		}


		//다음 프레임이 있으면 읽기
		if (!stop){
			if (!capture.read(bgr_image))
				break;

			if ((int)frameNumb % 3 == 0){

			}
			else
			{
				continue;
			}

			bgr_image = bgr_image.rowRange(220, 400);
			bgr_image = bgr_image.colRange(550, 800);

			orig_image = bgr_image.clone();


			cv::cvtColor(bgr_image, hsv_image, cv::COLOR_BGR2HSV);



			std::vector <cv::Vec3f> circles = FindColorCircle(hsv_image, &red_hue_image, red_ranges, 2);
			std::vector <cv::Vec3f> circles2 = FindColorCircle(hsv_image, &green_hue_image, &green_range);

			cv::imshow("red", red_hue_image);
			cv::imshow("green", green_hue_image);
			
			for (size_t current_circle = 0; current_circle < circles.size(); ++current_circle) {
				// Get Center
				cv::Point center(std::round(circles[current_circle][0]), std::round(circles[current_circle][1]));// Point center
				// Get Radius
				int radius = std::round(circles[current_circle][2]); // get the radius according to the center pointed
				// Draw Circle
				cv::circle(orig_image, center, radius, cv::Scalar(30, 30, 30), 3); // 
				putText(orig_image, std::to_string(current_circle + 1), center, 1, 2, cv::Scalar(0, 0, 255), 2);
				// Tell
				std::cout << current_circle + 1 << " Red light  :  " << center.x << "," << center.y << std::endl;
			}

			for (size_t current_circle2 = 0; current_circle2 < circles2.size(); ++current_circle2) {
				// Get Center
				cv::Point center2(std::round(circles2[current_circle2][0]), std::round(circles2[current_circle2][1]));// Point center
				// Get Radius
				int radius2 = std::round(circles2[current_circle2][2]); // get the radius according to the center pointed
				// Draw Circle
				cv::circle(orig_image, center2, radius2, cv::Scalar(200, 100, 100), 3); //
				putText(orig_image, std::to_string(current_circle2 + 1), center2, 1, 2, cv::Scalar(0, 255, 0), 2);
				// Tell
				std::cout << current_circle2 + 1 << " green light  :  " << center2.x << "," << center2.y << std::endl;
			}

			Sleep(300);
				
			cv::namedWindow("output", cv::WINDOW_AUTOSIZE);
			cv::imshow("output", orig_image);
		}
		//지연 도임
		//혹은 중지하기 위해 키 입력
		int key = waitKey(delay);

		if (key == 32)
		{
			if (stop == false)
				stop = true;
			else
				stop = false;
		}
		else if (key == 27)
		{
			break;
		}
		else if (key == 109)
		{
			capture.set(CV_CAP_PROP_POS_FRAMES, frameNumb + 100);
			frameNumb = frameNumb + 100;
		}
		else if (key == 110)
		{
			capture.set(CV_CAP_PROP_POS_FRAMES, frameNumb - 100);
			frameNumb = frameNumb - 100;
		}
	}

	capture.release();




	return 0;
}


