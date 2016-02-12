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

#define VOTE_MAX 7
#define TRUE_LIGHT 5

#define MOVING 0
#define STOP_NO_RED 1
#define STOP_RED_FOUND 2
#define LEFT_GREEN 3
#define STRAIGHT_GREEN 4
#define LEFT_STRAIGHT_GREEN 5

#define STOP_SEEK_FIRST_RED 6
#define STOP_SEEK_N_RED 7

#define OFFSET 5
class ThreeChannelRange{
public:
	int Low1;
	int High1;

	int Low2;
	int High2;

	int Low3;
	int High3;

	ThreeChannelRange(int Low1, int High1, int Low2, int High2, int Low3, int High3){
		this->Low1 = Low1;
		this->High1 = High1;
		this->Low2 = Low2;
		this->High2 = High2;
		this->Low3 = Low3;
		this->High3 = High3;
	}

	Scalar getLower(){
		//cout << "lower " << this->Low1 << " " << this->Low2 << " " << this->Low3 <<endl;
		return Scalar(this->Low1, this->Low2, this->Low3);
	}
	
	Scalar getHigher(){
		//cout << "higher " << this->High1 << " " << this->High2 << " " << this->High3 << endl;
		return Scalar(this->High1, this->High2, this->High3);
	}

	void controlPanel(char * title){
		cv::namedWindow(title, CV_WINDOW_AUTOSIZE); //create a window called "Control"
		//Create trackbars in "Control" window
		cvCreateTrackbar("LowH", title, &(this->Low1), 255); //Hue (0 - 179)
		cvCreateTrackbar("HighH", title, &(this->High1), 255);

		cvCreateTrackbar("LowS", title, &(this->Low2), 255); //Saturation (0 - 255)
		cvCreateTrackbar("HighS", title, &(this->High2), 255);

		cvCreateTrackbar("LowV", title, &(this->Low3), 255); //Value (0 - 255)
		cvCreateTrackbar("HighV", title, &(this->High3), 255);
		// Control Panel Color End
	}
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
	static void voteUpOrAdd(vector<TrafficLight> &temp, vector<TrafficLight> &cur, int minRadius, int maxRadius) {
		bool exist = false;
		for (int j = temp.size() - 1; j >= 0; j--){

			//int tempRadius = temp[j].getRadius();
			Point tempCenter = temp[j].getCenter();
			int tempRadius = temp[j].getRadius();
			//Check minimum and Maximum of the Radius
			/*
			if ((tempRadius < minRadius) || tempRadius > maxRadius){
				continue;
			}
			*/

			//Check the location of the Center(circle)
			//if it is too close to the border, it won't be count as the traffic light
			if ((tempCenter.x < tempRadius) || ((tempCenter.x + tempRadius) >  1280)){
				continue;
			}
			else if ((tempCenter.y < tempRadius) || ((tempCenter.y + tempRadius) >  720)){
				continue;
			}



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

	static void voteUpOrAdd(TrafficLight &temp, vector<TrafficLight> & trafficList) {
		
		bool exist = false;

		for (int i = trafficList.size() - 1; i >= 0; i--) {
			//(trafficList[i].getColor() == temp[j].getColor()) && 
			if ((trafficList[i].sameLight(temp.getCenter()))){ // exist then vote up
				trafficList[i].voteUp();
				exist = true;
				break;
			}
		}

		if (!exist){ // else if it doesn't exist in the list it will Add Up to the list
			trafficList.push_back(temp);
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

void findColor(cv::Mat &src, cv::Mat &dst, vector <ThreeChannelRange> &Range){

	for (int i = 0; i < Range.size(); i++){
		//Find Color range of :
		Mat temp;
		cv::inRange(src, Range[i].getLower() , Range[i].getHigher(), temp);
		//가우시안 블러 처리
		GaussianBlur(temp, temp, cv::Size(3, 3), 2, 2);

		// Combine the above two images
		if (i == 0)
			cv::addWeighted(temp, 1.0, temp, 1.0, 0.0, dst);
		else
			cv::addWeighted(temp, 1.0, dst, 1.0, 0.0, dst);
		
	}
}

// Circle should have its Area == r^2pi
void findCircle(Mat &src, vector<TrafficLight> & trafficList, float minRadius = 5, float maxRadius = 20, int rows = 720, int cols = 1280, int color = RED){
	
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy; // 그림그릴때 필요한 변수
	vector<Point> contoursOUT; // 각의 점들을 넣는곳
	
	findContours(src, contours, hierarchy, RETR_CCOMP, CV_CHAIN_APPROX_NONE);

	for (int i = 0; i < contours.size(); i++){
		approxPolyDP(Mat(contours[i]), contoursOUT, 3, true); //Get the 각

		if (contoursOUT.size() >= 4){ // If more than 3 points

			//int index = TrafficLight::voteUpIfExist(contoursOUT, radius, storedTrafficLight);
			//cout << "Circle Detected" << contoursOUT.size() << ", Number :" << i << endl;

			Point2f center;
			float radius;

			// Get center and radius of the circle & area
			minEnclosingCircle(contoursOUT, center, radius);
			float conArea = contourArea(contoursOUT, false);
			float radArea = (CV_PI*radius*radius);
			cv::Rect r = cv::boundingRect(contoursOUT);


			//cout << "Contour AREA : " << conArea << "circle AREA :" << radArea << endl;
			//cout << "RADIUS " << radius << "center.x " << center.x << "center.y" << center.y << endl;
			//drawContours(drawOnFrame, contours, i, Scalar(0, 255, 50), 2, 8, hierarchy);
			//if ((conArea + 45*radArea/100) > radArea)
			//circle(src, center, radius, cv::Scalar(30, 255, 30), 3);

			if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 &&   // Check if the shape is circle
				std::abs(1 - (conArea / radArea)) <= 0.5)
				if ((radius > minRadius) && radius < maxRadius) // Store when : minRadius < radius < maxRadius
					
					//Check the location of the Center(circle)
					//if it is too close to the border, it won't be count as the traffic light
					// because there may be error getting roi too ...
					if (((center.x > radius) && ((center.x + radius) <  cols)) &&
						((center.y > radius) && ((center.y + radius) <  rows))){

						TrafficLight::voteUpOrAdd(TrafficLight(center, radius, color), trafficList);
					}
					

		}

	}
}

void findCircleFromRoi(Mat &src, vector<TrafficLight> & trafficList, int index,  float minRadius = 5, float maxRadius = 20, int color = RED){
	
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy; // 그림그릴때 필요한 변수
	vector<Point> contoursOUT; // 각의 점들을 넣는곳

	findContours(src, contours, hierarchy, RETR_CCOMP, CV_CHAIN_APPROX_NONE);

	for (int i = 0; i < contours.size(); i++){
		approxPolyDP(Mat(contours[i]), contoursOUT, 3, true); //Get the 각

		if (contoursOUT.size() >= 4){ // If more than 3 points

			//int index = TrafficLight::voteUpIfExist(contoursOUT, radius, storedTrafficLight);
			//cout << "Circle Detected" << contoursOUT.size() << ", Number :" << i << endl;

			Point2f center;
			Point oldCenter = trafficList[index].getCenter();
			Point newCenter;
			float radius;
			float oldRadius = trafficList[index].getRadius();
			

			// Get center and radius of the circle & area
			minEnclosingCircle(contoursOUT, center, radius);
			float conArea = contourArea(contoursOUT, false);
			float radArea = (CV_PI*radius*radius);
			cv::Rect r = cv::boundingRect(contoursOUT);

			newCenter.x = oldCenter.x - oldRadius + radius;//- OFFSET + center.x;
			newCenter.y = oldCenter.y - oldRadius + radius;//- OFFSET + center.y;

			//cout << "Contour AREA : " << conArea << "circle AREA :" << radArea << endl;
			//cout << "RADIUS " << radius << "center.x " << center.x << "center.y" << center.y << endl;
			//drawContours(drawOnFrame, contours, i, Scalar(0, 255, 50), 2, 8, hierarchy);
			//if ((conArea + 45*radArea/100) > radArea)
			if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 &&   // Check if the shape is circle
				std::abs(1 - (conArea / radArea)) <= 0.5)
				if ((radius > minRadius) && radius < maxRadius) // Store when : minRadius < radius < maxRadius

					//Check the location of the Center(circle)
					//if it is too close to the border, it won't be count as the traffic light
					// because there may be error getting roi too ...
					/*if (((center.x > radius) && ((center.x + radius) <  cols)) &&
						((center.y > radius) && ((center.y + radius) <  rows))){*/

					TrafficLight::voteUpOrAdd(TrafficLight(newCenter, radius, color), trafficList);
					//}


		}

	}
}

void findHighIntensity(Mat &src, Mat &dst, int minThresh, int maxThresh){
	int dilation_elem = 0;
	int dilation_size = 5;
	int const max_elem = 2;
	int const max_kernel_size = 21;

	int dilation_type = MORPH_ELLIPSE;

	Mat element = getStructuringElement(dilation_type,
		Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		Point(dilation_size, dilation_size));


	cvtColor(src, dst, COLOR_BGR2GRAY);
	threshold(dst, dst, minThresh, maxThresh, CV_THRESH_BINARY);

	/// Apply the dilation operation
	dilate(dst, dst, element);
}

void getTrafficLightFromList(cv::Mat &src, vector<TrafficLight> &storedTrafficList, vector<Mat> &out_TrafficsRoi){
	
	//if (src.ro< radius*2)
	for (int i = storedTrafficList.size() - 1; i >= 0; i--){
		
		Point center = storedTrafficList[i].getCenter();
		double radius = storedTrafficList[i].getRadius();
		int offset = OFFSET;

		Mat temp = src.rowRange(center.y - radius - offset, center.y + radius + offset);
		temp = temp.colRange(center.x - radius - offset, center.x + radius + offset);

		out_TrafficsRoi.push_back(temp);


	}
	

	/*(*des) = (src).rowRange(y - ((double)radius*1.5), y + ((double)radius*1.5));
	(*des) = (src).colRange(x - ((double)radius*1.5), x - (10 * radius));*/
}

int main(int argc, char* argv[]){

	//
	// Color Range Setting

	vector<ThreeChannelRange> HSVred_ranges;
	vector<ThreeChannelRange> HSVgreen_ranges;

	HSVred_ranges.push_back(ThreeChannelRange(160, 179, 50, 255, 100, 255));
	HSVred_ranges.push_back(ThreeChannelRange(0, 25, 50, 255, 100, 255));

	//HSVred_ranges[0].controlPanel("First RED Control");
	//HSVred_ranges[1].controlPanel("Second RED Control");

	HSVgreen_ranges.push_back(ThreeChannelRange(70, 100, 100, 255, 100, 255));


	// Variables / Objects
	vector<TrafficLight> storedTrafficLight;
	

	Mat frame;
	

	int state = MOVING;
	int speed = 10;
	int count = 0;
	cv::namedWindow("Control Panel", CV_WINDOW_AUTOSIZE); //create a window called "Control"
	cvCreateTrackbar("speed", "Control Panel", &speed, 100);
	int minRadius = 5;
	int maxRadius = 30;

	//Threshold
	int minThresh = 250;
	int maxThresh = 255;

	// Morphological
	int dilation_elem = 0;
	int dilation_size = 3;
	int const max_elem = 2;
	int const max_kernel_size = 21;

	int dilation_type = MORPH_ELLIPSE;

	Mat element = getStructuringElement(dilation_type,
		Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		Point(dilation_size, dilation_size));


	// Video Capture Related
	cv::VideoCapture capture("야간/1.avi");
	if (!capture.isOpened())
		return -1;

	double rate = capture.get(CV_CAP_PROP_FPS);
	int delay = 1000 / rate;
	int frameNumb = 0;
	bool stop = false;

	clock_t begin, finish;
	


	while (true) {
		if (!stop)
		{
			frameNumb++;
			std::cout << "framenumber = " << frameNumb << endl;// << "frame row = " << frame.rows << " frame col" << frame.cols << std::endl;
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


			Mat intensity_frame, red_detected;
			vector<Mat> frame_channels;
			vector<Mat> trafficsRois;
			int maxVote = 0;

			switch (state){
			case MOVING:
				if (speed == 0){
					state = STOP_SEEK_FIRST_RED;
					begin = clock();
				}

				break;

			case STOP_SEEK_FIRST_RED:

				//empty storedTrafficLight when the car speed up again
				if (speed > 0){
					storedTrafficLight.clear();
					if (storedTrafficLight.empty()){
						cout << "Success Clearing";
						state = MOVING;
						break;
					}
				}

				

				// Threshold & Dilate (in order to leave the colored which is not high enough)
				// Intensity of the Original Input Image
				//findHighIntensity(frame, intensity_frame, 250, 255);
				

				cvtColor(frame, intensity_frame, COLOR_BGR2GRAY);
				threshold(intensity_frame, intensity_frame, minThresh, maxThresh, CV_THRESH_BINARY);
				/// Apply the dilation operation
				dilate(intensity_frame, intensity_frame, element);
				

				//Split the Original Frame & Apply Bitwise And operation with the intensity_frame
				split(frame, frame_channels);
				bitwise_and(intensity_frame, frame_channels[0], frame_channels[0]);
				bitwise_and(intensity_frame, frame_channels[1], frame_channels[1]);
				bitwise_and(intensity_frame, frame_channels[2], frame_channels[2]);
				merge(frame_channels, intensity_frame);
				
				imshow("INTENSITY", intensity_frame);
				// Convert intensity_frame to HSV color space
				cvtColor(intensity_frame, intensity_frame, cv::COLOR_BGR2HSV);
				
				findColor(intensity_frame, red_detected, HSVred_ranges);
				
				//imshow("Red_Detected ", red_detected);
				// find circle and store in temp TrafficLight list
				// if it new it will add or else vote up into storedTrafficLight
				findCircle(red_detected, storedTrafficLight, minRadius, maxRadius, frame.rows, frame.cols, RED);

				
				state = STOP_SEEK_N_RED;
				//state = STOP_RED_FOUND;
				/*
				if (storedTrafficLight.empty()){
					cout << "STILL EMPTY" << endl;
				}
				for (int i = storedTrafficLight.size() - 1; i >= 0; i--){
					
					//if (storedTrafficLight[i].getVote()>VOTE_MAX){ // If one of them is at higher than VOTE_MAX go to next State
						//circle(frame, storedTrafficLight[i].getCenter(), storedTrafficLight[i].getRadius(), cv::Scalar(30, 255, 30), 3);
						
						state = STOP_SEEK_N_RED;
						
						
						//finish = clock();
						//cout << "수행시간: " << ((double)(finish - begin) / CLOCKS_PER_SEC) << endl;
						//Sleep(1000);
					//}
				}
				*/
				break;
			case STOP_SEEK_N_RED:
				
				getTrafficLightFromList(frame, storedTrafficLight, trafficsRois);
				
				for (int i = trafficsRois.size() - 1; i >= 0; i--){

					imshow("ROI # " + to_string(i), trafficsRois[i]);
					
					cvtColor(trafficsRois[i], intensity_frame, COLOR_BGR2GRAY);
					threshold(intensity_frame, intensity_frame, minThresh, maxThresh, CV_THRESH_BINARY);
					/// Apply the dilation operation
					dilate(intensity_frame, intensity_frame, element);


					//Split the Original Frame & Apply Bitwise And operation with the intensity_frame
					split(trafficsRois[i], frame_channels);
					bitwise_and(intensity_frame, frame_channels[0], frame_channels[0]);
					bitwise_and(intensity_frame, frame_channels[1], frame_channels[1]);
					bitwise_and(intensity_frame, frame_channels[2], frame_channels[2]);
					merge(frame_channels, intensity_frame);

					//imshow("INTENSITY", intensity_frame);
					// Convert intensity_frame to HSV color space
					cvtColor(intensity_frame, intensity_frame, cv::COLOR_BGR2HSV);

					findColor(intensity_frame, red_detected, HSVred_ranges);

					// find circle and store in temp TrafficLight list
					// if it new it will add or else vote up into storedTrafficLight
					findCircleFromRoi(red_detected, storedTrafficLight, i, minRadius, maxRadius, RED);

					if (storedTrafficLight[i].getVote() > maxVote){
						maxVote = storedTrafficLight[i].getVote();
					}
				}

				if (maxVote > VOTE_MAX)
					state = STOP_RED_FOUND;
				else
					count++;
				break;

			case STOP_RED_FOUND:

				for (int i = storedTrafficLight.size() - 1; i >= 0; i--){

					if (storedTrafficLight[i].getVote() >=0){ // If one of them is at higher than VOTE_MAX go to next State
						circle(frame, storedTrafficLight[i].getCenter(), storedTrafficLight[i].getRadius(), cv::Scalar(30, 255, 30), 3);
						
					}

				}
				finish = clock();
				cout << "수행시간: " << ((double)(finish - begin) / CLOCKS_PER_SEC) << endl;
				break;
			default:
				break;
			}

			imshow("Original Frame", frame);
			/*
			Mat red_detected, HSV_frame;
			
			
			cvtColor(frame, HSV_frame, CV_BGR2HSV);
			findColor(HSV_frame, red_detected, HSVred_ranges);
			findCircle(red_detected.clone(), tempTrafficLightList); // may not need to Clone later

			//TrafficLight::voteUpOrAdd_IfRadiusIsBetween(tempTrafficLightList, storedTrafficLight, 5, 20);

			for (int i = tempTrafficLightList.size() - 1; i >= 0; i--){

				//if (storedTrafficLight[i].getVote()>VOTE_MAX){ // If one of them is at higher than VOTE_MAX go to next State
				circle(frame, tempTrafficLightList[i].getCenter(), tempTrafficLightList[i].getRadius(), cv::Scalar(30, 255, 30), 3);
					
				//}
			}
			tempTrafficLightList.clear();

			imshow("Original Frame", frame);
			imshow("red_detected Frame", red_detected);
			*/

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