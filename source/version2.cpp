#include <SDKDDKVer.h>
#include <Windows.h>
#include <iostream>

#pragma warning(disable:4819)
#pragma warning(disable:4996)

// for OpenCV2
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "opencv2/highgui/highgui.hpp"


#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <opencv2/nonfree/features2d.hpp>

using namespace cv;
using namespace std;

#define RED 0
#define GREEN 1

typedef struct HSV_struct{
	int iLowH;
	int iHighH;

	int iLowS;
	int iHighS;

	int iLowV;
	int iHighV;
};
class TrafficLight {
private:
	Point point;
	double radius;
	int color;

	int vote;

public:
	TrafficLight(Point point, double radius, int color) {
		this->point = point;
		this->radius = radius;
		this->color = color;
		this->vote = 0;

	}
	void voteUp() {
		vote++;
	}
	static void voteUpOrAdd(vector<TrafficLight> &temp, vector<TrafficLight> &cur) {
		bool exist = false;
		for (int j = temp.size() - 1; j >= 0; j--){
			exist = false;
			for (int i = cur.size() - 1; i >= 0; i--) {
				//(cur[i].getColor() == temp[j].getColor()) && 
				if ((cur[i].sameLight(temp[j].getCenter()))){ // exist
					cur[i].voteUp();
					exist = true;
					break;
				}
			}
			if (!exist){ // If it didn't exist Add Up
				cur.push_back(temp[j]);
			}
		}
		
	}
	
	bool sameLight(Point center) {

		Point thisCenter = getCenter();
		double thisRadius = getRadius();
		if (((thisCenter.x - thisRadius) < center.x) && ((thisCenter.y - thisRadius) < center.y) &&
			((thisCenter.x + thisRadius) > center.x) && ((thisCenter.y + thisRadius) > center.y)) {

			return true;
		}

		return false;
	}
	double getRadius() {
		return radius;
	}
	Point getCenter() {
		return point;
	}
	int getColor() {
		return color;
	}
	int getVote() {
		return vote;
	}

};
/**
* Helper function to display text in the center of a contour
*/
void setLabel(cv::Mat& im, const std::string label, std::vector<cv::Point>& contour)
{
	int fontface = cv::FONT_HERSHEY_SIMPLEX;
	double scale = 0.4;
	int thickness = 1;
	int baseline = 0;


	cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
	cv::Rect r = cv::boundingRect(contour);

	cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
	cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255, 255, 255), CV_FILLED);
	cv::putText(im, label, pt, fontface, scale, CV_RGB(0, 0, 0), thickness, 8);
}

void controlPanel(char* title, HSV_struct * range){
	cv::namedWindow(title, CV_WINDOW_AUTOSIZE); //create a window called "Control"
	//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", title, &(range->iLowH), 255); //Hue (0 - 179)
	cvCreateTrackbar("HighH", title, &(range->iHighH), 255);

	cvCreateTrackbar("LowS", title, &(range->iLowS), 255); //Saturation (0 - 255)
	cvCreateTrackbar("HighS", title, &(range->iHighS), 255);

	cvCreateTrackbar("LowV", title, &(range->iLowV), 255); //Value (0 - 255)
	cvCreateTrackbar("HighV", title, &(range->iHighV), 255);
	// Control Panel Color End
}

void findColor(cv::Mat &src, cv::Mat &dst, HSV_struct *Range, int n = 1){

	for (int i = 0; i < n; i++){
		//Find Color range of :
		Mat temp;
		cv::inRange(src, cv::Scalar(Range->iLowH, Range->iLowS, Range->iLowV), cv::Scalar(Range->iHighH, Range->iHighS, Range->iHighV), temp);
		//가우시안 블러 처리
		cv::GaussianBlur(temp, temp, cv::Size(5, 5), 2, 2);

		// Combine the above two images
		if (i == 0)
			cv::addWeighted(temp, 1.0, temp, 1.0, 0.0, dst);
		else
			cv::addWeighted(temp, 1.0, dst, 1.0, 0.0, dst);
		Range++;
	}
}

