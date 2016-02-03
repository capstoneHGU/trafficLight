#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <time.h>

using namespace cv;
using namespace std;

int main2()
{
	Mat original;
	Mat orig_copy;

	Mat gamma_outcome;

	Mat red;
	Mat red_gamma;
	Mat red_temp;
	Mat green;
	Mat green_gamma;
	Mat green_temp;
	Mat blue;
	Mat blue_gamma;
	Mat blue_temp;

	Mat blur;
	Mat thresh_temp;
	Mat gray;
	Mat roi_red, roi_red_equalized, red_thresh;

	vector<Mat> channels(3);
	vector<Mat> gamma_merge(3);

	bool stop(false);
	double gamma = 0.5;
	double frameNumb = 0;
	int times = 0, sum = 0;


	clock_t begin, end;

	// Video
	VideoCapture capture("야간/REC_2015_07_21_21_50_55_D.avi");
	double fpsNum = capture.get(CV_CAP_PROP_FPS);

	if (!capture.isOpened())
		return -1;

	// get frame

	int delay = 1000 / fpsNum;

	for (;;){
		//read if next frame
		if (!stop){

			begin = clock();

			frameNumb++;
			cout << "framenumb = " << frameNumb << endl;

			if (!capture.read(original))
				break;
			
			if ((int)frameNumb % 3 == 0){
				

			}
			else
			{
				continue;
			}

			orig_copy = original.clone();

			//resize the video to 50%
			//resize(orig_copy, orig_copy, Size(), 0.5, 0.5);

			cvtColor(orig_copy, gray, CV_BGR2GRAY);

			//split the channels into three and gamma transform
			split(orig_copy, channels);

			red = channels[2].clone();
			green = channels[1].clone();
			blue = channels[0].clone();

			red.convertTo(red_temp, CV_32F);
			pow(red_temp, gamma, red_gamma);
			normalize(red_gamma, red_gamma, 0, 1.0, NORM_MINMAX);
			red_gamma = 255 * red_gamma;
			red_gamma.convertTo(red_gamma, CV_8U);

			green.convertTo(green_temp, CV_32F);
			pow(green_temp, gamma, green_gamma);
			normalize(green_gamma, green_gamma, 0, 1.0, NORM_MINMAX);
			green_gamma = 255 * green_gamma;
			green_gamma.convertTo(green_gamma, CV_8U);

			blue.convertTo(blue_temp, CV_32F);
			pow(blue_temp, gamma, blue_gamma);
			normalize(blue_gamma, blue_gamma, 0, 1.0, NORM_MINMAX);
			blue_gamma = 255 * blue_gamma;
			blue_gamma.convertTo(blue_gamma, CV_8U);

			gamma_merge[2] = red_gamma.clone();
			gamma_merge[1] = green_gamma.clone();
			gamma_merge[0] = blue_gamma.clone();

			//merge the channels
			merge(gamma_merge, gamma_outcome);
			
			imshow("merged", gamma_outcome);
			imshow("back_light", orig_copy);

			
			
			//end = clock();
			//cout << "수행시간: " << ((double)(end - begin) / CLOCKS_PER_SEC) << endl;
		}

		int key = waitKey(delay);

		if (key == 32) // space to stop and resume
		{
			if (stop == false)
				stop = true;
			else
				stop = false;
		}
		else if (key == 27)// esc to exit
		{
			break;
		}
		else if (key == 109)// m to frame+100
		{
			frameNumb = (frameNumb + 100);
			capture.set(CV_CAP_PROP_POS_FRAMES, frameNumb);
		}
		else if (key == 110)// n to frame-100
		{
			frameNumb = (frameNumb - 10);
			if (frameNumb < 0)
				frameNumb = 0;
			capture.set(CV_CAP_PROP_POS_FRAMES, frameNumb);
		}
		else if (key == 97) // a to gamma - 0.1
		{
			gamma = gamma - 0.1;
			cout << "gamma = " << gamma << endl;
		}
		else if (key == 115)// s to gamma + 0.1
		{
			gamma = gamma + 0.1;
			cout << "gamma = " << gamma << endl;
		}
	}

	capture.release();

	return 0;
}
