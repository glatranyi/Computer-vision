#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>

#include<iostream>
#include<conio.h>           

#include "Blob.h"

#define SHOW_STEPS     // kommenteld ki ezt a sort ha l�tni akarod l�p�seket


// glob�lis v�ltoz�k  (sz�nek) ///////////////////////////////////////////////////////////////////////////////
const cv::Scalar SCALAR_BLACK = cv::Scalar(0.0, 0.0, 0.0);
const cv::Scalar SCALAR_WHITE = cv::Scalar(255.0, 255.0, 255.0);
const cv::Scalar SCALAR_YELLOW = cv::Scalar(0.0, 255.0, 255.0);
const cv::Scalar SCALAR_GREEN = cv::Scalar(0.0, 200.0, 0.0);
const cv::Scalar SCALAR_RED = cv::Scalar(0.0, 0.0, 255.0);

// f�ggv�nyek ////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs);
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex);
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs);
double distanceBetweenPoints(cv::Point point1, cv::Point point2);
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName);
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName);
bool checkIfBlobsCrossedTheLine(std::vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount);
void drawBlobInfoOnImage(std::vector<Blob> &blobs, cv::Mat &imgFrame2Copy);
void drawCarCountOnImage(int &carCount, cv::Mat &imgFrame2Copy);

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(void) {

	cv::VideoCapture capVideo;

	cv::Mat imgFrame1;
	cv::Mat imgFrame2;

	std::vector<Blob> blobs;

	cv::Point crossingLine[2];

	int carCount = 0;

	capVideo.open("Teszt1.mp4"); //UK Motorway M25 Trucks, Lorries, Cars Highway.mp4  videoblocks-aerial-view-on-the-driving-cars-on-bridge-vehicles-moving-on-the-street_rrgwh-2b__96ec38126bc3776ab0cd3b99619ede4e__P360.mp4

	if (!capVideo.isOpened()) {                                                 // ha nemlehet megnyitni vide� f�jlt
		std::cout << "error reading video file" << std::endl << std::endl;      // �rjunk ki error-t
		_getch();     // billenty�lenyom�sra v�r               
		return(0);                                                             
	}

	if (capVideo.get(cv::CAP_PROP_FRAME_COUNT) < 2) {			// ha a vide� kevesebb mint 2 k�pkock�b�l �ll
		std::cout << "error: video file must have at least two frames"; 
		_getch();   // billenty�lenyom�sra v�r                
		return(0);
	}

	capVideo.read(imgFrame1);
	capVideo.read(imgFrame2);

	int intHorizontalLinePosition = (int)std::round((double)imgFrame1.rows * 0.35);

	crossingLine[0].x = 0;
	crossingLine[0].y = intHorizontalLinePosition;

	crossingLine[1].x = imgFrame1.cols - 1;
	crossingLine[1].y = intHorizontalLinePosition;

	char chCheckForEscKey = 0;

	bool blnFirstFrame = true;

	int frameCount = 2;

	while (capVideo.isOpened() && chCheckForEscKey != 27) {   //ameddig a vide� meg van nyitva �s nem �t�ttek ESC-et

		std::vector<Blob> currentFrameBlobs;

		cv::Mat imgFrame1Copy = imgFrame1.clone();  //beolvas�s
		cv::Mat imgFrame2Copy = imgFrame2.clone();  //beolvas�s

		cv::Mat imgDifference;
		cv::Mat imgThresh;

		cv::cvtColor(imgFrame1Copy, imgFrame1Copy, cv::COLOR_BGR2GRAY);  //sz�rke�rnyalat
		cv::cvtColor(imgFrame2Copy, imgFrame2Copy, cv::COLOR_BGR2GRAY);  //sz�rke�rnyalat

		cv::GaussianBlur(imgFrame1Copy, imgFrame1Copy, cv::Size(5, 5), 0);
		cv::GaussianBlur(imgFrame2Copy, imgFrame2Copy, cv::Size(5, 5), 0);

		cv::absdiff(imgFrame1Copy, imgFrame2Copy, imgDifference); //k�t k�pkocka kivon�sa �s elment�se imgDifference-be
		//cv::imshow("imgDiff", imgDifference); #teszt

		cv::threshold(imgDifference, imgThresh, 30, 255.0, cv::THRESH_BINARY);  //forr�sk�p,kimeneti k�p,k�sz�b�rt�k(mi �s mi legyen),bin�ris k�sz�b�l�s. Azon �rt�kek, amelyek nagyobbak a 30,30,30(s�t�tsz�rke) intenzit�s k�sz�bn�l, a 255 (fekete) param�ter �rt�k�t kapj�k, a t�bbi helyen 0 lesz.

		cv::imshow("imgThresh", imgThresh);

		cv::Mat structuringElement3x3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)); //teszt
		cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5)); 
		cv::Mat structuringElement7x7 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7)); //teszt
		cv::Mat structuringElement15x15 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15)); //teszt

		for (unsigned int i = 0; i < 2; i++) {
			cv::dilate(imgThresh, imgThresh, structuringElement5x5);  //ny�jt�s
			cv::dilate(imgThresh, imgThresh, structuringElement5x5); //ny�jt�s
			cv::erode(imgThresh, imgThresh, structuringElement5x5); //elkoptat�s
		}

		cv::Mat imgThreshCopy = imgThresh.clone();

		std::vector<std::vector<cv::Point> > contours;

		cv::findContours(imgThreshCopy, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE); //kont�r hierarchia...

		drawAndShowContours(imgThresh.size(), contours, "imgContours");

		std::vector<std::vector<cv::Point> > convexHulls(contours.size());

		for (unsigned int i = 0; i < contours.size(); i++) {
			cv::convexHull(contours[i], convexHulls[i]);
		}

		drawAndShowContours(imgThresh.size(), convexHulls, "imgConvexHulls");

		for (auto &convexHull : convexHulls) {
			Blob possibleBlob(convexHull);

			if (possibleBlob.currentBoundingRect.area() > 400 &&             //ami nem el�g nagy/kicsi ne legyen hull
				possibleBlob.dblCurrentAspectRatio > 0.2 &&
				possibleBlob.dblCurrentAspectRatio < 4.0 &&
				possibleBlob.currentBoundingRect.width > 30 &&
				possibleBlob.currentBoundingRect.height > 30 &&
				possibleBlob.dblCurrentDiagonalSize > 60.0 &&
				(cv::contourArea(possibleBlob.currentContour) / (double)possibleBlob.currentBoundingRect.area()) > 0.50) {
				currentFrameBlobs.push_back(possibleBlob);
			}
		}

		drawAndShowContours(imgThresh.size(), currentFrameBlobs, "imgCurrentFrameBlobs");

		if (blnFirstFrame == true) { //els� k�pkock�n
			for (auto &currentFrameBlob : currentFrameBlobs) {
				blobs.push_back(currentFrameBlob); //legyen mindegyik v�gleges Blob
			}
		}
		else {
			matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs); // megl�v� Blob-hoz matchelj�k (melyik van legk�zelebb)
		}

		drawAndShowContours(imgThresh.size(), blobs, "imgBlobs");

		imgFrame2Copy = imgFrame2.clone();          // m�gegyszer frame 2 mert megv�ltoztattuk azt

		drawBlobInfoOnImage(blobs, imgFrame2Copy);

		bool blnAtLeastOneBlobCrossedTheLine = checkIfBlobsCrossedTheLine(blobs, intHorizontalLinePosition, carCount);

		if (blnAtLeastOneBlobCrossedTheLine == true) {
			cv::line(imgFrame2Copy, crossingLine[0], crossingLine[1], SCALAR_GREEN, 2);
		}
		else {
			cv::line(imgFrame2Copy, crossingLine[0], crossingLine[1], SCALAR_RED, 2);
		}

		drawCarCountOnImage(carCount, imgFrame2Copy);

		cv::imshow("imgFrame2Copy", imgFrame2Copy);

		//cv::waitKey(0);                 // debughoz kommenteld vissza

		currentFrameBlobs.clear();

		imgFrame1 = imgFrame2.clone();           // els� k�pkock�t a m�sodik hely�re mozgatja

		if ((capVideo.get(cv::CAP_PROP_POS_FRAMES) + 1) < capVideo.get(cv::CAP_PROP_FRAME_COUNT)) {
			capVideo.read(imgFrame2);
		}
		else {
			std::cout << "end of video\n";
			break;
		}

		blnFirstFrame = false;
		frameCount++;
		chCheckForEscKey = cv::waitKey(1);
	}

	if (chCheckForEscKey != 27) {               // ha a felhaszn�l� m�g nem �t�tt ESC-et (vagyis el�rt�k a vide� v�g�t)
		cv::waitKey(0);                         // nyitvatartja az ablakokat �s kiir�juk az "end of video" -sz�veget
	}
	// ha a felhaszn�l� nem �t�tt ESC-et nemkell az ablakokat nyitva tartanunk csak a programbol lepunk ki ami bez�rja az ablakokat

	return(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs) {

	for (auto &existingBlob : existingBlobs) {

		existingBlob.blnCurrentMatchFoundOrNewBlob = false; //alap�rtelmezetten	 hamisra �ll�tjuk, hogy nincs match-e

		existingBlob.predictNextPosition();   
	}

	for (auto &currentFrameBlob : currentFrameBlobs) {  //a jelenlegi k�pkock�n szerepl� Blob-okon v�gigmegy�nk

		int intIndexOfLeastDistance = 0;
		double dblLeastDistance = 100000.0;

		for (unsigned int i = 0; i < existingBlobs.size(); i++) {  //a m�r l�tez� Blob-okon v�gigmegy�nk

			if (existingBlobs[i].blnStillBeingTracked == true) {  

				double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);

				if (dblDistance < dblLeastDistance) {
					dblLeastDistance = dblDistance;
					intIndexOfLeastDistance = i;
				}
			}
		}

		if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize * 0.5) { //ha a current blob tresholdja(konvex burka �tl�san *0,5) akkor
			addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance); //a megl�v�h�z hozz�adjuk
		}
		else {
			addNewBlob(currentFrameBlob, existingBlobs);  //csakis �j Blob lehet �gy l�trehozunk egyet
		}

	}

	for (auto &existingBlob : existingBlobs) {   

		if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {   // ha m�r megtal�ltuk az �sszes Blob-ot ugrunk a k�vetkez� k�pkock�ra
			existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
		}
			
		if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 5) {   // biztos, hogy kiment a k�pr�l, nem k�vetj�k tov�bb
			existingBlob.blnStillBeingTracked = false;
		}

	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex) {

	existingBlobs[intIndex].currentContour = currentFrameBlob.currentContour;
	existingBlobs[intIndex].currentBoundingRect = currentFrameBlob.currentBoundingRect;

	existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());

	existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
	existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;

	existingBlobs[intIndex].blnStillBeingTracked = true;
	existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs) {

	currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;

	existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(cv::Point point1, cv::Point point2) {

	int intX = abs(point1.x - point2.x);
	int intY = abs(point1.y - point2.y);

	return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName) {
	cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

	cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

	cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName) {

	cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

	std::vector<std::vector<cv::Point> > contours;

	for (auto &blob : blobs) {
		if (blob.blnStillBeingTracked == true) {
			contours.push_back(blob.currentContour);
		}
	}

	cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

	cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLine(std::vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount) {
	bool blnAtLeastOneBlobCrossedTheLine = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[prevFrameIndex].y > intHorizontalLinePosition && blob.centerPositions[currFrameIndex].y <= intHorizontalLinePosition) {  // blob poz�ci� az el�z� k�pkock�ban alatta vagy f�l�tte lett
				carCount++;
				blnAtLeastOneBlobCrossedTheLine = true;
			}
		}

	}

	return blnAtLeastOneBlobCrossedTheLine; // ha kereszt�l megy a s�v-on
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawBlobInfoOnImage(std::vector<Blob> &blobs, cv::Mat &imgFrame2Copy) { //kiirat�s aut�inf� (keret,id)

	for (unsigned int i = 0; i < blobs.size(); i++) {

		if (blobs[i].blnStillBeingTracked == true) {
			cv::rectangle(imgFrame2Copy, blobs[i].currentBoundingRect, SCALAR_RED, 2);

			int intFontFace = cv::FONT_HERSHEY_SIMPLEX;
			double dblFontScale = blobs[i].dblCurrentDiagonalSize / 60.0;
			int intFontThickness = (int)std::round(dblFontScale * 1.0);

			cv::putText(imgFrame2Copy, std::to_string(i), blobs[i].centerPositions.back(), intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawCarCountOnImage(int &carCount, cv::Mat &imgFrame2Copy) { // kiirat�s aut��sszeg jobb fels�

	int intFontFace = cv::FONT_HERSHEY_SIMPLEX;
	double dblFontScale = (imgFrame2Copy.rows * imgFrame2Copy.cols) / 300000.0;
	int intFontThickness = (int)std::round(dblFontScale * 1.5);

	cv::Size textSize = cv::getTextSize(std::to_string(carCount), intFontFace, dblFontScale, intFontThickness, 0);

	cv::Point ptTextBottomLeftPosition;

	ptTextBottomLeftPosition.x = imgFrame2Copy.cols - 1 - (int)((double)textSize.width * 1.25);
	ptTextBottomLeftPosition.y = (int)((double)textSize.height * 1.25);

	cv::putText(imgFrame2Copy, std::to_string(carCount), ptTextBottomLeftPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);

}