vector<TrafficLight> findCircle(Mat &src, Mat &drawOnFrame){
	vector<TrafficLight> lightList;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy; // 그림그릴때 필요한 변수
	vector<Point> contoursOUT; // 각의 점들을 넣는곳

	Point2f center;
	float radius;
	findContours(src, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

	for (int i = 0; i < contours.size(); i++){
		//float area = contourArea(contours[i], false);

		approxPolyDP(Mat(contours[i]), contoursOUT, 3, true); //Get the 각

		//drawContours(frame, contours, i, Scalar(0, 50, 50), 2, 8, hierarchy);

		if (contoursOUT.size() >= 4){ // If circle
			//int index = TrafficLight::voteUpIfExist(contoursOUT, radius, storedTrafficLight);
			cout << "Circle Detected" << contoursOUT.size() << ", Number :" << i << endl;

			// Get center and radius of the circle
			minEnclosingCircle(contoursOUT, center, radius); 
			//drawContours(drawOnFrame, contours, i, Scalar(0, 255, 50), 2, 8, hierarchy);
			lightList.push_back(TrafficLight(center, radius, RED));
			/*double area = cv::contourArea(contours[i]);
			
			cv::Rect r = cv::boundingRect(contours[i]);
			int radius = r.width / 2;

			if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 &&
				std::abs(1 - (area / (CV_PI * (radius*radius)))) <= 0.2)

				setLabel(red_image_thresh, "RC", contours[i]);*/
		}

		//float area = contourArea(contours[i], false);
		//drawContours(test, contours, i, Scalar(255, 255, 255), 2, 8, hierarchy);

	}

	return lightList;
}



int main(int argc, char* argv[]){

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




	//controlPanel("RGB Red Color Low Range Editor", &gbr_red_ranges[0]);
	//////////////////////////

	struct HSV_struct green_range;
	green_range.iLowH = 70;
	green_range.iHighH = 100;

	green_range.iLowS = 100;
	green_range.iHighS = 255;

	green_range.iLowV = 100;
	green_range.iHighV = 255;

	//controlPanel("RGB Red Color Low Range Editor", &gbr_green_ranges[0]);


	vector<TrafficLight> storedTrafficLight;


	cv::VideoCapture capture("5.avi");
	if (!capture.isOpened())
		return -1;

	double rate = capture.get(CV_CAP_PROP_FPS);
	//cap.set(CV_CAP_PROP_FPS, 250);
	int delay = 1000 / rate;

	int frameNumb = 0;
	int old_framecount = 0;
	bool stop = false;

	while (true) {
		if (!stop)
		{
			frameNumb++;
			std::cout << "framenumber = " << frameNumb << std::endl;
		}
		if (!stop){


			if ((int)frameNumb % 3 == 0){

			}
			else
			{
				continue;
			}
			cv::Mat frame;
			if (!capture.read(frame))
				break;

			frame = frame.rowRange(200, 350);
			frame = frame.colRange(400, 900);
			//resize(frame, frame, Size(), 2, 2);
			Mat hsv_image;

			Mat hsv_red_image;
			Mat hsv_green_image;

			Mat red_image_thresh;
			Mat green_image_thresh;


			

			cv::cvtColor(frame, hsv_image, cv::COLOR_BGR2HSV);

			//Find Color Range
			findColor(hsv_image, hsv_red_image, red_ranges, 2); // RED
			findColor(hsv_image, hsv_green_image, &green_range, 1); // Green
			
			
			//이진화
			threshold(hsv_red_image, red_image_thresh, 150, 255, CV_THRESH_BINARY);
			threshold(hsv_green_image, green_image_thresh, 0, 255, CV_THRESH_BINARY);

			/*
			imshow("red_image_thresh Frame", red_image_thresh);

			resize(red_image_thresh, red_image_canny, Size(), 1, 1);
			resize(green_image_thresh, green_image_canny, Size(), 1, 1);
			//Canny
			Canny(red_image_canny, red_image_canny, 0, 0);
			Canny(green_image_canny, green_image_canny, 0, 0);
			*/
			
			//Mat test = red_image_thresh.clone();

			vector<TrafficLight> tempTrafficLightList = findCircle(red_image_thresh.clone(), frame);
			TrafficLight::voteUpOrAdd(tempTrafficLightList, storedTrafficLight);

			for (int i = storedTrafficLight.size() - 1; i >= 0; i--){
				if (storedTrafficLight[i].getVote()>50)
					circle(frame, storedTrafficLight[i].getCenter(), storedTrafficLight[i].getRadius(), cv::Scalar(30, 255, 30), 3);

			}
			//vector<vector<Point>> contours;
			//vector<Vec4i> hierarchy;
			//vector<Point> contoursOUT;
			//findContours(red_image_thresh, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

			//for (int i = 0; i < contours.size(); i++){
			//	float area = contourArea(contours[i], false);
			//	
			//	

			//	approxPolyDP(Mat(contours[i]), contoursOUT, 3, true); //Get the 각

			//	drawContours(frame, contours, i, Scalar(0, 50, 50), 2, 8, hierarchy);

			//	if (contoursOUT.size() >= 4){
			//		//int index = TrafficLight::voteUpIfExist(contoursOUT, radius, storedTrafficLight);
			//		cout << "Circle Detected" << contoursOUT.size() << ", Number :" << i << endl;

			//		double area = cv::contourArea(contours[i]);
			//		cv::Rect r = cv::boundingRect(contours[i]);
			//		int radius = r.width / 2;

			//		/*if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 &&
			//			std::abs(1 - (area / (CV_PI * (radius*radius)))) <= 0.2)

			//			setLabel(red_image_thresh, "RC", contours[i]);*/
			//	}

			//	//float area = contourArea(contours[i], false);
			//	//drawContours(test, contours, i, Scalar(255, 255, 255), 2, 8, hierarchy);

			//}



			imshow("Original Frame", frame);
			imshow("gbr_red_image", red_image_thresh);
			//imshow("gbr_red_image", hsv_red_image);
			//imshow("T test ", red_image_thresh);
			//imshow("green_image_canny", green_image_canny);

			//imshow("red_image_thresh", red_image_thresh);
			//imshow("T test ", test);
			//imshow("red_image_sobel", red_image_canny);

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


}