#include <SDKDDKVer.h>
#include <Windows.h>
#include <iostream>
#include <time.h>

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

#define VOTE_MAX 20
#define TRUE_LIGHT 5

#define MOVING 0
#define STOP_NO_RED 1
#define STOP_RED_FOUND 2
#define LEFT_GREEN 3
#define STRAIGHT_GREEN 4
#define LEFT_STRAIGHT_GREEN 5

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

void RoiFromCenter(cv::Mat &src, cv::Mat &des, Point center, int radius){
	double x = center.x;
	double y = center.y;
	cout << "radius" << radius << endl;

	//if (src.ro< radius*2)

	if ((y - ((double)radius * 2)) < 0){
		(des) = (src).rowRange(0, y + ((double)radius * 2));
	}
	else if (y + ((double)radius * 2) >(src).rows){
		(des) = (src).rowRange(y - ((double)radius * 2), (src).rows);
	}
	else{
		(des) = (src).rowRange(y - ((double)radius * 2), y + ((double)radius * 2));
	}

	if (x - ((double)radius * 2) < 0){
		(des) = (des).colRange(0, x + (10 * radius));
	}
	else if (x + ((double)radius * 2) >(src).cols){
		(des) = (des).colRange(x - ((double)radius * 2), (src).cols);
	}
	else{
		(des) = (des).colRange(x - ((double)radius * 2), x + (10 * radius));
	}

	/*(*des) = (src).rowRange(y - ((double)radius*1.5), y + ((double)radius*1.5));
	(*des) = (src).colRange(x - ((double)radius*1.5), x - (10 * radius));*/
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
	vector<TrafficLight> tempTrafficLightList;
	int state = MOVING;
	

	Mat frame;
	Mat hsv_image;

	Mat hsv_red_image;
	Mat hsv_green_image;

	Mat red_image_thresh;
	Mat green_image_thresh;

	Mat red_captured;

	/* Control Panel For Speed */
	int speed = 3;
	cv::namedWindow("Car Speed", CV_WINDOW_AUTOSIZE); //create a window called "Control"
	//Create trackbars in "Control" window
	cvCreateTrackbar("speed", "Car Speed", &speed, 100);



	cv::VideoCapture capture("야간/1.avi");
	if (!capture.isOpened())
		return -1;

	double rate = capture.get(CV_CAP_PROP_FPS);
	//cap.set(CV_CAP_PROP_FPS, 250);
	int delay = 1000 / rate;

	int frameNumb = 0;
	int old_framecount = 0;
	bool stop = false;
	clock_t begin, finish;

	begin = clock();



	int erosion_elem = 0;
	int erosion_size = 5;
	int const max_elem = 2;
	int const max_kernel_size = 21;
	
	int erosion_type = MORPH_ELLIPSE;


	while (true) {
		if (!stop)
		{
			frameNumb++;
			std::cout << "framenumber = " << frameNumb << std::endl;
		}
		if (!stop){
			if (!capture.read(frame))
				break;

			if ((int)frameNumb % 3 == 0){

			}
			else
			{
				continue;
			}
			
			

			//Mat org = frame.clone();
			//frame = frame.rowRange(0, 200);
			//frame = frame.colRange(550, 800);
			Mat bw, refined, hsv_refined, hsv_red_refined;
			vector<Mat> frame_channels;
			



			cvtColor(frame, bw, COLOR_BGR2GRAY);
			threshold(bw, bw, 252, 255, CV_THRESH_BINARY);

			split(frame, frame_channels);


			Mat element = getStructuringElement(erosion_type,
				Size(2 * erosion_size + 1, 2 * erosion_size + 1),
				Point(erosion_size, erosion_size));

			/// Apply the erosion operation
			dilate(bw, bw, element);

			bitwise_and(bw, frame_channels[0], frame_channels[0]);
			bitwise_and(bw, frame_channels[1], frame_channels[1]);
			bitwise_and(bw, frame_channels[2], frame_channels[2]);

			merge(frame_channels, refined);

			cvtColor(refined, hsv_refined, cv::COLOR_BGR2HSV);

			findColor(hsv_refined, hsv_red_refined, red_ranges, 2);

			tempTrafficLightList = findCircle(hsv_red_refined.clone(), frame);

			for (int i = tempTrafficLightList.size() - 1; i >= 0; i--){

				if (tempTrafficLightList[i].getRadius() < 10)
					circle(frame, tempTrafficLightList[i].getCenter(), tempTrafficLightList[i].getRadius(), cv::Scalar(30, 255, 30), 3);
				
			}

			imshow("Final", hsv_red_refined);

			switch (state){
			case MOVING:
				if (speed == 100)
					state = STOP_NO_RED;
				break;

			case STOP_NO_RED: // Search for RED
				

				cv::cvtColor(frame, hsv_image, cv::COLOR_BGR2HSV);

				// Find Color
				findColor(hsv_image, hsv_red_image, red_ranges, 2); 
				//threshold
				threshold(hsv_red_image, red_image_thresh, 15, 255, CV_THRESH_BINARY);

				imshow("red_image_thresh ww", hsv_red_image);
				//Find circle Shapes and save in tempTrafficLightList 
				tempTrafficLightList = findCircle(red_image_thresh.clone(), frame);
				
				//Search through the main list StoreTrafficLight 
				//If it exist it will voteUp
				//If it doesn't exist it will make new object in the StoreTrafficLight
				TrafficLight::voteUpOrAdd(tempTrafficLightList, storedTrafficLight);

				for (int i = storedTrafficLight.size() - 1; i >= 0; i--){

					if (storedTrafficLight[i].getVote()>VOTE_MAX){ // If one of them is at higher than VOTE_MAX go to next State
						//circle(frame, storedTrafficLight[i].getCenter(), storedTrafficLight[i].getRadius(), cv::Scalar(30, 255, 30), 3);
						state = STOP_RED_FOUND;
						//finish = clock();
						//cout << "수행시간: " << ((double)(finish - begin) / CLOCKS_PER_SEC) << endl;
						//Sleep(1000);
					}
				}

				break;

			case STOP_RED_FOUND:
				cv::cvtColor(frame, hsv_image, cv::COLOR_BGR2HSV);

				

				for (int i = storedTrafficLight.size() - 1; i >= 0; i--){
					if (storedTrafficLight[i].getVote()>TRUE_LIGHT){ // If one of them is at higher than VOTE_MAX go to next State
						circle(frame, storedTrafficLight[i].getCenter(), storedTrafficLight[i].getRadius(), cv::Scalar(30, 255, 30), 3);
						
						//RoiFromCenter(hsv_image, red_captured, storedTrafficLight[i].getCenter(), storedTrafficLight[i].getRadius());
						//
						//imshow("red_captured Frame", red_captured);
						//findColor(red_captured, hsv_green_image, &green_range, 1); // Green

						//vector<TrafficLight> tempGreenList = findCircle(hsv_green_image.clone(), frame);
						//imshow("hsv_green_image Frame", hsv_green_image);
						//if (tempGreenList.empty() == false){
						//	state = STRAIGHT_GREEN;
						//}
					}
				}
				
				break;
			case STRAIGHT_GREEN:

				putText(frame, "Time TO Move ", Point(50,50), 1, 3, CV_RGB(0, 0, 0), 1, 8);
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



			//imshow("Original Frame", frame);
			//imshow("gbr_red_image", red_image_thresh);
			//imshow("Real org", org);
			//imshow("gbr_red_image", hsv_red_image);
			//imshow("T test ", red_image_thresh);
			//imshow("green_image_canny", green_image_canny);

			//imshow("red_image_thresh", red_image_thresh);
			//imshow("T test ", test);
			//imshow("red_image_sobel", red_image_canny);
			imshow("Original Frame", frame);

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